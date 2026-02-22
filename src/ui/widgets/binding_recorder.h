#pragma once

#include <array>
#include <string>

#include "../../core/config/input_binding.h"

namespace dw {

// Widget for recording input bindings in Settings UI
struct BindingRecorder {
    bool recording = false;
    BindAction recordingAction = BindAction::LightDirDrag;
    std::string conflictMessage;

    // Render a binding row. Returns true if the binding was changed.
    bool renderBindingRow(
        BindAction action,
        InputBinding& binding,
        const std::array<InputBinding, static_cast<int>(BindAction::COUNT)>& allBindings);

  private:
    // Poll ImGui input state for a new binding during recording
    bool captureInput(InputBinding& out);

    // Check if a proposed binding conflicts with another action
    bool checkConflict(
        BindAction selfAction,
        const InputBinding& proposed,
        const std::array<InputBinding, static_cast<int>(BindAction::COUNT)>& allBindings);
};

} // namespace dw
