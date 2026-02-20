#pragma once

#include <vector>

#include "material.h"

namespace dw {

// Returns the built-in list of default materials shipped with the app.
// These are seeded into the database on first run (when materials table is empty).
// All defaults have no archivePath or thumbnailPath (.dwmat files are user-provided).
std::vector<MaterialRecord> getDefaultMaterials();

} // namespace dw
