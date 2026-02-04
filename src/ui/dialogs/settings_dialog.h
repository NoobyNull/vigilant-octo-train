#pragma once

#include "../../core/config/config.h"
#include "dialog.h"

namespace dw {

// Application settings dialog
class SettingsDialog : public Dialog {
public:
    explicit SettingsDialog(Config* config);
    ~SettingsDialog() override = default;

    void render() override;

private:
    void renderGeneralTab();
    void renderAppearanceTab();
    void renderPathsTab();
    void renderAboutTab();

    Config* m_config;

    // Temporary values for editing
    int m_selectedTheme = 0;
    float m_uiScale = 1.0f;
    bool m_showGrid = true;
    bool m_showAxis = true;
};

}  // namespace dw
