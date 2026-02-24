#pragma once

#include <string>
#include <string_view>

#include "../types.h"

namespace dw {

// CNC router bit types
enum class CncToolType {
    FlatEndMill,
    BallNose,
    VBit,
    SurfacingBit
};

inline std::string cncToolTypeToString(CncToolType type) {
    switch (type) {
    case CncToolType::FlatEndMill:
        return "flat_end_mill";
    case CncToolType::BallNose:
        return "ball_nose";
    case CncToolType::VBit:
        return "v_bit";
    case CncToolType::SurfacingBit:
        return "surfacing_bit";
    default:
        return "flat_end_mill";
    }
}

inline CncToolType stringToCncToolType(std::string_view str) {
    if (str == "flat_end_mill")
        return CncToolType::FlatEndMill;
    if (str == "ball_nose")
        return CncToolType::BallNose;
    if (str == "v_bit")
        return CncToolType::VBit;
    if (str == "surfacing_bit")
        return CncToolType::SurfacingBit;
    return CncToolType::FlatEndMill;
}

// CNC tool record (stored in cnc_tools table)
struct CncToolRecord {
    i64 id = 0;
    std::string name;
    CncToolType type = CncToolType::FlatEndMill;
    f64 diameter = 0.0;
    int fluteCount = 2;
    f64 maxRPM = 24000.0;
    f64 maxDOC = 0.0;
    f64 shankDiameter = 0.25;
    std::string notes;
    std::string createdAt;
};

// Per-tool-per-material cutting parameters (stored in tool_material_params table)
struct ToolMaterialParams {
    i64 id = 0;
    i64 toolId = 0;
    i64 materialId = 0;
    f64 feedRate = 0.0;
    f64 spindleSpeed = 0.0;
    f64 depthOfCut = 0.0;
    f64 chipLoad = 0.0;
};

} // namespace dw
