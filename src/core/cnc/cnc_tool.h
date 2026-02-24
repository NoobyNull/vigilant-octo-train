#pragma once

#include <string>
#include <vector>

#include "../types.h"

namespace dw {

// Vectric tool type enum (matching .vtdb tool_geometry.tool_type values)
enum class VtdbToolType : int {
    BallNose = 0,
    EndMill = 1,
    Radiused = 2,
    VBit = 3,
    // 4 unused
    TaperedBallNose = 5,
    Drill = 6,
    ThreadMill = 7,
    FormTool = 8,
    DiamondDrag = 9
};

// Units (matching .vtdb units column)
enum class VtdbUnits : int {
    Metric = 0,
    Imperial = 1
};

// Drive type for rigidity derating
enum class DriveType : int {
    Belt = 0,
    LeadScrew = 1,
    BallScrew = 2,
    RackPinion = 3
};

// Maps 1:1 to tool_geometry table in .vtdb
struct VtdbToolGeometry {
    std::string id;
    std::string name_format;
    std::string notes;
    VtdbToolType tool_type = VtdbToolType::EndMill;
    VtdbUnits units = VtdbUnits::Imperial;
    f64 diameter = 0.0;
    f64 included_angle = 0.0;
    f64 flat_diameter = 0.0;
    int num_flutes = 2;
    f64 flute_length = 0.0;
    f64 thread_pitch = 0.0;
    std::vector<u8> outline;
    f64 tip_radius = 0.0;
    int laser_watt = 0;
    std::string custom_attributes;
    f64 tooth_size = 0.0;
    f64 tooth_offset = 0.0;
    f64 neck_length = 0.0;
    f64 tooth_height = 0.0;
    f64 threaded_length = 0.0;
};

// Maps 1:1 to tool_cutting_data table in .vtdb
struct VtdbCuttingData {
    std::string id;
    int rate_units = 4;
    f64 feed_rate = 0.0;
    f64 plunge_rate = 0.0;
    int spindle_speed = 0;
    int spindle_dir = 0;
    f64 stepdown = 0.0;
    f64 stepover = 0.0;
    f64 clear_stepover = 0.0;
    f64 thread_depth = 0.0;
    f64 thread_step_in = 0.0;
    f64 laser_power = 0.0;
    int laser_passes = 0;
    f64 laser_burn_rate = 0.0;
    f64 line_width = 0.0;
    int length_units = 0;
    int tool_number = 0;
    int laser_kerf = 0;
    std::string notes;
};

// Maps 1:1 to tool_entity junction table in .vtdb
// Links tool_geometry + material + machine -> tool_cutting_data
struct VtdbToolEntity {
    std::string id;
    std::string material_id;          // Empty/null for "all materials"
    std::string machine_id;
    std::string tool_geometry_id;
    std::string tool_cutting_data_id;
};

// Maps 1:1 to tool_tree_entry table in .vtdb
struct VtdbTreeEntry {
    std::string id;
    std::string parent_group_id;      // Empty string for root entries
    int sibling_order = 0;
    std::string tool_geometry_id;     // Empty for group/folder entries
    std::string name;
    std::string notes;
    int expanded = 0;
};

// Maps 1:1 to material table in .vtdb
struct VtdbMaterial {
    std::string id;
    std::string name;
};

// Maps 1:1 to machine table in .vtdb (extended with DW-specific fields)
struct VtdbMachine {
    std::string id;
    std::string name;
    std::string make;
    std::string model;
    std::string controller_type;
    int dimensions_units = 1;
    f64 max_width = 0.0;
    f64 max_height = 0.0;
    int support_rotary = 0;
    int support_tool_change = 0;
    int has_laser_head = 0;

    // DW extensions for tool calculations
    f64 spindle_power_watts = 0.0;   // Spindle power in watts (e.g. 800W trim router)
    int max_rpm = 24000;             // Maximum spindle RPM
    DriveType drive_type = DriveType::Belt;
};

// Convenience: assembled tool view with all related data
struct VtdbToolView {
    VtdbTreeEntry tree_entry;
    VtdbToolGeometry geometry;
    VtdbCuttingData cutting_data;
    VtdbMaterial material;
    VtdbMachine machine;
};

} // namespace dw
