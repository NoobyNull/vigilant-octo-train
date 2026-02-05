#pragma once

#include "../../core/gcode/gcode_analyzer.h"
#include "../../core/gcode/gcode_parser.h"
#include "panel.h"

#include <memory>
#include <string>

namespace dw {

class FileDialog;

// G-code viewer panel
class GCodePanel : public Panel {
public:
    GCodePanel();
    ~GCodePanel() override = default;

    void render() override;

    // Set the file dialog used for opening G-code files
    void setFileDialog(FileDialog* dialog) { m_fileDialog = dialog; }

    // Load G-code from file
    bool loadFile(const std::string& path);

    // Clear loaded G-code
    void clear();

    // Check if G-code is loaded
    bool hasGCode() const { return m_program.commands.size() > 0; }

private:
    void renderToolbar();
    void renderStatistics();
    void renderPathView();
    void renderLayerSlider();

    FileDialog* m_fileDialog = nullptr;

    gcode::Program m_program;
    gcode::Statistics m_stats;
    std::string m_filePath;

    // View settings
    float m_currentLayer = 0.0f;
    float m_maxLayer = 100.0f;
    bool m_showTravel = false;
    bool m_showExtrusion = true;
    float m_zoom = 1.0f;
    float m_panX = 0.0f;
    float m_panY = 0.0f;
};

}  // namespace dw
