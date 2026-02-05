#pragma once

#include "../../render/renderer.h"

namespace dw {

// Dialog for adjusting lighting settings
class LightingDialog {
public:
    LightingDialog() = default;

    // Open/close
    void open() { m_open = true; }
    void close() { m_open = false; }
    bool isOpen() const { return m_open; }

    // Set the render settings to modify
    void setSettings(RenderSettings* settings) { m_settings = settings; }

    // Render the dialog
    void render();

private:
    bool m_open = false;
    RenderSettings* m_settings = nullptr;
};

}  // namespace dw
