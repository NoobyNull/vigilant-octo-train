#pragma once

#include <string>

#include "../types.h"
#include "gcode_types.h"

namespace dw {
namespace gcode {

class Parser {
  public:
    Parser() = default;

    // Parse G-code from string
    Program parse(const std::string& content);

    // Parse G-code from file
    Program parseFile(const Path& path);

    // Get last error message
    const std::string& lastError() const { return m_lastError; }

  private:
    Command parseLine(const std::string& line, int lineNumber);
    CommandType parseCommandType(char letter, int number);
    f32 parseParameter(const std::string& line, char param);

    std::string m_lastError;
};

} // namespace gcode
} // namespace dw
