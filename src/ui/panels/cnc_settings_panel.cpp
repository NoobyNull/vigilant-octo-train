#include "ui/panels/cnc_settings_panel.h"

#include <cstdio>
#include <cstring>
#include <fstream>

#include <imgui.h>

#include "core/cnc/cnc_controller.h"
#include "ui/dialogs/file_dialog.h"
#include "ui/icons.h"
#include "ui/theme.h"

namespace dw {

namespace {

// Color for modified values
const ImVec4 kModifiedColor{1.0f, 0.8f, 0.2f, 1.0f};
const ImVec4 kErrorColor{1.0f, 0.3f, 0.3f, 1.0f};
const ImVec4 kOkColor{0.3f, 0.8f, 0.3f, 1.0f};

} // namespace

CncSettingsPanel::CncSettingsPanel() : Panel("CNC Settings") {}

void CncSettingsPanel::render() {
    if (!m_open)
        return;

    if (!ImGui::Begin(m_title.c_str(), &m_open)) {
        ImGui::End();
        return;
    }

    if (!m_connected) {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s Disconnected",
                           Icons::Unlink);
        ImGui::TextDisabled("Connect a CNC machine to view firmware settings");
        ImGui::End();
        return;
    }

    // Process write queue
    if (m_writing && m_writeIndex < static_cast<int>(m_writeQueue.size())) {
        m_writeTimer += ImGui::GetIO().DeltaTime;
        if (m_writeTimer >= WRITE_DELAY_SEC) {
            m_writeTimer = 0.0f;
            auto& item = m_writeQueue[static_cast<size_t>(m_writeIndex)];
            applySettingToGrbl(item.id, item.value);
            ++m_writeIndex;
            if (m_writeIndex >= static_cast<int>(m_writeQueue.size())) {
                m_writing = false;
                m_writeQueue.clear();
                m_writeIndex = 0;
                // Re-read settings to confirm
                requestSettings();
            }
        }
    }

    renderToolbar();
    ImGui::Spacing();

    // Tab bar
    if (ImGui::BeginTabBar("SettingsTabs")) {
        if (ImGui::BeginTabItem("All Settings")) {
            m_activeTab = 0;
            renderSettingsTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Machine Tuning")) {
            m_activeTab = 1;
            renderTuningTab();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    // Diff dialog overlay
    if (m_showDiffDialog) {
        renderDiffDialog();
    }

    ImGui::End();
}

void CncSettingsPanel::renderToolbar() {
    bool canWrite =
        m_connected &&
        m_machineState != MachineState::Run &&
        m_machineState != MachineState::Alarm &&
        !m_writing;

    // Read Settings button
    char btnLabel[64];
    std::snprintf(btnLabel, sizeof(btnLabel), "%s Read", Icons::Refresh);
    if (ImGui::Button(btnLabel)) {
        requestSettings();
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Query all GRBL settings ($$)");

    ImGui::SameLine();

    // Write All Modified
    ImGui::BeginDisabled(!canWrite);
    std::snprintf(btnLabel, sizeof(btnLabel), "%s Write All", Icons::Save);
    if (ImGui::Button(btnLabel)) {
        writeAllModified();
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Write all modified settings to GRBL");
    ImGui::EndDisabled();

    ImGui::SameLine();

    // Backup
    std::snprintf(btnLabel, sizeof(btnLabel), "%s Backup", Icons::Export);
    if (ImGui::Button(btnLabel)) {
        backupToFile();
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Export settings to JSON file");

    ImGui::SameLine();

    // Restore
    std::snprintf(btnLabel, sizeof(btnLabel), "%s Restore", Icons::Import);
    if (ImGui::Button(btnLabel)) {
        restoreFromFile();
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Import settings from JSON file");

    // Write progress indicator
    if (m_writing) {
        ImGui::SameLine();
        ImGui::TextColored(kModifiedColor, "Writing %d/%d...",
                           m_writeIndex + 1,
                           static_cast<int>(m_writeQueue.size()));
    }

    // State guard warning
    if (m_machineState == MachineState::Run) {
        ImGui::SameLine();
        ImGui::TextColored(kErrorColor, "Cannot write during streaming");
    } else if (m_machineState == MachineState::Alarm) {
        ImGui::SameLine();
        ImGui::TextColored(kErrorColor, "Alarm active -- unlock first ($X)");
    }
}

void CncSettingsPanel::renderSettingsTab() {
    if (m_settings.empty()) {
        ImGui::TextDisabled("No settings loaded. Click 'Read' to query GRBL.");
        return;
    }

    bool canWrite =
        m_connected &&
        m_machineState != MachineState::Run &&
        m_machineState != MachineState::Alarm &&
        !m_writing;

    auto grouped = m_settings.getGrouped();
    for (const auto& [group, settings] : grouped) {
        if (!ImGui::CollapsingHeader(grblSettingGroupName(group),
                                      ImGuiTreeNodeFlags_DefaultOpen)) {
            continue;
        }

        ImGui::BeginTable("settings", 5,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                              ImGuiTableFlags_SizingStretchProp);
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 50.0f);
        ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("Units", ImGuiTableColumnFlags_WidthFixed, 70.0f);
        ImGui::TableSetupColumn("##apply", ImGuiTableColumnFlags_WidthFixed, 60.0f);
        ImGui::TableHeadersRow();

        for (const auto* s : settings) {
            ImGui::TableNextRow();

            // ID
            ImGui::TableNextColumn();
            ImGui::Text("$%d", s->id);

            // Description
            ImGui::TableNextColumn();
            if (s->modified) {
                ImGui::TextColored(kModifiedColor, "%s", s->description.c_str());
            } else {
                ImGui::TextUnformatted(s->description.c_str());
            }

            // Value (editable input)
            ImGui::TableNextColumn();
            auto& eb = m_editBuffers[s->id];
            if (!eb.active) {
                eb.id = s->id;
                if (s->isBoolean) {
                    std::snprintf(eb.buf, sizeof(eb.buf), "%d",
                                  static_cast<int>(s->value));
                } else if (s->value == std::floor(s->value) &&
                           std::abs(s->value) < 1e6f) {
                    std::snprintf(eb.buf, sizeof(eb.buf), "%d",
                                  static_cast<int>(s->value));
                } else {
                    std::snprintf(eb.buf, sizeof(eb.buf), "%.3f",
                                  static_cast<double>(s->value));
                }
                eb.active = true;
            }

            ImGui::PushItemWidth(-1);
            char label[32];
            std::snprintf(label, sizeof(label), "##val%d", s->id);
            if (ImGui::InputText(label, eb.buf, sizeof(eb.buf),
                                  ImGuiInputTextFlags_EnterReturnsTrue)) {
                // Apply on Enter
                float newVal = 0.0f;
                try {
                    newVal = std::stof(eb.buf);
                    m_settings.set(s->id, newVal); // May fail validation
                } catch (...) {
                    // Invalid input — will reset to current value
                }
                eb.active = false; // Always refresh display from model
            }
            ImGui::PopItemWidth();

            // Units
            ImGui::TableNextColumn();
            ImGui::TextDisabled("%s", s->units.c_str());

            // Apply button
            ImGui::TableNextColumn();
            ImGui::BeginDisabled(!canWrite || !s->modified);
            char applyLabel[32];
            std::snprintf(applyLabel, sizeof(applyLabel), "Apply##%d", s->id);
            if (ImGui::SmallButton(applyLabel)) {
                applySettingToGrbl(s->id, s->value);
            }
            ImGui::EndDisabled();
        }

        ImGui::EndTable();
    }
}

void CncSettingsPanel::renderTuningTab() {
    if (m_settings.empty()) {
        ImGui::TextDisabled("No settings loaded. Click 'Read' to query GRBL.");
        return;
    }

    bool canWrite =
        m_connected &&
        m_machineState != MachineState::Run &&
        m_machineState != MachineState::Alarm &&
        !m_writing;

    static const char* axisLabels[] = {"X", "Y", "Z"};
    static const ImVec4 axisColors[] = {
        {1.0f, 0.3f, 0.3f, 1.0f}, // X red
        {0.3f, 1.0f, 0.3f, 1.0f}, // Y green
        {0.3f, 0.5f, 1.0f, 1.0f}, // Z blue
    };

    // Per-axis tuning: steps/mm, max feed, acceleration
    struct TuningRow {
        const char* label;
        int ids[3]; // X, Y, Z setting IDs
        const char* units;
    };

    static const TuningRow rows[] = {
        {"Steps/mm", {100, 101, 102}, "steps/mm"},
        {"Max Feed Rate", {110, 111, 112}, "mm/min"},
        {"Acceleration", {120, 121, 122}, "mm/s^2"},
        {"Max Travel", {130, 131, 132}, "mm"},
    };

    ImGui::SeparatorText("Per-Axis Machine Parameters");

    for (const auto& row : rows) {
        ImGui::Spacing();
        ImGui::Text("%s (%s)", row.label, row.units);
        ImGui::Indent();

        for (int axis = 0; axis < 3; ++axis) {
            int id = row.ids[axis];
            const auto* s = m_settings.get(id);
            if (!s) continue;

            ImGui::TextColored(axisColors[axis], "%s:", axisLabels[axis]);
            ImGui::SameLine();

            auto& eb = m_editBuffers[id];
            if (!eb.active) {
                eb.id = id;
                if (s->value == std::floor(s->value) && std::abs(s->value) < 1e6f) {
                    std::snprintf(eb.buf, sizeof(eb.buf), "%d",
                                  static_cast<int>(s->value));
                } else {
                    std::snprintf(eb.buf, sizeof(eb.buf), "%.3f",
                                  static_cast<double>(s->value));
                }
                eb.active = true;
            }

            ImGui::PushItemWidth(120.0f);
            char label[32];
            std::snprintf(label, sizeof(label), "##tune%d", id);
            if (ImGui::InputText(label, eb.buf, sizeof(eb.buf),
                                  ImGuiInputTextFlags_EnterReturnsTrue)) {
                float newVal = 0.0f;
                try {
                    newVal = std::stof(eb.buf);
                    m_settings.set(id, newVal); // May fail validation
                } catch (...) {
                    // Invalid input — will reset to current value
                }
                eb.active = false; // Always refresh display from model
            }
            ImGui::PopItemWidth();

            if (s->modified) {
                ImGui::SameLine();
                ImGui::TextColored(kModifiedColor, "(modified)");

                ImGui::SameLine();
                ImGui::BeginDisabled(!canWrite);
                char applyLabel[32];
                std::snprintf(applyLabel, sizeof(applyLabel), "Apply##t%d", id);
                if (ImGui::SmallButton(applyLabel)) {
                    applySettingToGrbl(id, s->value);
                }
                ImGui::EndDisabled();
            }
        }

        ImGui::Unindent();
    }
}

void CncSettingsPanel::renderDiffDialog() {
    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Restore Settings Preview", &m_showDiffDialog)) {
        ImGui::End();
        return;
    }

    if (m_diffEntries.empty()) {
        ImGui::Text("No differences found -- backup matches current settings.");
    } else {
        ImGui::Text("%d setting(s) differ:", static_cast<int>(m_diffEntries.size()));
        ImGui::Spacing();

        ImGui::BeginTable("diff", 4,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
        ImGui::TableSetupColumn("Setting");
        ImGui::TableSetupColumn("Current");
        ImGui::TableSetupColumn("Backup");
        ImGui::TableSetupColumn("Description");
        ImGui::TableHeadersRow();

        for (const auto& [current, backup] : m_diffEntries) {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("$%d", backup.id);
            ImGui::TableNextColumn();
            ImGui::Text("%.3f", static_cast<double>(current.value));
            ImGui::TableNextColumn();
            ImGui::TextColored(kModifiedColor, "%.3f",
                               static_cast<double>(backup.value));
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(backup.description.c_str());
        }
        ImGui::EndTable();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (!m_diffEntries.empty()) {
        if (ImGui::Button("Apply All Changes")) {
            // Queue all differing settings for sequential write
            m_writeQueue.clear();
            for (const auto& [current, backup] : m_diffEntries) {
                m_writeQueue.push_back({backup.id, backup.value});
                m_settings.set(backup.id, backup.value);
            }
            m_writeIndex = 0;
            m_writeTimer = 0.0f;
            m_writing = true;
            m_showDiffDialog = false;
        }
        ImGui::SameLine();
    }
    if (ImGui::Button("Cancel")) {
        m_showDiffDialog = false;
    }

    ImGui::End();
}

// --- Actions ---

void CncSettingsPanel::requestSettings() {
    if (!m_cnc || !m_connected) return;
    m_collecting = true;
    m_settings.clear();
    m_editBuffers.clear();
    m_cnc->sendCommand("$$");
}

void CncSettingsPanel::applySettingToGrbl(int id, float value) {
    if (!m_cnc || !m_connected) return;
    std::string cmd = GrblSettings::buildSetCommand(id, value);
    // Remove trailing newline for sendCommand (controller adds it)
    if (!cmd.empty() && cmd.back() == '\n') cmd.pop_back();
    m_cnc->sendCommand(cmd);
}

void CncSettingsPanel::writeAllModified() {
    m_writeQueue.clear();
    for (const auto& [id, setting] : m_settings.getAll()) {
        if (setting.modified) {
            m_writeQueue.push_back({id, setting.value});
        }
    }
    if (m_writeQueue.empty()) return;
    m_writeIndex = 0;
    m_writeTimer = 0.0f;
    m_writing = true;
}

void CncSettingsPanel::backupToFile() {
    if (!m_fileDialog || m_settings.empty()) return;
    std::string json = m_settings.toJsonString();
    m_fileDialog->showSave(
        "Backup GRBL Settings",
        {{.name = "JSON Files", .extensions = "*.json"}},
        "grbl_settings_backup.json",
        [json](const std::string& path) {
            std::ofstream ofs(path);
            if (ofs.is_open()) {
                ofs << json;
            }
        });
}

void CncSettingsPanel::restoreFromFile() {
    if (!m_fileDialog) return;
    m_fileDialog->showOpen(
        "Restore GRBL Settings",
        {{.name = "JSON Files", .extensions = "*.json"}},
        [this](const std::string& path) {
            std::ifstream ifs(path);
            if (!ifs.is_open()) return;
            std::string content((std::istreambuf_iterator<char>(ifs)),
                                 std::istreambuf_iterator<char>());
            GrblSettings backup;
            if (!backup.fromJsonString(content)) return;

            m_restoreSettings = backup;
            m_diffEntries = m_settings.diff(backup);
            m_showDiffDialog = true;
        });
}

// --- Callbacks ---

void CncSettingsPanel::onConnectionChanged(bool connected,
                                            const std::string& /*version*/) {
    m_connected = connected;
    if (connected) {
        // Auto-query settings on connect
        requestSettings();
    } else {
        m_collecting = false;
        m_writing = false;
        m_writeQueue.clear();
    }
}

void CncSettingsPanel::onRawLine(const std::string& line, bool isSent) {
    if (isSent) return;

    // Capture $N=V lines from $$ response
    if (!line.empty() && line[0] == '$' && line.find('=') != std::string::npos) {
        m_settings.parseLine(line);
        m_editBuffers.clear(); // Reset edit buffers to show new values
        m_collecting = true;
    } else if (m_collecting && (line == "ok" || line.substr(0, 5) == "error")) {
        // End of $$ response
        m_collecting = false;
    }
}

void CncSettingsPanel::onStatusUpdate(const MachineStatus& status) {
    m_machineState = status.state;
}

} // namespace dw
