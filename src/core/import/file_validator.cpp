#include "file_validator.h"

#include <algorithm>
#include <cctype>
#include <cstring>

#include "../utils/log.h"
#include "../utils/string_utils.h"

namespace dw {
namespace file_validator {

namespace {

// Check if a byte range is predominantly printable text (ASCII + common UTF-8).
float textFraction(const u8* data, size_t len) {
    if (len == 0) return 0.0f;
    size_t printable = 0;
    for (size_t i = 0; i < len; ++i) {
        u8 c = data[i];
        if ((c >= 32 && c <= 126) || c == '\t' || c == '\n' || c == '\r' || c >= 128)
            ++printable;
    }
    return static_cast<float>(printable) / static_cast<float>(len);
}

// Sample a buffer at multiple evenly-spaced offsets and check if each sample
// is valid text. Returns false if any sample falls below the threshold.
bool isTextThroughout(const ByteBuffer& data, size_t sampleSize = 512, int sampleCount = 8) {
    if (data.size() <= sampleSize)
        return textFraction(data.data(), data.size()) > 0.95f;

    size_t step = data.size() / static_cast<size_t>(sampleCount);
    for (int i = 0; i < sampleCount; ++i) {
        size_t offset = step * static_cast<size_t>(i);
        size_t len = std::min(sampleSize, data.size() - offset);
        if (textFraction(data.data() + offset, len) < 0.90f)
            return false;
    }
    return true;
}

// Check if a line looks like a valid G-code line.
bool isGCodeLine(const char* line, size_t len) {
    size_t i = 0;
    while (i < len && (line[i] == ' ' || line[i] == '\t')) ++i;
    if (i >= len) return true; // blank line

    char first = static_cast<char>(std::toupper(static_cast<unsigned char>(line[i])));

    if (first == ';' || first == '(' || first == '%') return true;

    static const char validStarts[] = "GMNTOXYZBCIJKFSPRHDALQ$";
    if (std::strchr(validStarts, first) != nullptr) return true;
    if (first >= '0' && first <= '9') return true;

    return false;
}

// Check if a line starts with an actual G-code command (not blank/comment).
bool isGCodeCommand(const char* line, size_t len) {
    size_t i = 0;
    while (i < len && (line[i] == ' ' || line[i] == '\t')) ++i;
    if (i >= len) return false;

    char first = static_cast<char>(std::toupper(static_cast<unsigned char>(line[i])));
    return first == 'G' || first == 'M' || first == 'N' || first == 'T' || first == '$';
}

// Validate binary STL: size must match triangle count within tolerance.
ValidationResult validateBinarySTL(const ByteBuffer& data, u32 triangleCount) {
    if (triangleCount == 0)
        return {false, "Binary STL declares 0 triangles"};

    size_t expectedSize = 84 + static_cast<size_t>(triangleCount) * 50;
    constexpr float TOLERANCE = 0.15f;
    size_t minSize = static_cast<size_t>(static_cast<float>(expectedSize) * (1.0f - TOLERANCE));
    size_t maxSize = static_cast<size_t>(static_cast<float>(expectedSize) * (1.0f + TOLERANCE));

    if (data.size() < minSize || data.size() > maxSize) {
        return {false, "File size does not match STL triangle count"
                       " (expected ~" + std::to_string(expectedSize) +
                       " bytes, got " + std::to_string(data.size()) + ")"};
    }
    return {true, ""};
}

// Validate ASCII STL: must be text with required keywords.
ValidationResult validateAsciiSTL(const ByteBuffer& data) {
    if (!isTextThroughout(data))
        return {false, "File starts with 'solid' but contains binary data"};

    size_t checkLen = std::min(data.size(), static_cast<size_t>(4096));
    std::string sample(reinterpret_cast<const char*>(data.data()), checkLen);
    std::string lower = str::toLower(sample);

    bool hasFacet = lower.find("facet") != std::string::npos;
    bool hasVertex = lower.find("vertex") != std::string::npos;
    bool hasEndsolid = lower.find("endsolid") != std::string::npos;

    if (!hasFacet && !hasVertex && !hasEndsolid)
        return {false, "ASCII STL missing required keywords (facet, vertex, endsolid)"};

    return {true, ""};
}

// Sample lines at an offset and count valid/command lines.
struct LineSampleResult {
    int totalLines = 0;
    int validLines = 0;
    int commandLines = 0;
};

LineSampleResult sampleGCodeLines(const ByteBuffer& data, size_t offset, int maxLines) {
    LineSampleResult result;

    // Align to start of a line
    while (offset < data.size() && offset > 0 && data[offset - 1] != '\n')
        ++offset;

    const char* ptr = reinterpret_cast<const char*>(data.data()) + offset;
    size_t remaining = data.size() - offset;

    for (int line = 0; line < maxLines && remaining > 0; ++line) {
        const char* eol = static_cast<const char*>(std::memchr(ptr, '\n', remaining));
        size_t lineLen = eol ? static_cast<size_t>(eol - ptr) : remaining;
        size_t cleanLen = lineLen;
        if (cleanLen > 0 && ptr[cleanLen - 1] == '\r') --cleanLen;

        if (isGCodeLine(ptr, cleanLen)) {
            ++result.validLines;
            if (isGCodeCommand(ptr, cleanLen))
                ++result.commandLines;
        }
        ++result.totalLines;

        if (!eol) break;
        size_t advance = lineLen + 1;
        ptr += advance;
        remaining -= advance;
    }

    return result;
}

} // anonymous namespace

ValidationResult validate(const ByteBuffer& data, const std::string& extension) {
    if (data.empty())
        return {false, "File is empty"};

    std::string ext = str::toLower(extension);

    if (ext == "stl") return validateSTL(data);
    if (ext == "obj") return validateOBJ(data);
    if (ext == "3mf") return validate3MF(data);
    if (ext == "gcode" || ext == "nc" || ext == "ngc" || ext == "tap") return validateGCode(data);

    return {false, "Unsupported format: " + extension};
}

ValidationResult validateSTL(const ByteBuffer& data) {
    if (data.size() < 84)
        return {false, "File too small for STL format (< 84 bytes)"};

    std::string start(reinterpret_cast<const char*>(data.data()),
                      std::min(data.size(), static_cast<size_t>(6)));
    bool startsWithSolid = str::toLower(start).find("solid") == 0;

    u32 triangleCount = 0;
    std::memcpy(&triangleCount, data.data() + 80, sizeof(triangleCount));

    size_t expectedSize = 84 + static_cast<size_t>(triangleCount) * 50;
    constexpr float TOLERANCE = 0.15f;
    size_t minSize = static_cast<size_t>(static_cast<float>(expectedSize) * (1.0f - TOLERANCE));
    size_t maxSize = static_cast<size_t>(static_cast<float>(expectedSize) * (1.0f + TOLERANCE));

    if (!startsWithSolid)
        return validateBinarySTL(data, triangleCount);

    // Starts with "solid" — could be ASCII or binary with "solid" in header
    if (data.size() >= minSize && data.size() <= maxSize && triangleCount > 0)
        return {true, ""}; // Size matches binary format despite "solid" header

    return validateAsciiSTL(data);
}

ValidationResult validateOBJ(const ByteBuffer& data) {
    if (!isTextThroughout(data))
        return {false, "OBJ file contains binary data"};

    size_t checkLen = std::min(data.size(), static_cast<size_t>(8192));
    std::string sample(reinterpret_cast<const char*>(data.data()), checkLen);

    bool hasVertexLine = false;
    bool hasFaceLine = false;

    size_t pos = 0;
    while (pos < sample.size()) {
        while (pos < sample.size() && (sample[pos] == ' ' || sample[pos] == '\t')) ++pos;

        if (pos < sample.size()) {
            char c = sample[pos];
            if (c == 'v' && pos + 1 < sample.size() &&
                (sample[pos + 1] == ' ' || sample[pos + 1] == '\t'))
                hasVertexLine = true;
            if (c == 'f' && pos + 1 < sample.size() &&
                (sample[pos + 1] == ' ' || sample[pos + 1] == '\t'))
                hasFaceLine = true;
        }

        while (pos < sample.size() && sample[pos] != '\n') ++pos;
        if (pos < sample.size()) ++pos;

        if (hasVertexLine && hasFaceLine) break;
    }

    if (!hasVertexLine)
        return {false, "OBJ file has no vertex data"};

    return {true, ""};
}

ValidationResult validate3MF(const ByteBuffer& data) {
    if (data.size() < 4)
        return {false, "File too small for 3MF format"};

    if (data[0] != 'P' || data[1] != 'K' || data[2] != 0x03 || data[3] != 0x04)
        return {false, "Not a valid ZIP/3MF archive (bad magic bytes)"};

    std::string haystack(reinterpret_cast<const char*>(data.data()), data.size());
    bool hasContentTypes = haystack.find("[Content_Types].xml") != std::string::npos;
    bool has3DModel = haystack.find("3dmodel.model") != std::string::npos;

    if (!hasContentTypes && !has3DModel)
        return {false, "ZIP archive does not contain 3MF content"};

    return {true, ""};
}

ValidationResult validateGCode(const ByteBuffer& data) {
    if (!isTextThroughout(data))
        return {false, "G-code file contains binary data"};

    constexpr int SAMPLE_POINTS = 10;
    constexpr int LINES_PER_SAMPLE = 20;

    int totalLines = 0;
    int validLines = 0;
    int commandLines = 0;

    size_t step = data.size() / SAMPLE_POINTS;
    for (int s = 0; s < SAMPLE_POINTS; ++s) {
        auto result = sampleGCodeLines(data, step * static_cast<size_t>(s), LINES_PER_SAMPLE);
        totalLines += result.totalLines;
        validLines += result.validLines;
        commandLines += result.commandLines;
    }

    if (totalLines == 0)
        return {false, "Could not sample any lines from G-code file"};

    float validRatio = static_cast<float>(validLines) / static_cast<float>(totalLines);
    if (validRatio < 0.80f) {
        return {false, "File content does not match G-code format"
                       " (" + std::to_string(static_cast<int>(validRatio * 100)) +
                       "% valid lines)"};
    }

    if (commandLines == 0)
        return {false, "No G-code commands found (G/M/T) — file may be plain text"};

    return {true, ""};
}

} // namespace file_validator
} // namespace dw
