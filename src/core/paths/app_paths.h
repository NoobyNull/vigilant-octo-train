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

// Tool database file path (.vtdb format)
Path getToolDatabasePath();

// Log file path
Path getLogPath();

// Content-addressable blob store directory
Path getBlobStoreDir();

// Temporary file directory for atomic writes (co-located with blob store)
Path getTempStoreDir();

// Materials directory (app-managed .dwmat files)
Path getMaterialsDir();

// User root directory (~/DigitalWorkshop)
Path getUserRoot();

// Default category directories under user root
Path getDefaultModelsDir();
Path getDefaultGCodeDir();
Path getDefaultMaterialsDir();
Path getDefaultSupportDir();

// Bundled materials directory (shipped .dwmat files next to the executable)
// Returns <exe_dir>/resources/materials/
Path getBundledMaterialsDir();

// Ensure all application directories exist
bool ensureDirectoriesExist();

// Get application name (used in paths)
const char* getAppName();

} // namespace paths
} // namespace dw
