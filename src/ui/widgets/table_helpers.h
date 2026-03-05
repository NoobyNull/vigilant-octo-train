#pragma once

#include <imgui.h>

namespace dw {

/// RAII helper for 2-column label/value tables.
///
/// Usage:
///   if (auto t = LabelValueTable("##id")) {
///       t.row("Label:", "some value");
///       t.rowColored("Status:", ImVec4(0.3f, 0.8f, 0.3f, 1.0f), "OK");
///   }
///
/// The table uses ImGuiTableFlags_None with 0.35/0.65 stretch columns,
/// matching the existing label/value layout used across CNC panels.
struct LabelValueTable {
    bool open = false;

    /// Begin a 2-column label/value table.
    /// @param id  ImGui table ID (e.g. "##toolparams")
    /// @param labelWeight  label column stretch weight (default 0.35)
    /// @param valueWeight  value column stretch weight (default 0.65)
    explicit LabelValueTable(const char* id,
                             float labelWeight = 0.35f,
                             float valueWeight = 0.65f) {
        open = ImGui::BeginTable(id, 2, ImGuiTableFlags_None);
        if (open) {
            ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch, labelWeight);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch, valueWeight);
        }
    }

    ~LabelValueTable() {
        if (open)
            ImGui::EndTable();
    }

    // Non-copyable, non-movable (RAII guard)
    LabelValueTable(const LabelValueTable&) = delete;
    LabelValueTable& operator=(const LabelValueTable&) = delete;

    /// Truthy when the table was successfully opened.
    explicit operator bool() const { return open; }

    /// Render a label/value row with default text color.
    void row(const char* label, const char* value) const {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(label);
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(value);
    }

    /// Render a label/value row with a colored value.
    void rowColored(const char* label, const ImVec4& color, const char* value) const {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(label);
        ImGui::TableNextColumn();
        ImGui::TextColored(color, "%s", value);
    }

    /// Render a label/value row where the value uses printf-style formatting.
    void rowf(const char* label, const char* fmt, ...) const {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(label);
        ImGui::TableNextColumn();
        va_list args;
        va_start(args, fmt);
        ImGui::TextV(fmt, args);
        va_end(args);
    }

    /// Render a label/value row with colored, printf-style formatted value.
    void rowfColored(const char* label, const ImVec4& color, const char* fmt, ...) const {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(label);
        ImGui::TableNextColumn();
        va_list args;
        va_start(args, fmt);
        char buf[256];
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        ImGui::TextColored(color, "%s", buf);
    }

    /// Render a label/value row where the value is dimmed (TextDisabled).
    void rowDisabled(const char* label, const char* value) const {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(label);
        ImGui::TableNextColumn();
        ImGui::TextDisabled("%s", value);
    }

    /// Begin a new row, position cursor at label column.
    /// Use when the value column needs custom rendering (widgets, etc.).
    void beginRow(const char* label) const {
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        ImGui::TextUnformatted(label);
        ImGui::TableNextColumn();
    }
};

} // namespace dw
