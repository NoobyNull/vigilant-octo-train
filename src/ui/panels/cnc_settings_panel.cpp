#include "ui/panels/cnc_settings_panel.h"

#include <cmath>
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

const ImVec4 kModifiedColor{1.0f, 0.8f, 0.2f, 1.0f};
const ImVec4 kErrorColor{1.0f, 0.3f, 0.3f, 1.0f};
const ImVec4 kOkColor{0.3f, 0.8f, 0.3f, 1.0f};
const ImVec4 kDimColor{0.5f, 0.5f, 0.5f, 1.0f};
const ImVec4 kAxisX{1.0f, 0.3f, 0.3f, 1.0f};
const ImVec4 kAxisY{0.3f, 1.0f, 0.3f, 1.0f};
const ImVec4 kAxisZ{0.3f, 0.5f, 1.0f, 1.0f};

// Helper: render a checkbox for a boolean setting ($4, $5, $6, $13, $20, $21, $22, $32)
// Returns true if value changed
bool renderBoolSetting(GrblSettings& settings, int id, const char* label,
                       const char* onLabel, const char* offLabel) {
    const auto* s = settings.get(id);
    if (!s) return false;

    bool val = (s->value != 0.0f);
    bool origVal = val;

    char cbLabel[64];
    std::snprintf(cbLabel, sizeof(cbLabel), "%s##bool%d", label, id);
    ImGui::Checkbox(cbLabel, &val);

    // Show on/off description
    ImGui::SameLine();
    if (val) {
        ImGui::TextColored(kOkColor, "(%s)", onLabel);
    } else {
        ImGui::TextColored(kDimColor, "(%s)", offLabel);
    }

    if (val != origVal) {
        settings.set(id, val ? 1.0f : 0.0f);
        return true;
    }
    return false;
}

// Helper: render a 3-axis bitmask editor ($2, $3, $23)
// Bits: 0=X, 1=Y, 2=Z
bool renderBitmaskAxes(GrblSettings& settings, int id, const char* label) {
    const auto* s = settings.get(id);
    if (!s) return false;

    int mask = static_cast<int>(s->value);
    int origMask = mask;
    bool bx = (mask & 1) != 0;
    bool by = (mask & 2) != 0;
    bool bz = (mask & 4) != 0;

    ImGui::Text("%s", label);
    ImGui::SameLine(200.0f);

    char lbl[32];
    std::snprintf(lbl, sizeof(lbl), "X##bm%d", id);
    ImGui::TextColored(kAxisX, "X");
    ImGui::SameLine();
    ImGui::Checkbox(lbl, &bx);

    ImGui::SameLine();
    std::snprintf(lbl, sizeof(lbl), "Y##bm%d", id);
    ImGui::TextColored(kAxisY, "Y");
    ImGui::SameLine();
    ImGui::Checkbox(lbl, &by);

    ImGui::SameLine();
    std::snprintf(lbl, sizeof(lbl), "Z##bm%d", id);
    ImGui::TextColored(kAxisZ, "Z");
    ImGui::SameLine();
    ImGui::Checkbox(lbl, &bz);

    mask = (bx ? 1 : 0) | (by ? 2 : 0) | (bz ? 4 : 0);
    if (mask != origMask) {
        settings.set(id, static_cast<float>(mask));
        return true;
    }
    return false;
}

// Helper: render a numeric input for a setting
bool renderNumericSetting(GrblSettings& settings, int id, const char* label,
                          const char* units, float width,
                          std::map<int, CncSettingsPanel::EditBuffer>& editBuffers) {
    const auto* s = settings.get(id);
    if (!s) return false;

    auto& eb = editBuffers[id];
    if (!eb.active) {
        eb.id = id;
        if (s->value == std::floor(s->value) && std::abs(s->value) < 1e6f) {
            std::snprintf(eb.buf, sizeof(eb.buf), "%d", static_cast<int>(s->value));
        } else {
            std::snprintf(eb.buf, sizeof(eb.buf), "%.3f", static_cast<double>(s->value));
        }
        eb.active = true;
    }

    ImGui::Text("%s", label);
    ImGui::SameLine(200.0f);
    ImGui::PushItemWidth(width);
    char inputLabel[32];
    std::snprintf(inputLabel, sizeof(inputLabel), "##num%d", id);
    bool changed = false;
    if (ImGui::InputText(inputLabel, eb.buf, sizeof(eb.buf),
                          ImGuiInputTextFlags_EnterReturnsTrue)) {
        try {
            float newVal = std::stof(eb.buf);
            settings.set(id, newVal);
            changed = true;
        } catch (...) {}
        eb.active = false;
    }
    ImGui::PopItemWidth();

    if (units[0] != '\0') {
        ImGui::SameLine();
        ImGui::TextDisabled("%s", units);
    }

    if (s->modified) {
        ImGui::SameLine();
        ImGui::TextColored(kModifiedColor, "(modified)");
    }

    return changed;
}

// Helper: render a per-axis numeric group ($100-102, $110-112, etc.)
void renderPerAxisGroup(GrblSettings& settings, const char* label, const char* units,
                        int idX, int idY, int idZ,
                        std::map<int, CncSettingsPanel::EditBuffer>& editBuffers) {
    ImGui::SeparatorText(label);

    const ImVec4* colors[] = {&kAxisX, &kAxisY, &kAxisZ};
    const char* axes[] = {"X", "Y", "Z"};
    int ids[] = {idX, idY, idZ};

    for (int i = 0; i < 3; ++i) {
        const auto* s = settings.get(ids[i]);
        if (!s) continue;

        auto& eb = editBuffers[ids[i]];
        if (!eb.active) {
            eb.id = ids[i];
            if (s->value == std::floor(s->value) && std::abs(s->value) < 1e6f) {
                std::snprintf(eb.buf, sizeof(eb.buf), "%d", static_cast<int>(s->value));
            } else {
                std::snprintf(eb.buf, sizeof(eb.buf), "%.3f", static_cast<double>(s->value));
            }
            eb.active = true;
        }

        ImGui::TextColored(*colors[i], "  %s", axes[i]);
        ImGui::SameLine(200.0f);
        ImGui::PushItemWidth(120.0f);
        char lbl[32];
        std::snprintf(lbl, sizeof(lbl), "##ax%d", ids[i]);
        if (ImGui::InputText(lbl, eb.buf, sizeof(eb.buf),
                              ImGuiInputTextFlags_EnterReturnsTrue)) {
            try {
                float newVal = std::stof(eb.buf);
                settings.set(ids[i], newVal);
            } catch (...) {}
            eb.active = false;
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();
        ImGui::TextDisabled("%s", units);

        if (s->modified) {
            ImGui::SameLine();
            ImGui::TextColored(kModifiedColor, "(modified)");
        }
    }
}

} // namespace

CncSettingsPanel::CncSettingsPanel() : Panel("Firmware Settings") {}

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
                requestSettings();
            }
        }
    }

    renderToolbar();
    ImGui::Spacing();

    if (ImGui::BeginTabBar("SettingsTabs")) {
        if (ImGui::BeginTabItem("Settings")) {
            m_activeTab = 0;
            renderSettingsTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Machine Tuning")) {
            m_activeTab = 1;
            renderTuningTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Raw")) {
            m_activeTab = 2;
            renderRawTab();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

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

    char btnLabel[64];
    std::snprintf(btnLabel, sizeof(btnLabel), "%s Read", Icons::Refresh);
    if (ImGui::Button(btnLabel)) {
        requestSettings();
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Query all GRBL settings ($$)");

    ImGui::SameLine();

    ImGui::BeginDisabled(!canWrite);
    std::snprintf(btnLabel, sizeof(btnLabel), "%s Write All", Icons::Save);
    if (ImGui::Button(btnLabel)) {
        writeAllModified();
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Write all modified settings to GRBL (with EEPROM-safe delays)");
    ImGui::EndDisabled();

    ImGui::SameLine();

    std::snprintf(btnLabel, sizeof(btnLabel), "%s Backup", Icons::Export);
    if (ImGui::Button(btnLabel)) {
        backupToFile();
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Export settings to JSON file");

    ImGui::SameLine();

    std::snprintf(btnLabel, sizeof(btnLabel), "%s Restore", Icons::Import);
    if (ImGui::Button(btnLabel)) {
        restoreFromFile();
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Import settings from JSON file");

    if (m_writing) {
        ImGui::SameLine();
        ImGui::TextColored(kModifiedColor, "Writing %d/%d...",
                           m_writeIndex + 1,
                           static_cast<int>(m_writeQueue.size()));
    }

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

    ImGui::BeginChild("SettingsScroll", ImVec2(0, 0), ImGuiChildFlags_None);

    // --- Signal Configuration ---
    if (ImGui::CollapsingHeader("Signal Configuration", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        ImGui::Spacing();

        ImGui::TextDisabled("Switch type: NC (normally closed) inverts the signal logic");
        ImGui::Spacing();

        renderBoolSetting(m_settings, 5, "Limit switches", "NC (Normally Closed)",
                          "NO (Normally Open)");
        renderBoolSetting(m_settings, 6, "Probe pin", "NC (Normally Closed)",
                          "NO (Normally Open)");
        renderBoolSetting(m_settings, 4, "Step enable", "Inverted", "Normal");

        ImGui::Spacing();
        renderBitmaskAxes(m_settings, 2, "Step pulse invert");
        renderBitmaskAxes(m_settings, 3, "Direction invert");

        ImGui::Spacing();
        ImGui::Unindent();
    }

    // --- Limits & Homing ---
    if (ImGui::CollapsingHeader("Limits & Homing", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        ImGui::Spacing();

        renderBoolSetting(m_settings, 20, "Soft limits", "Enabled", "Disabled");
        renderBoolSetting(m_settings, 21, "Hard limits", "Enabled", "Disabled");
        renderBoolSetting(m_settings, 22, "Homing cycle", "Enabled", "Disabled");

        ImGui::Spacing();
        renderBitmaskAxes(m_settings, 23, "Homing direction invert");

        ImGui::Spacing();
        renderNumericSetting(m_settings, 24, "Homing feed rate", "mm/min", 100.0f, m_editBuffers);
        renderNumericSetting(m_settings, 25, "Homing seek rate", "mm/min", 100.0f, m_editBuffers);
        renderNumericSetting(m_settings, 26, "Homing debounce", "ms", 80.0f, m_editBuffers);
        renderNumericSetting(m_settings, 27, "Homing pull-off", "mm", 80.0f, m_editBuffers);

        ImGui::Spacing();
        ImGui::Unindent();
    }

    // --- Spindle ---
    if (ImGui::CollapsingHeader("Spindle", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        ImGui::Spacing();

        renderNumericSetting(m_settings, 30, "Max spindle speed", "RPM", 100.0f, m_editBuffers);
        renderNumericSetting(m_settings, 31, "Min spindle speed", "RPM", 100.0f, m_editBuffers);
        renderBoolSetting(m_settings, 32, "Laser mode", "Enabled", "Disabled");

        ImGui::Spacing();
        ImGui::Unindent();
    }

    // --- Motion ---
    if (ImGui::CollapsingHeader("Motion Parameters")) {
        ImGui::Indent();
        ImGui::Spacing();

        renderNumericSetting(m_settings, 0, "Step pulse time", "us", 80.0f, m_editBuffers);
        renderNumericSetting(m_settings, 1, "Step idle delay", "ms", 80.0f, m_editBuffers);
        ImGui::SameLine();
        ImGui::TextDisabled("(255 = always on)");
        renderNumericSetting(m_settings, 11, "Junction deviation", "mm", 80.0f, m_editBuffers);
        renderNumericSetting(m_settings, 12, "Arc tolerance", "mm", 80.0f, m_editBuffers);
        renderBoolSetting(m_settings, 13, "Report in inches", "Inches", "Millimeters");

        ImGui::Spacing();
        ImGui::Unindent();
    }

    // --- Status Report ---
    if (ImGui::CollapsingHeader("Status Report")) {
        ImGui::Indent();
        ImGui::Spacing();

        const auto* s10 = m_settings.get(10);
        if (s10) {
            int mask = static_cast<int>(s10->value);
            int origMask = mask;
            bool workPos = (mask & 1) != 0;
            bool bufferData = (mask & 2) != 0;

            ImGui::Checkbox("Report work position (WPos)##sr0", &workPos);
            ImGui::SameLine();
            if (!workPos) {
                ImGui::TextColored(kDimColor, "(reports MPos instead)");
            } else {
                ImGui::TextColored(kOkColor, "(WPos active)");
            }
            ImGui::Checkbox("Report buffer state##sr1", &bufferData);

            mask = (workPos ? 1 : 0) | (bufferData ? 2 : 0);
            if (mask != origMask) {
                m_settings.set(10, static_cast<float>(mask));
            }
        }

        ImGui::Spacing();
        ImGui::Unindent();
    }

    // --- Per-axis parameters ---
    if (ImGui::CollapsingHeader("Per-Axis Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();

        renderPerAxisGroup(m_settings, "Steps per mm", "steps/mm", 100, 101, 102, m_editBuffers);
        renderPerAxisGroup(m_settings, "Max Feed Rate", "mm/min", 110, 111, 112, m_editBuffers);
        renderPerAxisGroup(m_settings, "Acceleration", "mm/s\xc2\xb2", 120, 121, 122, m_editBuffers);
        renderPerAxisGroup(m_settings, "Max Travel", "mm", 130, 131, 132, m_editBuffers);

        ImGui::Spacing();
        ImGui::Unindent();
    }

    // --- Unknown/extension settings ---
    bool hasUnknown = false;
    for (const auto& [id, setting] : m_settings.getAll()) {
        if (grblSettingGroup(id) == GrblSettingGroup::Unknown) {
            hasUnknown = true;
            break;
        }
    }

    if (hasUnknown && ImGui::CollapsingHeader("Extension Settings (grblHAL/FluidNC)")) {
        ImGui::Indent();
        ImGui::Spacing();
        ImGui::TextDisabled("Settings not in standard GRBL -- shown as raw values");
        ImGui::Spacing();

        for (const auto& [id, setting] : m_settings.getAll()) {
            if (grblSettingGroup(id) != GrblSettingGroup::Unknown)
                continue;

            char label[64];
            std::snprintf(label, sizeof(label), "$%d: %s", id, setting.description.c_str());
            renderNumericSetting(m_settings, id, label,
                                 setting.units.c_str(), 100.0f, m_editBuffers);
        }

        ImGui::Spacing();
        ImGui::Unindent();
    }

    ImGui::EndChild();
}

void CncSettingsPanel::renderTuningTab() {
    if (m_settings.empty()) {
        ImGui::TextDisabled("No settings loaded. Click 'Read' to query GRBL.");
        return;
    }

    ImGui::BeginChild("TuningScroll", ImVec2(0, 0), ImGuiChildFlags_None);

    ImGui::TextDisabled("Quick access to common machine tuning parameters");
    ImGui::Spacing();

    renderPerAxisGroup(m_settings, "Steps per mm", "steps/mm", 100, 101, 102, m_editBuffers);
    renderPerAxisGroup(m_settings, "Max Feed Rate", "mm/min", 110, 111, 112, m_editBuffers);
    renderPerAxisGroup(m_settings, "Acceleration", "mm/s\xc2\xb2", 120, 121, 122, m_editBuffers);
    renderPerAxisGroup(m_settings, "Max Travel", "mm", 130, 131, 132, m_editBuffers);

    ImGui::EndChild();
}

void CncSettingsPanel::renderRawTab() {
    if (m_settings.empty()) {
        ImGui::TextDisabled("No settings loaded. Click 'Read' to query GRBL.");
        return;
    }

    bool canWrite =
        m_connected &&
        m_machineState != MachineState::Run &&
        m_machineState != MachineState::Alarm &&
        !m_writing;

    ImGui::TextDisabled("All settings as raw $N=value pairs");
    ImGui::Spacing();

    ImGui::BeginTable("rawSettings", 5,
                      ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                          ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_ScrollY,
                      ImVec2(0, 0));
    ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 50.0f);
    ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 120.0f);
    ImGui::TableSetupColumn("Units", ImGuiTableColumnFlags_WidthFixed, 70.0f);
    ImGui::TableSetupColumn("##apply", ImGuiTableColumnFlags_WidthFixed, 60.0f);
    ImGui::TableHeadersRow();
    ImGui::TableSetupScrollFreeze(0, 1);

    for (const auto& [id, s] : m_settings.getAll()) {
        ImGui::TableNextRow();

        ImGui::TableNextColumn();
        ImGui::Text("$%d", s.id);

        ImGui::TableNextColumn();
        if (s.modified) {
            ImGui::TextColored(kModifiedColor, "%s", s.description.c_str());
        } else {
            ImGui::TextUnformatted(s.description.c_str());
        }

        ImGui::TableNextColumn();
        auto& eb = m_editBuffers[s.id];
        if (!eb.active) {
            eb.id = s.id;
            if (s.isBoolean) {
                std::snprintf(eb.buf, sizeof(eb.buf), "%d", static_cast<int>(s.value));
            } else if (s.value == std::floor(s.value) && std::abs(s.value) < 1e6f) {
                std::snprintf(eb.buf, sizeof(eb.buf), "%d", static_cast<int>(s.value));
            } else {
                std::snprintf(eb.buf, sizeof(eb.buf), "%.3f", static_cast<double>(s.value));
            }
            eb.active = true;
        }

        ImGui::PushItemWidth(-1);
        char label[32];
        std::snprintf(label, sizeof(label), "##val%d", s.id);
        if (ImGui::InputText(label, eb.buf, sizeof(eb.buf),
                              ImGuiInputTextFlags_EnterReturnsTrue)) {
            try {
                float newVal = std::stof(eb.buf);
                m_settings.set(s.id, newVal);
            } catch (...) {}
            eb.active = false;
        }
        ImGui::PopItemWidth();

        ImGui::TableNextColumn();
        ImGui::TextDisabled("%s", s.units.c_str());

        ImGui::TableNextColumn();
        ImGui::BeginDisabled(!canWrite || !s.modified);
        char applyLabel[32];
        std::snprintf(applyLabel, sizeof(applyLabel), "Apply##%d", s.id);
        if (ImGui::SmallButton(applyLabel)) {
            applySettingToGrbl(s.id, s.value);
        }
        ImGui::EndDisabled();
    }

    ImGui::EndTable();
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

        ImGui::BeginTable("diff", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
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
            ImGui::TextColored(kModifiedColor, "%.3f", static_cast<double>(backup.value));
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
        requestSettings();
    } else {
        m_collecting = false;
        m_writing = false;
        m_writeQueue.clear();
    }
}

void CncSettingsPanel::onRawLine(const std::string& line, bool isSent) {
    if (isSent) return;

    if (!line.empty() && line[0] == '$' && line.find('=') != std::string::npos) {
        m_settings.parseLine(line);
        m_editBuffers.clear();
        m_collecting = true;
    } else if (m_collecting && (line == "ok" || line.substr(0, 5) == "error")) {
        m_collecting = false;
    }
}

void CncSettingsPanel::onStatusUpdate(const MachineStatus& status) {
    m_machineState = status.state;
}

} // namespace dw
