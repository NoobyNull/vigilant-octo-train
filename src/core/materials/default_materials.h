#pragma once

#include <string>
#include <vector>

#include "material.h"

namespace dw {

// Returns the built-in list of default materials shipped with the app.
// These are seeded into the database on first run (when materials table is empty).
// All defaults have no archivePath or thumbnailPath (.dwmat files are user-provided).
std::vector<MaterialRecord> getDefaultMaterials();

// Returns just the names of all default materials (for matching against bundled .dwmat filenames).
std::vector<std::string> getDefaultMaterialNames();

} // namespace dw
