#pragma once

#include "toolpath_types.h"

#include <string>

namespace dw {
namespace carve {

// Export toolpath to G-code file
// Returns true on success, false if file could not be written
bool exportGcode(const std::string& path,
                 const MultiPassToolpath& toolpath,
                 const ToolpathConfig& config,
                 const std::string& modelName,
                 const std::string& toolName);

// Generate G-code string (for testing without filesystem)
std::string generateGcode(const MultiPassToolpath& toolpath,
                          const ToolpathConfig& config,
                          const std::string& modelName,
                          const std::string& toolName);

} // namespace carve
} // namespace dw
