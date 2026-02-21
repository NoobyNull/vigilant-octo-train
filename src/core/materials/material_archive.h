#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "../archive/archive.h"
#include "material.h"

namespace dw {

// Raw data extracted from a .dwmat archive (before GPU upload)
struct MaterialData {
    std::vector<uint8_t> textureData; // Raw PNG bytes (not decoded pixels)
    int textureWidth = 0;             // Width hint from metadata (0 if unknown)
    int textureHeight = 0;            // Height hint from metadata (0 if unknown)
    MaterialRecord metadata;          // Parsed material properties
};

// Archive format for portable material bundles (.dwmat = ZIP)
// Internal structure:
//   material.dwmat (ZIP)
//   +-- texture.png        (tileable wood grain texture)
//   +-- metadata.json      (material properties as JSON)
//   +-- thumbnail.png      (optional, 128x128 preview)
class MaterialArchive {
  public:
    // Create a .dwmat archive from a texture file + metadata
    // texturePath must point to a PNG file
    static ArchiveResult create(const std::string& archivePath, const std::string& texturePath,
                                const MaterialRecord& record);

    // Load a .dwmat archive — returns raw PNG bytes + parsed metadata
    // Does NOT decode the PNG into pixels (use TextureLoader for that)
    static std::optional<MaterialData> load(const std::string& archivePath);

    // List all entries in a .dwmat archive
    static std::vector<ArchiveEntry> list(const std::string& archivePath);

    // Check if the file is a valid .dwmat archive (is a ZIP with metadata.json)
    static bool isValidArchive(const std::string& archivePath);

    // Archive file extension
    static constexpr const char* Extension = ".dwmat";

  private:
    // JSON serialization helpers for MaterialRecord
    static std::string metadataToJson(const MaterialRecord& record);
    static std::optional<MaterialRecord> jsonToMaterial(const std::string& json);
};

} // namespace dw
