#pragma once

#include <array>
#include <cstdint>
#include <string>

namespace dw {

// Modifier flags (bitfield)
enum ModFlags : uint8_t {
    Mod_None = 0,
    Mod_Shift = 1 << 0,
    Mod_Ctrl = 1 << 1,
    Mod_Alt = 1 << 2,
};

inline ModFlags operator|(ModFlags a, ModFlags b) {
    return static_cast<ModFlags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

// Input trigger type
enum class InputType : uint8_t {
    None = 0,
    Key,
    MouseButton,
};

// A single input binding: modifiers + trigger (key or mouse button)
struct InputBinding {
    ModFlags modifiers = Mod_None;
    InputType type = InputType::None;
    int value = 0; // ImGuiKey enum value, or mouse button index (0-4)

    bool isValid() const { return type != InputType::None; }
    bool operator==(const InputBinding& o) const;
    bool operator!=(const InputBinding& o) const { return !(*this == o); }

    // Human-readable label, e.g. "Alt+Shift+LMB"
    std::string displayName() const;

    // Serialize to/from INI string, e.g. "alt+shift+mouse0"
    std::string serialize() const;
    static InputBinding deserialize(const std::string& s);

    // Check if this binding is currently held (strict modifier match + trigger down)
    bool isHeld() const;
};

// Bindable actions
enum class BindAction : int {
    LightDirDrag = 0,
    LightIntensityDrag,
    FeedOverridePlus,
    FeedOverrideMinus,
    SpindleOverridePlus,
    SpindleOverrideMinus,
    COUNT,
};

const char* bindActionName(BindAction action);
InputBinding defaultBinding(BindAction action);

} // namespace dw
