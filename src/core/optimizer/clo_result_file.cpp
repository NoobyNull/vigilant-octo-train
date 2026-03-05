#include "clo_result_file.h"

#include <algorithm>
#include <filesystem>

#include <nlohmann/json.hpp>

#include "../utils/file_utils.h"
#include "../utils/log.h"

namespace dw {

using json = nlohmann::json;

void CloResultFile::setDirectory(const Path& dir) {
    m_directory = dir;
}

std::string CloResultFile::sanitizeFilename(const std::string& name) {
    std::string result;
    result.reserve(name.size());
    for (char c : name) {
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == ' ')
            result += c;
        else
            result += '_';
    }
    while (!result.empty() && result.front() == ' ')
        result.erase(result.begin());
    while (!result.empty() && result.back() == ' ')
        result.pop_back();
    if (result.empty())
        result = "clo_result";
    return result;
}

bool CloResultFile::save(const std::string& name,
                          const optimizer::MultiStockResult& result,
                          const std::vector<optimizer::Part>& parts,
                          const std::string& algorithm,
                          bool allowRotation, f32 kerf, f32 margin,
                          const std::vector<i64>& stockSizeIds) {
    if (m_directory.empty()) {
        log::error("CloResultFile", "No directory set");
        return false;
    }

    static_cast<void>(file::createDirectories(m_directory));

    json doc;
    doc["format_version"] = 1;
    doc["name"] = name;
    doc["algorithm"] = algorithm;
    doc["allow_rotation"] = allowRotation;
    doc["kerf"] = kerf;
    doc["margin"] = margin;

    // Parts
    auto& partsArr = doc["parts"];
    partsArr = json::array();
    for (const auto& part : parts) {
        partsArr.push_back({
            {"id", part.id},
            {"material_id", part.materialId},
            {"name", part.name},
            {"width", part.width},
            {"height", part.height},
            {"quantity", part.quantity},
            {"can_rotate", part.canRotate}
        });
    }

    // Groups
    auto& groupsArr = doc["groups"];
    groupsArr = json::array();
    for (size_t gi = 0; gi < result.groups.size(); ++gi) {
        const auto& gr = result.groups[gi];
        json groupObj;
        groupObj["material_id"] = gr.materialId;
        groupObj["material_name"] = gr.materialName;

        // Stock size ID
        if (gi < stockSizeIds.size()) {
            groupObj["stock_size_id"] = stockSizeIds[gi];
        } else {
            groupObj["stock_size_id"] = 0;
        }

        // Sheet
        groupObj["sheet"] = {
            {"width", gr.usedSheet.width},
            {"height", gr.usedSheet.height},
            {"cost", gr.usedSheet.cost},
            {"name", gr.usedSheet.name},
            {"grain_horizontal", gr.usedSheet.grainHorizontal}
        };

        // Plan
        json planObj;
        planObj["sheets_used"] = gr.plan.sheetsUsed;
        planObj["total_used_area"] = gr.plan.totalUsedArea;
        planObj["total_waste_area"] = gr.plan.totalWasteArea;
        planObj["total_cost"] = gr.plan.totalCost;

        auto& sheetsArr = planObj["sheets"];
        sheetsArr = json::array();
        for (const auto& sr : gr.plan.sheets) {
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
        auto& unplacedArr = planObj["unplaced_parts"];
        unplacedArr = json::array();
        for (const auto& part : gr.plan.unplacedParts) {
            unplacedArr.push_back({
                {"id", part.id},
                {"name", part.name},
                {"width", part.width},
                {"height", part.height},
                {"quantity", part.quantity}
            });
        }

        groupObj["plan"] = planObj;

        // Waste breakdown
        json wasteObj;
        auto& scrapArr = wasteObj["scrap_pieces"];
        scrapArr = json::array();
        for (const auto& sp : gr.waste.scrapPieces) {
            scrapArr.push_back({
                {"x", sp.x},
                {"y", sp.y},
                {"width", sp.width},
                {"height", sp.height},
                {"sheet_index", sp.sheetIndex}
            });
        }
        wasteObj["total_scrap_area"] = gr.waste.totalScrapArea;
        wasteObj["total_kerf_area"] = gr.waste.totalKerfArea;
        wasteObj["total_unusable_area"] = gr.waste.totalUnusableArea;
        wasteObj["scrap_value"] = gr.waste.scrapValue;
        wasteObj["kerf_value"] = gr.waste.kerfValue;
        wasteObj["unusable_value"] = gr.waste.unusableValue;
        wasteObj["total_waste_value"] = gr.waste.totalWasteValue;

        groupObj["waste"] = wasteObj;
        groupsArr.push_back(groupObj);
    }

    // Totals
    doc["totals"] = {
        {"total_cost", result.totalCost},
        {"total_sheets_used", result.totalSheetsUsed}
    };

    Path filePath = m_directory / (sanitizeFilename(name) + ".clo.json");
    std::string content = doc.dump(2);

    if (!file::writeText(filePath, content)) {
        log::errorf("CloResultFile", "Failed to write %s", filePath.string().c_str());
        return false;
    }

    log::infof("CloResultFile", "Saved CLO result: %s", filePath.string().c_str());
    return true;
}

std::optional<CloResultFile::LoadResult> CloResultFile::load(const Path& filePath) {
    auto textResult = file::readText(filePath);
    if (!textResult) {
        log::errorf("CloResultFile", "Failed to read %s", filePath.string().c_str());
        return std::nullopt;
    }

    json doc;
    try {
        doc = json::parse(*textResult);
    } catch (const json::parse_error& e) {
        log::errorf("CloResultFile", "JSON parse error in %s: %s",
                    filePath.string().c_str(), e.what());
        return std::nullopt;
    }

    LoadResult lr;
    lr.name = doc.value("name", file::getStem(filePath));
    lr.algorithm = doc.value("algorithm", "guillotine");
    lr.allowRotation = doc.value("allow_rotation", true);
    lr.kerf = doc.value("kerf", 3.0f);
    lr.margin = doc.value("margin", 5.0f);

    // Parts
    if (doc.contains("parts") && doc["parts"].is_array()) {
        for (const auto& p : doc["parts"]) {
            optimizer::Part part;
            part.id = p.value("id", static_cast<i64>(0));
            part.materialId = p.value("material_id", static_cast<i64>(0));
            part.name = p.value("name", std::string{});
            part.width = p.value("width", 0.0f);
            part.height = p.value("height", 0.0f);
            part.quantity = p.value("quantity", 1);
            part.canRotate = p.value("can_rotate", true);
            lr.parts.push_back(part);
        }
    }

    // Groups
    if (doc.contains("groups") && doc["groups"].is_array()) {
        for (const auto& g : doc["groups"]) {
            optimizer::MultiStockResult::GroupResult gr;
            gr.materialId = g.value("material_id", static_cast<i64>(0));
            gr.materialName = g.value("material_name", std::string{});

            // Stock size ID
            lr.stockSizeIds.push_back(g.value("stock_size_id", static_cast<i64>(0)));

            // Sheet
            if (g.contains("sheet")) {
                auto& s = g["sheet"];
                gr.usedSheet.width = s.value("width", 0.0f);
                gr.usedSheet.height = s.value("height", 0.0f);
                gr.usedSheet.cost = s.value("cost", 0.0f);
                gr.usedSheet.name = s.value("name", std::string{});
                gr.usedSheet.grainHorizontal = s.value("grain_horizontal", true);
            }

            // Plan
            if (g.contains("plan")) {
                auto& r = g["plan"];
                gr.plan.sheetsUsed = r.value("sheets_used", 0);
                gr.plan.totalUsedArea = r.value("total_used_area", 0.0f);
                gr.plan.totalWasteArea = r.value("total_waste_area", 0.0f);
                gr.plan.totalCost = r.value("total_cost", 0.0f);

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
                        gr.plan.sheets.push_back(sheetResult);
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
                        gr.plan.unplacedParts.push_back(part);
                    }
                }
            }

            // Waste
            if (g.contains("waste")) {
                auto& w = g["waste"];
                if (w.contains("scrap_pieces") && w["scrap_pieces"].is_array()) {
                    for (const auto& sp : w["scrap_pieces"]) {
                        optimizer::ScrapPiece piece;
                        piece.x = sp.value("x", 0.0f);
                        piece.y = sp.value("y", 0.0f);
                        piece.width = sp.value("width", 0.0f);
                        piece.height = sp.value("height", 0.0f);
                        piece.sheetIndex = sp.value("sheet_index", 0);
                        gr.waste.scrapPieces.push_back(piece);
                    }
                }
                gr.waste.totalScrapArea = w.value("total_scrap_area", 0.0f);
                gr.waste.totalKerfArea = w.value("total_kerf_area", 0.0f);
                gr.waste.totalUnusableArea = w.value("total_unusable_area", 0.0f);
                gr.waste.scrapValue = w.value("scrap_value", 0.0f);
                gr.waste.kerfValue = w.value("kerf_value", 0.0f);
                gr.waste.unusableValue = w.value("unusable_value", 0.0f);
                gr.waste.totalWasteValue = w.value("total_waste_value", 0.0f);
            }

            lr.result.groups.push_back(gr);
        }
    }

    // Totals
    if (doc.contains("totals")) {
        auto& t = doc["totals"];
        lr.result.totalCost = t.value("total_cost", 0.0f);
        lr.result.totalSheetsUsed = t.value("total_sheets_used", 0);
    }

    // Re-link placement part pointers to loaded parts
    for (auto& gr : lr.result.groups) {
        for (auto& sr : gr.plan.sheets) {
            for (auto& pl : sr.placements) {
                if (pl.partIndex >= 0 && pl.partIndex < static_cast<int>(lr.parts.size()))
                    pl.part = &lr.parts[static_cast<size_t>(pl.partIndex)];
            }
        }
    }

    return lr;
}

std::vector<CloResultMeta> CloResultFile::list() {
    std::vector<CloResultMeta> results;

    if (m_directory.empty() || !file::isDirectory(m_directory))
        return results;

    // Scan for .clo.json files
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(m_directory, ec)) {
        if (!entry.is_regular_file())
            continue;
        auto path = entry.path();
        // Check for .clo.json extension
        std::string filename = path.filename().string();
        if (filename.size() < 9 || filename.substr(filename.size() - 9) != ".clo.json")
            continue;

        CloResultMeta meta;
        meta.filePath = path;
        meta.name = filename.substr(0, filename.size() - 9);

        // Quick-parse metadata
        auto textResult = file::readText(path);
        if (textResult) {
            try {
                auto doc = json::parse(*textResult);
                meta.name = doc.value("name", meta.name);
                if (doc.contains("totals")) {
                    auto& t = doc["totals"];
                    meta.totalSheetsUsed = t.value("total_sheets_used", 0);
                    meta.totalCost = t.value("total_cost", 0.0f);
                }
                if (doc.contains("groups") && doc["groups"].is_array()) {
                    meta.groupCount = static_cast<int>(doc["groups"].size());
                }
            } catch (...) {
                // Use defaults
            }
        }

        results.push_back(meta);
    }

    // Sort by name
    std::sort(results.begin(), results.end(),
              [](const CloResultMeta& a, const CloResultMeta& b) {
                  return a.name < b.name;
              });

    return results;
}

bool CloResultFile::remove(const Path& filePath) {
    return file::remove(filePath);
}

} // namespace dw
