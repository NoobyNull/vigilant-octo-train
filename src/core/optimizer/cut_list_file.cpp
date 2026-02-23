#include "cut_list_file.h"

#include <algorithm>
#include <filesystem>

#include <nlohmann/json.hpp>

#include "../utils/file_utils.h"
#include "../utils/log.h"

namespace dw {

using json = nlohmann::json;

void CutListFile::setDirectory(const Path& dir) {
    m_directory = dir;
}

std::string CutListFile::sanitizeFilename(const std::string& name) {
    std::string result;
    result.reserve(name.size());
    for (char c : name) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == ' ')
            result += c;
        else
            result += '_';
    }
    // Trim leading/trailing spaces
    while (!result.empty() && result.front() == ' ')
        result.erase(result.begin());
    while (!result.empty() && result.back() == ' ')
        result.pop_back();
    if (result.empty())
        result = "cut_plan";
    return result;
}

bool CutListFile::save(const std::string& name,
                       const optimizer::Sheet& sheet,
                       const std::vector<optimizer::Part>& parts,
                       const optimizer::CutPlan& result,
                       const std::string& algorithm,
                       bool allowRotation, float kerf, float margin) {
    if (m_directory.empty()) {
        log::error("CutListFile", "No directory set");
        return false;
    }

    static_cast<void>(file::createDirectories(m_directory));

    // Build JSON document
    json doc;
    doc["format_version"] = 1;
    doc["name"] = name;
    doc["algorithm"] = algorithm;
    doc["allow_rotation"] = allowRotation;
    doc["kerf"] = kerf;
    doc["margin"] = margin;

    // Sheet
    doc["sheet"] = {
        {"width", sheet.width},
        {"height", sheet.height},
        {"cost", sheet.cost},
        {"name", sheet.name},
        {"grain_horizontal", sheet.grainHorizontal}
    };

    // Parts
    auto& partsArr = doc["parts"];
    partsArr = json::array();
    for (const auto& part : parts) {
        partsArr.push_back({
            {"id", part.id},
            {"name", part.name},
            {"width", part.width},
            {"height", part.height},
            {"quantity", part.quantity},
            {"can_rotate", part.canRotate}
        });
    }

    // Result
    json resultObj;
    resultObj["sheets_used"] = result.sheetsUsed;
    resultObj["total_used_area"] = result.totalUsedArea;
    resultObj["total_waste_area"] = result.totalWasteArea;
    resultObj["total_cost"] = result.totalCost;

    auto& sheetsArr = resultObj["sheets"];
    sheetsArr = json::array();
    for (const auto& sr : result.sheets) {
        json sheetResult;
        sheetResult["sheet_index"] = sr.sheetIndex;
        sheetResult["used_area"] = sr.usedArea;
        sheetResult["waste_area"] = sr.wasteArea;

        auto& placementsArr = sheetResult["placements"];
        placementsArr = json::array();
        for (const auto& p : sr.placements) {
            placementsArr.push_back({
                {"part_index", p.partIndex},
                {"instance_index", p.instanceIndex},
                {"x", p.x},
                {"y", p.y},
                {"rotated", p.rotated}
            });
        }
        sheetsArr.push_back(sheetResult);
    }

    // Unplaced parts
    auto& unplacedArr = resultObj["unplaced_parts"];
    unplacedArr = json::array();
    for (const auto& part : result.unplacedParts) {
        unplacedArr.push_back({
            {"id", part.id},
            {"name", part.name},
            {"width", part.width},
            {"height", part.height},
            {"quantity", part.quantity}
        });
    }

    doc["result"] = resultObj;

    Path filePath = m_directory / (sanitizeFilename(name) + ".json");
    std::string content = doc.dump(2);

    if (!file::writeText(filePath, content)) {
        log::errorf("CutListFile", "Failed to write %s", filePath.string().c_str());
        return false;
    }

    log::infof("CutListFile", "Saved cut plan: %s", filePath.string().c_str());
    return true;
}

std::optional<CutListFile::LoadResult> CutListFile::load(const Path& filePath) {
    auto textResult = file::readText(filePath);
    if (!textResult) {
        log::errorf("CutListFile", "Failed to read %s", filePath.string().c_str());
        return std::nullopt;
    }

    json doc;
    try {
        doc = json::parse(*textResult);
    } catch (const json::parse_error& e) {
        log::errorf("CutListFile", "JSON parse error in %s: %s",
                    filePath.string().c_str(), e.what());
        return std::nullopt;
    }

    LoadResult lr;
    lr.name = doc.value("name", file::getStem(filePath));
    lr.algorithm = doc.value("algorithm", "guillotine");
    lr.allowRotation = doc.value("allow_rotation", true);
    lr.kerf = doc.value("kerf", 3.0f);
    lr.margin = doc.value("margin", 5.0f);

    // Sheet
    if (doc.contains("sheet")) {
        auto& s = doc["sheet"];
        lr.sheet.width = s.value("width", 0.0f);
        lr.sheet.height = s.value("height", 0.0f);
        lr.sheet.cost = s.value("cost", 0.0f);
        lr.sheet.name = s.value("name", std::string{});
        lr.sheet.grainHorizontal = s.value("grain_horizontal", true);
    }

    // Parts
    if (doc.contains("parts") && doc["parts"].is_array()) {
        for (const auto& p : doc["parts"]) {
            optimizer::Part part;
            part.id = p.value("id", static_cast<i64>(0));
            part.name = p.value("name", std::string{});
            part.width = p.value("width", 0.0f);
            part.height = p.value("height", 0.0f);
            part.quantity = p.value("quantity", 1);
            part.canRotate = p.value("can_rotate", true);
            lr.parts.push_back(part);
        }
    }

    // Result
    if (doc.contains("result")) {
        auto& r = doc["result"];
        lr.result.sheetsUsed = r.value("sheets_used", 0);
        lr.result.totalUsedArea = r.value("total_used_area", 0.0f);
        lr.result.totalWasteArea = r.value("total_waste_area", 0.0f);
        lr.result.totalCost = r.value("total_cost", 0.0f);

        if (r.contains("sheets") && r["sheets"].is_array()) {
            for (const auto& sr : r["sheets"]) {
                optimizer::SheetResult sheetResult;
                sheetResult.sheetIndex = sr.value("sheet_index", 0);
                sheetResult.usedArea = sr.value("used_area", 0.0f);
                sheetResult.wasteArea = sr.value("waste_area", 0.0f);

                if (sr.contains("placements") && sr["placements"].is_array()) {
                    for (const auto& pl : sr["placements"]) {
                        optimizer::Placement placement;
                        placement.partIndex = pl.value("part_index", 0);
                        placement.instanceIndex = pl.value("instance_index", 0);
                        placement.x = pl.value("x", 0.0f);
                        placement.y = pl.value("y", 0.0f);
                        placement.rotated = pl.value("rotated", false);
                        sheetResult.placements.push_back(placement);
                    }
                }
                lr.result.sheets.push_back(sheetResult);
            }
        }

        // Unplaced parts
        if (r.contains("unplaced_parts") && r["unplaced_parts"].is_array()) {
            for (const auto& p : r["unplaced_parts"]) {
                optimizer::Part part;
                part.id = p.value("id", static_cast<i64>(0));
                part.name = p.value("name", std::string{});
                part.width = p.value("width", 0.0f);
                part.height = p.value("height", 0.0f);
                part.quantity = p.value("quantity", 1);
                lr.result.unplacedParts.push_back(part);
            }
        }
    }

    // Re-link placement part pointers to loaded parts
    for (auto& sr : lr.result.sheets) {
        for (auto& pl : sr.placements) {
            if (pl.partIndex >= 0 && pl.partIndex < static_cast<int>(lr.parts.size()))
                pl.part = &lr.parts[static_cast<size_t>(pl.partIndex)];
        }
    }

    return lr;
}

std::vector<CutListMeta> CutListFile::list() {
    std::vector<CutListMeta> results;

    if (m_directory.empty() || !file::isDirectory(m_directory))
        return results;

    auto files = file::listFiles(m_directory, "json");

    // Sort by modification time (newest first)
    std::sort(files.begin(), files.end(), [](const Path& a, const Path& b) {
        std::error_code ec;
        auto ta = std::filesystem::last_write_time(a, ec);
        auto tb = std::filesystem::last_write_time(b, ec);
        return ta > tb;
    });

    for (const auto& fp : files) {
        CutListMeta meta;
        meta.filePath = fp;
        meta.name = file::getStem(fp);

        // Quick-parse just the metadata fields
        auto textResult = file::readText(fp);
        if (textResult) {
            try {
                auto doc = json::parse(*textResult);
                meta.name = doc.value("name", meta.name);
                if (doc.contains("result")) {
                    auto& r = doc["result"];
                    meta.sheetsUsed = r.value("sheets_used", 0);
                    float usedArea = r.value("total_used_area", 0.0f);
                    float wasteArea = r.value("total_waste_area", 0.0f);
                    float total = usedArea + wasteArea;
                    meta.efficiency = total > 0.0f ? usedArea / total : 0.0f;
                }
            } catch (...) {
                // Use filename as fallback
            }
        }

        results.push_back(meta);
    }

    return results;
}

bool CutListFile::remove(const Path& filePath) {
    return file::remove(filePath);
}

} // namespace dw
