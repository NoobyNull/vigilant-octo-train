#include "gemini_material_service.h"

#include <cstdlib>
#include <fstream>

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include "../paths/app_paths.h"
#include "../utils/log.h"
#include "material_archive.h"

namespace dw {

// ---------------------------------------------------------------------------
// curl helpers
// ---------------------------------------------------------------------------

namespace {

size_t writeCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* buffer = static_cast<std::string*>(userdata);
    size_t bytes = size * nmemb;
    buffer->append(ptr, bytes);
    return bytes;
}

// POST JSON to a URL, return response body. Empty string on failure.
std::string curlPost(const std::string& url, const std::string& body) {
    CURL* curl = curl_easy_init();
    if (curl == nullptr) {
        return {};
    }

    std::string response;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, static_cast<long>(body.size()));
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);

    CURLcode res = curl_easy_perform(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        log::errorf("GeminiService", "curl error: %s", curl_easy_strerror(res));
        return {};
    }
    return response;
}

// Base64 decode (standard alphabet)
std::vector<uint8_t> base64Decode(const std::string& encoded) {
    static constexpr unsigned char kTable[128] = {
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
        64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
        64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
        15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
        64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
        41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
    };

    std::vector<uint8_t> out;
    out.reserve(encoded.size() * 3 / 4);

    uint32_t val = 0;
    int bits = -8;
    for (unsigned char c : encoded) {
        if (c == '=' || c == '\n' || c == '\r') {
            continue;
        }
        if (c >= 128 || kTable[c] == 64) {
            continue;
        }
        val = (val << 6) | kTable[c];
        bits += 6;
        if (bits >= 0) {
            out.push_back(static_cast<uint8_t>((val >> bits) & 0xFF));
            bits -= 8;
        }
    }
    return out;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// API calls
// ---------------------------------------------------------------------------

namespace {
const char* kGeminiApiBase = "https://generativelanguage.googleapis.com/v1beta/models/";
} // anonymous namespace

std::string GeminiMaterialService::fetchProperties(const std::string& prompt,
                                                    const std::string& apiKey) {
    std::string url = std::string(kGeminiApiBase) +
                      "gemini-3-flash-preview:generateContent?key=" + apiKey;

    // Build request JSON
    nlohmann::json schema;
    schema["type"] = "OBJECT";
    schema["properties"] = {
        {"category", {{"type", "STRING"}}},
        {"description", {{"type", "STRING"}}},
        {"density", {{"type", "NUMBER"}}},
        {"hardness", {{"type", "STRING"}}},
        {"colorHex", {{"type", "STRING"}}},
        {"recommendedFeedRate", {{"type", "NUMBER"}}},
        {"recommendedSpindleSpeed", {{"type", "NUMBER"}}},
        {"recommendedDepthPerPass", {{"type", "NUMBER"}}},
        {"recommendedToolType", {{"type", "STRING"}}}};
    schema["required"] = nlohmann::json::array(
        {"category", "description", "density", "hardness", "colorHex", "recommendedFeedRate",
         "recommendedSpindleSpeed", "recommendedDepthPerPass", "recommendedToolType"});

    nlohmann::json requestBody;
    requestBody["systemInstruction"]["parts"] = nlohmann::json::array(
        {{{"text",
           "You are a material scientist and CNC fabrication specialist. "
           "Analyze the material name provided and return a comprehensive technical profile. "
           "Categorize the material into: Wood, Metal, Plastic, Composite, Foam, or Other. "
           "Include CNC parameters like feed rate (mm/min), spindle speed (RPM), and depth per "
           "pass (mm). "
           "Always interpret ambiguous names (Cherry, Bass, Zebra, Canary) as wood timber. "
           "Return valid JSON only matching the schema."}}});
    requestBody["contents"] = nlohmann::json::array(
        {{{"parts", nlohmann::json::array(
                        {{{"text", "Analyze the material: \"" + prompt +
                                       "\". Provide technical physical properties and CNC "
                                       "machining parameters."}}})}}});
    requestBody["generationConfig"]["responseMimeType"] = "application/json";
    requestBody["generationConfig"]["responseSchema"] = schema;

    std::string response = curlPost(url, requestBody.dump());
    if (response.empty()) {
        return {};
    }

    // Extract the text content from Gemini response
    try {
        auto json = nlohmann::json::parse(response);
        auto& candidates = json["candidates"];
        if (candidates.empty()) {
            log::error("GeminiService", "No candidates in properties response");
            return {};
        }
        auto& parts = candidates[0]["content"]["parts"];
        if (parts.empty()) {
            log::error("GeminiService", "No parts in properties response");
            return {};
        }
        return parts[0]["text"].get<std::string>();
    } catch (const nlohmann::json::exception& e) {
        log::errorf("GeminiService", "Failed to parse properties response: %s", e.what());
        return {};
    }
}

std::vector<uint8_t> GeminiMaterialService::fetchTexture(const std::string& prompt,
                                                          const std::string& apiKey) {
    std::string url = std::string(kGeminiApiBase) +
                      "gemini-2.5-flash-image:generateContent?key=" + apiKey;

    nlohmann::json requestBody = {
        {"contents",
         {{{"parts",
            {{{"text", "Generate a high-resolution, seamless texture map of " + prompt +
                           ". "
                           "Format: Flat orthographic top-down view. "
                           "Lighting: Uniform flat lighting, no shadows, no 3D depth. "
                           "Subject: A raw material surface grain only."}}}}}}},
        {"generationConfig",
         {{"responseModalities", nlohmann::json::array({"IMAGE"})},
          {"imageSizeOptions", {{"aspectRatio", "1:1"}}}}}};


    std::string response = curlPost(url, requestBody.dump());
    if (response.empty()) {
        return {};
    }

    // Extract base64 image data from Gemini response
    try {
        auto json = nlohmann::json::parse(response);
        if (!json.contains("candidates") || json["candidates"].empty()) {
            if (json.contains("promptFeedback")) {
                log::error("GeminiService", "Texture request blocked by safety filter");
            } else {
                std::string detail = response.substr(0, 500);
                log::errorf("GeminiService", "Texture response has no candidates: %s",
                            detail.c_str());
            }
            return {};
        }
        auto& candidates = json["candidates"];

        auto& parts = candidates[0]["content"]["parts"];
        for (auto& part : parts) {
            if (part.contains("inlineData")) {
                std::string base64 = part["inlineData"]["data"].get<std::string>();
                return base64Decode(base64);
            }
        }
        log::error("GeminiService", "No image data in texture response");
        return {};
    } catch (const nlohmann::json::exception& e) {
        log::errorf("GeminiService", "Failed to parse texture response: %s", e.what());
        return {};
    }
}

// ---------------------------------------------------------------------------
// Property parsing + category mapping
// ---------------------------------------------------------------------------

MaterialRecord GeminiMaterialService::parseProperties(const std::string& json,
                                                       const std::string& name) {
    MaterialRecord record;
    record.name = name;

    try {
        auto props = nlohmann::json::parse(json);

        // Map category string to MaterialCategory enum
        std::string category = props.value("category", "Other");
        if (category == "Wood") {
            record.category = MaterialCategory::Hardwood;
        } else {
            record.category = MaterialCategory::Composite;
        }

        // Parse hardness string (e.g. "1290 lbf") -> float
        std::string hardnessStr = props.value("hardness", "0");
        try {
            record.jankaHardness = std::stof(hardnessStr);
        } catch (...) {
            record.jankaHardness = 0.0f;
        }

        // Convert mm/min -> in/min (divide by 25.4)
        float feedRateMm = props.value("recommendedFeedRate", 0.0f);
        record.feedRate = feedRateMm / 25.4f;

        // RPM direct
        record.spindleSpeed = props.value("recommendedSpindleSpeed", 0.0f);

        // Convert mm -> in (divide by 25.4)
        float depthMm = props.value("recommendedDepthPerPass", 0.0f);
        record.depthOfCut = depthMm / 25.4f;

    } catch (const nlohmann::json::exception& e) {
        log::errorf("GeminiService", "Failed to parse material properties: %s", e.what());
    }

    return record;
}

// ---------------------------------------------------------------------------
// Main generate flow
// ---------------------------------------------------------------------------

GenerateResult GeminiMaterialService::generate(const std::string& prompt,
                                                const std::string& apiKey) {
    GenerateResult result;

    log::infof("GeminiService", "Generating material: %s", prompt.c_str());

    // Fetch properties and texture (sequentially to keep it simple)
    std::string propsJson = fetchProperties(prompt, apiKey);
    if (propsJson.empty()) {
        result.error = "Failed to fetch material properties from Gemini API";
        return result;
    }

    std::vector<uint8_t> textureData = fetchTexture(prompt, apiKey);
    if (textureData.empty()) {
        result.error = "Failed to fetch texture image from Gemini API";
        return result;
    }

    // Parse properties into MaterialRecord
    result.record = parseProperties(propsJson, prompt);

    // Write texture PNG to a temp file for MaterialArchive::create()
    Path tempDir = paths::getCacheDir();
    Path tempTexture = tempDir / (prompt + "_texture.png");
    {
        std::ofstream ofs(tempTexture, std::ios::binary);
        if (!ofs) {
            result.error = "Failed to write temporary texture file";
            return result;
        }
        ofs.write(reinterpret_cast<const char*>(textureData.data()),
                  static_cast<std::streamsize>(textureData.size()));
    }

    // Create .dwmat archive in materials directory
    Path materialsDir = paths::getMaterialsDir();
    Path archivePath = materialsDir / (prompt + ".dwmat");

    auto archiveResult =
        MaterialArchive::create(archivePath.string(), tempTexture.string(), result.record);
    if (!archiveResult.success) {
        result.error = "Failed to create .dwmat archive: " + archiveResult.error;
        std::error_code ec;
        fs::remove(tempTexture, ec);
        return result;
    }

    // Clean up temp texture file
    {
        std::error_code ec;
        fs::remove(tempTexture, ec);
    }

    result.dwmatPath = archivePath;
    result.record.archivePath = archivePath;
    result.success = true;

    log::infof("GeminiService", "Generated material archive: %s", archivePath.string().c_str());
    return result;
}

} // namespace dw
