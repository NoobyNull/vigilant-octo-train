#pragma once

#include <optional>
#include <string>
#include <vector>

#include "../types.h"
#include "multi_stock_optimizer.h"

namespace dw {

// Metadata for a saved CLO result
struct CloResultMeta {
    std::string name;
    Path filePath;
    int totalSheetsUsed = 0;
    f32 totalCost = 0.0f;
    int groupCount = 0;
};

// Persists MultiStockResult to project folder JSON with stock_size_id references
class CloResultFile {
  public:
    void setDirectory(const Path& dir);
    Path directory() const { return m_directory; }

    // Save a CLO result with a name
    bool save(const std::string& name,
              const optimizer::MultiStockResult& result,
              const std::vector<optimizer::Part>& parts,
              const std::string& algorithm,
              bool allowRotation, f32 kerf, f32 margin,
              const std::vector<i64>& stockSizeIds);

    struct LoadResult {
        optimizer::MultiStockResult result;
        std::vector<optimizer::Part> parts;
        std::string algorithm;
        bool allowRotation = true;
        f32 kerf = 3.0f;
        f32 margin = 5.0f;
        std::string name;
        std::vector<i64> stockSizeIds;
    };
    std::optional<LoadResult> load(const Path& filePath);

    std::vector<CloResultMeta> list();
    bool remove(const Path& filePath);

  private:
    static std::string sanitizeFilename(const std::string& name);
    Path m_directory;
};

} // namespace dw
