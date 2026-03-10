#include "settings_import_dialog.h"

#include <imgui.h>

#include "../../core/utils/log.h"
#include "../icons.h"
#include "../widgets/toast.h"

namespace dw {

SettingsImportDialog::SettingsImportDialog() : Dialog("Import Settings") {}

void SettingsImportDialog::open(const std::string& archivePath) {
    m_archivePath = archivePath;
    m_loaded = readSettingsManifest(Path(archivePath), m_manifest);
    m_open = true;

    // Pre-select all available categories
    for (int i = 0; i < static_cast<int>(SettingsCategory::Count); ++i)
        m_selected[i] = false;

    if (m_loaded) {
        for (auto cat : m_manifest.categories)
            m_selected[static_cast<int>(cat)] = true;
    }
}

void SettingsImportDialog::render() {
    if (!m_open)
        return;

    ImGui::OpenPopup(m_title.c_str());

    const auto* viewport = ImGui::GetMainViewport();
    ImVec2 center = viewport->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(
        ImVec2(viewport->WorkSize.x * 0.35f, viewport->WorkSize.y * 0.55f),
        ImGuiCond_Appearing);

    if (!ImGui::BeginPopupModal(m_title.c_str(), &m_open)) {
        return;
    }

    if (!m_loaded) {
        ImGui::TextColored(ImVec4(0.8f, 0.3f, 0.3f, 1.0f),
                           "Failed to read archive. File may be corrupt.");
        if (ImGui::Button("Close"))
            m_open = false;
        ImGui::EndPopup();
        return;
    }

    // Archive info
    ImGui::TextDisabled("Exported: %s", m_manifest.exportDate.c_str());
    if (!m_manifest.hostname.empty())
        ImGui::SameLine(), ImGui::TextDisabled("from %s", m_manifest.hostname.c_str());
    if (!m_manifest.appVersion.empty())
        ImGui::TextDisabled("App version: %s", m_manifest.appVersion.c_str());

    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Text("Select settings to restore:");
    ImGui::Spacing();

    // Select All / None buttons
    if (ImGui::SmallButton("All")) {
        for (auto cat : m_manifest.categories)
            m_selected[static_cast<int>(cat)] = true;
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("None")) {
        for (int i = 0; i < static_cast<int>(SettingsCategory::Count); ++i)
            m_selected[i] = false;
    }

    ImGui::Spacing();

    // Category checkboxes
    ImGui::BeginChild("##categories", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 2),
                      ImGuiChildFlags_Borders);

    for (auto cat : m_manifest.categories) {
        int idx = static_cast<int>(cat);
        ImGui::Checkbox(settingsCategoryName(cat), &m_selected[idx]);
        ImGui::SameLine(0, ImGui::GetStyle().ItemSpacing.x);
        ImGui::TextDisabled("(%s)", settingsCategoryDescription(cat));
    }

    ImGui::EndChild();

    ImGui::Spacing();

    // Count selected
    int selectedCount = 0;
    std::vector<SettingsCategory> toImport;
    for (auto cat : m_manifest.categories) {
        if (m_selected[static_cast<int>(cat)]) {
            toImport.push_back(cat);
            ++selectedCount;
        }
    }

    // Import / Cancel buttons
    bool canImport = selectedCount > 0;
    if (!canImport)
        ImGui::BeginDisabled();

    if (ImGui::Button("Import Selected")) {
        if (importSettings(Path(m_archivePath), toImport)) {
            ToastManager::instance().show(ToastType::Success,
                "Settings imported (" + std::to_string(selectedCount) + " categories)");
            if (m_onImported)
                m_onImported();
        } else {
            ToastManager::instance().show(ToastType::Error, "Failed to import settings");
        }
        m_open = false;
    }

    if (!canImport)
        ImGui::EndDisabled();

    ImGui::SameLine();
    if (ImGui::Button("Cancel"))
        m_open = false;

    ImGui::EndPopup();
}

} // namespace dw
