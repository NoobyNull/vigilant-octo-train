#pragma once

#include <functional>
#include <string>
#include <vector>

#include "../../core/config/settings_archive.h"
#include "dialog.h"

namespace dw {

// Dialog for selecting which settings categories to import
class SettingsImportDialog : public Dialog {
  public:
    SettingsImportDialog();
    ~SettingsImportDialog() override = default;

    void render() override;

    // Open with a specific archive file
    void open(const std::string& archivePath);

    // Callback after successful import
    void setOnImported(std::function<void()> cb) { m_onImported = std::move(cb); }

  private:
    std::string m_archivePath;
    SettingsArchiveManifest m_manifest;
    bool m_loaded = false;
    bool m_selected[static_cast<int>(SettingsCategory::Count)] = {};
    std::function<void()> m_onImported;
};

} // namespace dw
