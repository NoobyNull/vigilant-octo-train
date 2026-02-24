#include "machine_profile.h"

#include <nlohmann/json.hpp>

namespace dw {
namespace gcode {

// --- JSON serialization ---

std::string MachineProfile::toJsonString() const {
    nlohmann::json j{
        {"name", name},
        {"maxFeedRateX", maxFeedRateX},
        {"maxFeedRateY", maxFeedRateY},
        {"maxFeedRateZ", maxFeedRateZ},
        {"accelX", accelX},
        {"accelY", accelY},
        {"accelZ", accelZ},
        {"maxTravelX", maxTravelX},
        {"maxTravelY", maxTravelY},
        {"maxTravelZ", maxTravelZ},
        {"junctionDeviation", junctionDeviation},
        {"rapidRate", rapidRate},
        {"defaultFeedRate", defaultFeedRate},
    };
    return j.dump();
}

MachineProfile MachineProfile::fromJsonString(const std::string& jsonStr) {
    auto j = nlohmann::json::parse(jsonStr, nullptr, false);
    if (j.is_discarded())
        return MachineProfile{};

    MachineProfile p;
    if (j.contains("name")) p.name = j["name"].get<std::string>();
    if (j.contains("maxFeedRateX")) p.maxFeedRateX = j["maxFeedRateX"].get<f32>();
    if (j.contains("maxFeedRateY")) p.maxFeedRateY = j["maxFeedRateY"].get<f32>();
    if (j.contains("maxFeedRateZ")) p.maxFeedRateZ = j["maxFeedRateZ"].get<f32>();
    if (j.contains("accelX")) p.accelX = j["accelX"].get<f32>();
    if (j.contains("accelY")) p.accelY = j["accelY"].get<f32>();
    if (j.contains("accelZ")) p.accelZ = j["accelZ"].get<f32>();
    if (j.contains("maxTravelX")) p.maxTravelX = j["maxTravelX"].get<f32>();
    if (j.contains("maxTravelY")) p.maxTravelY = j["maxTravelY"].get<f32>();
    if (j.contains("maxTravelZ")) p.maxTravelZ = j["maxTravelZ"].get<f32>();
    if (j.contains("junctionDeviation")) p.junctionDeviation = j["junctionDeviation"].get<f32>();
    if (j.contains("rapidRate")) p.rapidRate = j["rapidRate"].get<f32>();
    if (j.contains("defaultFeedRate")) p.defaultFeedRate = j["defaultFeedRate"].get<f32>();
    return p;
}

// --- Preset factories ---

MachineProfile MachineProfile::defaultProfile() {
    MachineProfile p;
    p.builtIn = true;
    return p;
}

MachineProfile MachineProfile::shapeoko4() {
    MachineProfile p;
    p.name = "Shapeoko 4";
    p.builtIn = true;
    p.maxFeedRateX = 10000.0f;
    p.maxFeedRateY = 10000.0f;
    p.maxFeedRateZ = 5000.0f;
    p.accelX = 400.0f;
    p.accelY = 400.0f;
    p.accelZ = 200.0f;
    p.maxTravelX = 838.0f;
    p.maxTravelY = 838.0f;
    p.maxTravelZ = 75.0f;
    p.rapidRate = 10000.0f;
    p.defaultFeedRate = 1000.0f;
    return p;
}

MachineProfile MachineProfile::longmillMK2() {
    MachineProfile p;
    p.name = "LongMill MK2 30x30";
    p.builtIn = true;
    p.maxFeedRateX = 4000.0f;
    p.maxFeedRateY = 4000.0f;
    p.maxFeedRateZ = 3000.0f;
    p.accelX = 200.0f;
    p.accelY = 200.0f;
    p.accelZ = 100.0f;
    p.maxTravelX = 792.0f;
    p.maxTravelY = 845.0f;
    p.maxTravelZ = 114.0f;
    p.rapidRate = 4000.0f;
    p.defaultFeedRate = 1000.0f;
    return p;
}

} // namespace gcode
} // namespace dw
