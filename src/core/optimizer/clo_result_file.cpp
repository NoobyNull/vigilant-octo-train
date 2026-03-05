#include "clo_result_file.h"

namespace dw {

void CloResultFile::setDirectory(const Path& dir) {
    m_directory = dir;
}

std::string CloResultFile::sanitizeFilename(const std::string& /*name*/) {
    return "stub";
}

bool CloResultFile::save(const std::string& /*name*/,
                          const optimizer::MultiStockResult& /*result*/,
                          const std::vector<optimizer::Part>& /*parts*/,
                          const std::string& /*algorithm*/,
                          bool /*allowRotation*/, f32 /*kerf*/, f32 /*margin*/,
                          const std::vector<i64>& /*stockSizeIds*/) {
    // Stub
    return false;
}

std::optional<CloResultFile::LoadResult> CloResultFile::load(const Path& /*filePath*/) {
    return std::nullopt;
}

std::vector<CloResultMeta> CloResultFile::list() {
    return {};
}

bool CloResultFile::remove(const Path& /*filePath*/) {
    return false;
}

} // namespace dw
