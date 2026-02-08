#include "input_binding.h"

#include <algorithm>
#include <sstream>

#include <imgui.h>

namespace dw {

bool InputBinding::operator==(const InputBinding& o) const {
    return modifiers == o.modifiers && type == o.type && value == o.value;
}

std::string InputBinding::displayName() const {
    if (!isValid()) {
        return "None";
    }

    std::string result;
    if (modifiers & Mod_Ctrl)
        result += "Ctrl+";
    if (modifiers & Mod_Alt)
        result += "Alt+";
    if (modifiers & Mod_Shift)
        result += "Shift+";

    if (type == InputType::MouseButton) {
        static const char* names[] = {"LMB", "RMB", "MMB", "X1", "X2"};
        if (value >= 0 && value <= 4) {
            result += names[value];
        } else {
            result += "Mouse?";
        }
    } else if (type == InputType::Key) {
        const char* keyName = ImGui::GetKeyName(static_cast<ImGuiKey>(value));
        result += keyName ? keyName : "???";
    }
    return result;
}

std::string InputBinding::serialize() const {
    if (!isValid()) {
        return "none";
    }

    std::string result;
    if (modifiers & Mod_Ctrl)
        result += "ctrl+";
    if (modifiers & Mod_Alt)
        result += "alt+";
    if (modifiers & Mod_Shift)
        result += "shift+";

    if (type == InputType::MouseButton) {
        result += "mouse" + std::to_string(value);
    } else if (type == InputType::Key) {
        const char* keyName = ImGui::GetKeyName(static_cast<ImGuiKey>(value));
        std::string name = keyName ? keyName : "unknown";
        // Lowercase for config
        std::transform(name.begin(), name.end(), name.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        result += "key_" + name;
    }
    return result;
}

InputBinding InputBinding::deserialize(const std::string& s) {
    if (s.empty() || s == "none") {
        return {};
    }

    InputBinding binding;
    std::string remaining = s;

    // Parse modifiers
    auto consume = [&](const std::string& prefix, ModFlags flag) {
        if (remaining.size() >= prefix.size() && remaining.substr(0, prefix.size()) == prefix) {
            binding.modifiers = binding.modifiers | flag;
            remaining = remaining.substr(prefix.size());
        }
    };

    // Keep consuming modifiers (order: ctrl, alt, shift)
    consume("ctrl+", Mod_Ctrl);
    consume("alt+", Mod_Alt);
    consume("shift+", Mod_Shift);

    // Parse trigger
    if (remaining.size() >= 5 && remaining.substr(0, 5) == "mouse") {
        binding.type = InputType::MouseButton;
        binding.value = std::atoi(remaining.c_str() + 5);
        if (binding.value < 0 || binding.value > 4) {
            binding.value = 0;
        }
    } else if (remaining.size() >= 4 && remaining.substr(0, 4) == "key_") {
        binding.type = InputType::Key;
        std::string keyStr = remaining.substr(4);

        // Search ImGui named keys for a match
        for (int k = ImGuiKey_NamedKey_BEGIN; k < ImGuiKey_NamedKey_END; k++) {
            const char* name = ImGui::GetKeyName(static_cast<ImGuiKey>(k));
            if (!name)
                continue;
            std::string lower = name;
            std::transform(lower.begin(), lower.end(), lower.begin(),
                           [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (lower == keyStr) {
                binding.value = k;
                break;
            }
        }
    }

    return binding;
}

bool InputBinding::isHeld() const {
    if (!isValid()) {
        return false;
    }

    auto& io = ImGui::GetIO();

    // Strict modifier match: require exactly the specified modifiers
    bool wantShift = (modifiers & Mod_Shift) != 0;
    bool wantCtrl = (modifiers & Mod_Ctrl) != 0;
    bool wantAlt = (modifiers & Mod_Alt) != 0;

    if (io.KeyShift != wantShift)
        return false;
    if (io.KeyCtrl != wantCtrl)
        return false;
    if (io.KeyAlt != wantAlt)
        return false;

    // Check trigger
    if (type == InputType::MouseButton) {
        return io.MouseDown[value];
    }
    if (type == InputType::Key) {
        return ImGui::IsKeyDown(static_cast<ImGuiKey>(value));
    }

    return false;
}

const char* bindActionName(BindAction action) {
    switch (action) {
    case BindAction::LightDirDrag:
        return "Light Direction";
    case BindAction::LightIntensityDrag:
        return "Light Intensity";
    default:
        return "Unknown";
    }
}

InputBinding defaultBinding(BindAction action) {
    switch (action) {
    case BindAction::LightDirDrag:
        return {Mod_None, InputType::MouseButton, 3}; // Mouse X1 (thumb button)
    case BindAction::LightIntensityDrag:
        return {Mod_None, InputType::MouseButton, 4}; // Mouse X2 (thumb button)
    default:
        return {};
    }
}

} // namespace dw
