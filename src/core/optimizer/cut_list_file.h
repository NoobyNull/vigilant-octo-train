#pragma once

#include <optional>
#include <string>
#include <vector>

#include "../types.h"
#include "sheet.h"

namespace dw {

struct CutListMeta {
    std::string name;
    Path filePath;
    int sheetsUsed = 0;
    float efficiency = 0.0f;
};

class CutListFile {
  public:
    void setDirectory(const Path& dir);
    Path directory() const { return m_directory; }

    bool save(const std::string& name,
              const optimizer::Sheet& sheet,
              const std::vector<optimizer::Part>& parts,
              const optimizer::CutPlan& result,
              const std::string& algorithm,
              bool allowRotation, float kerf, float margin);

    struct LoadResult {
        optimizer::Sheet sheet;
        std::vector<optimizer::Part> parts;
        optimizer::CutPlan result;
        std::string algorithm;
        bool allowRotation = true;
        float kerf = 3.0f;
        float margin = 5.0f;
        std::string name;
    };
    std::optional<LoadResult> load(const Path& filePath);

    std::vector<CutListMeta> list();

    bool remove(const Path& filePath);

  private:
    static std::string sanitizeFilename(const std::string& name);
    Path m_directory;
};

} // namespace dw
