#pragma once

#include "../types.h"

namespace dw {
namespace paths {

// Platform-specific application directories
// Linux:   ~/.config/digitalworkshop/, ~/.local/share/digitalworkshop/
// Windows: %APPDATA%/DigitalWorkshop/, %LOCALAPPDATA%/DigitalWorkshop/
// macOS:   ~/Library/Application Support/DigitalWorkshop/

// Configuration directory (settings, preferences)
Path getConfigDir();

// Data directory (database, cache, thumbnails)
Path getDataDir();

// Default user projects directory
Path getDefaultProjectsDir();

// Cache directory (temporary files, thumbnail cache)
Path getCacheDir();

// Thumbnail cache directory
Path getThumbnailDir();

// Database file path
Path getDatabasePath();

// Log file path
Path getLogPath();

// Materials directory (app-managed .dwmat files)
Path getMaterialsDir();

// Bundled materials directory (shipped .dwmat files next to the executable)
// Returns <exe_dir>/resources/materials/
Path getBundledMaterialsDir();

// Ensure all application directories exist
bool ensureDirectoriesExist();

// Get application name (used in paths)
const char* getAppName();

} // namespace paths
} // namespace dw
