#include "gcode_parser.h"

#include "../utils/file_utils.h"
#include "../utils/log.h"
#include "../utils/string_utils.h"

#include <cmath>
#include <sstream>

namespace dw {
namespace gcode {

Program Parser::parse(const std::string& content) {
    Program program;
    m_lastError.clear();

    std::istringstream stream(content);
    std::string line;
    int lineNumber = 0;

    Vec3 currentPos{0.0f, 0.0f, 0.0f};
    f32 currentFeedRate = 0.0f;

    while (std::getline(stream, line)) {
        lineNumber++;

        // Trim and skip empty lines/comments
        line = str::trim(line);
        if (line.empty() || line[0] == ';' || line[0] == '(') {
            continue;
        }

        // Remove inline comments
        auto commentPos = line.find(';');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
            line = str::trim(line);
        }

        Command cmd = parseLine(line, lineNumber);
        cmd.raw = line;

        // Handle unit changes
        if (cmd.type == CommandType::G20) {
            program.units = Units::Inches;
        } else if (cmd.type == CommandType::G21) {
            program.units = Units::Millimeters;
        }

        // Handle positioning mode
        if (cmd.type == CommandType::G90) {
            program.positioning = PositioningMode::Absolute;
        } else if (cmd.type == CommandType::G91) {
            program.positioning = PositioningMode::Relative;
        }

        // Update feed rate if specified
        if (cmd.hasF()) {
            currentFeedRate = cmd.f;
        }

        // Generate path segments for motion commands
        if (cmd.isMotion()) {
            Vec3 targetPos = currentPos;

            if (program.positioning == PositioningMode::Absolute) {
                if (cmd.hasX()) targetPos.x = cmd.x;
                if (cmd.hasY()) targetPos.y = cmd.y;
                if (cmd.hasZ()) targetPos.z = cmd.z;
            } else {
                if (cmd.hasX()) targetPos.x += cmd.x;
                if (cmd.hasY()) targetPos.y += cmd.y;
                if (cmd.hasZ()) targetPos.z += cmd.z;
            }

            PathSegment segment;
            segment.start = currentPos;
            segment.end = targetPos;
            segment.isRapid = (cmd.type == CommandType::G0);
            segment.feedRate = cmd.hasF() ? cmd.f : currentFeedRate;
            segment.lineNumber = lineNumber;

            program.path.push_back(segment);

            // Update bounds
            auto updateBounds = [&](const Vec3& p) {
                if (program.path.size() == 1) {
                    program.boundsMin = p;
                    program.boundsMax = p;
                } else {
                    program.boundsMin.x = std::min(program.boundsMin.x, p.x);
                    program.boundsMin.y = std::min(program.boundsMin.y, p.y);
                    program.boundsMin.z = std::min(program.boundsMin.z, p.z);
                    program.boundsMax.x = std::max(program.boundsMax.x, p.x);
                    program.boundsMax.y = std::max(program.boundsMax.y, p.y);
                    program.boundsMax.z = std::max(program.boundsMax.z, p.z);
                }
            };

            updateBounds(currentPos);
            updateBounds(targetPos);

            currentPos = targetPos;
        }

        program.commands.push_back(cmd);
    }

    return program;
}

Program Parser::parseFile(const Path& path) {
    auto content = file::readText(path);
    if (!content) {
        m_lastError = "Failed to read file: " + path.string();
        log::error(m_lastError);
        return Program{};
    }

    return parse(*content);
}

Command Parser::parseLine(const std::string& line, int lineNumber) {
    Command cmd;
    cmd.lineNumber = lineNumber;

    std::string upper = str::toUpper(line);

    // Parse command code (G or M followed by number)
    for (usize i = 0; i < upper.size(); ++i) {
        char c = upper[i];

        if (c == 'G' || c == 'M') {
            // Find the number following the letter
            usize numStart = i + 1;
            while (numStart < upper.size() && upper[numStart] == ' ') {
                numStart++;
            }

            int number = 0;
            if (numStart < upper.size() && std::isdigit(upper[numStart])) {
                std::string numStr;
                while (numStart < upper.size() && std::isdigit(upper[numStart])) {
                    numStr += upper[numStart];
                    numStart++;
                }
                number = std::stoi(numStr);
            }

            CommandType type = parseCommandType(c, number);
            if (type != CommandType::Unknown) {
                cmd.type = type;
            }
        }
    }

    // Parse parameters
    cmd.x = parseParameter(upper, 'X');
    cmd.y = parseParameter(upper, 'Y');
    cmd.z = parseParameter(upper, 'Z');
    cmd.i = parseParameter(upper, 'I');
    cmd.j = parseParameter(upper, 'J');
    cmd.r = parseParameter(upper, 'R');
    cmd.f = parseParameter(upper, 'F');
    cmd.s = parseParameter(upper, 'S');

    // Parse tool number
    f32 t = parseParameter(upper, 'T');
    if (!std::isnan(t)) {
        cmd.t = static_cast<int>(t);
    }

    return cmd;
}

CommandType Parser::parseCommandType(char letter, int number) {
    if (letter == 'G') {
        switch (number) {
            case 0: return CommandType::G0;
            case 1: return CommandType::G1;
            case 2: return CommandType::G2;
            case 3: return CommandType::G3;
            case 20: return CommandType::G20;
            case 21: return CommandType::G21;
            case 28: return CommandType::G28;
            case 90: return CommandType::G90;
            case 91: return CommandType::G91;
            case 92: return CommandType::G92;
            default: return CommandType::Unknown;
        }
    } else if (letter == 'M') {
        switch (number) {
            case 0: return CommandType::M0;
            case 1: return CommandType::M1;
            case 2: return CommandType::M2;
            case 3: return CommandType::M3;
            case 4: return CommandType::M4;
            case 5: return CommandType::M5;
            case 6: return CommandType::M6;
            case 30: return CommandType::M30;
            default: return CommandType::Unknown;
        }
    }

    return CommandType::Unknown;
}

f32 Parser::parseParameter(const std::string& line, char param) {
    usize pos = line.find(param);
    if (pos == std::string::npos) {
        return std::numeric_limits<f32>::quiet_NaN();
    }

    pos++;  // Move past the parameter letter

    // Skip spaces
    while (pos < line.size() && line[pos] == ' ') {
        pos++;
    }

    if (pos >= line.size()) {
        return std::numeric_limits<f32>::quiet_NaN();
    }

    // Extract number (including sign and decimal)
    std::string numStr;
    if (line[pos] == '-' || line[pos] == '+') {
        numStr += line[pos];
        pos++;
    }

    while (pos < line.size() && (std::isdigit(line[pos]) || line[pos] == '.')) {
        numStr += line[pos];
        pos++;
    }

    if (numStr.empty() || numStr == "-" || numStr == "+") {
        return std::numeric_limits<f32>::quiet_NaN();
    }

    try {
        return std::stof(numStr);
    } catch (...) {
        return std::numeric_limits<f32>::quiet_NaN();
    }
}

}  // namespace gcode
}  // namespace dw
