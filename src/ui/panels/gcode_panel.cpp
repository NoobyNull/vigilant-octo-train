#include "gcode_panel.h"

#include <imgui.h>

#include "../../core/utils/file_utils.h"
#include "../dialogs/file_dialog.h"
#include "../icons.h"
#include "../widgets/toast.h"

namespace dw {

GCodePanel::GCodePanel() : Panel("G-code Viewer") {}

void GCodePanel::render() {
    if (!m_open) {
        return;
    }

    if (ImGui::Begin(m_title.c_str(), &m_open)) {
        renderToolbar();

        if (hasGCode()) {
            ImGui::Separator();

            // Responsive layout: side-by-side when wide, stacked when narrow
            float availWidth = ImGui::GetContentRegionAvail().x;

            if (availWidth < 420.0f) {
                // Stacked layout for narrow docks
                ImGui::BeginChild("Stats", ImVec2(0, 250), true);
                renderStatistics();
                ImGui::EndChild();

                // Path view below
                ImGui::BeginChild("PathView", ImVec2(0, 0), true);
                renderPathView();
                ImGui::EndChild();
            } else {
                // Side-by-side layout for wide docks
                float statsWidth = std::min(250.0f, availWidth * 0.4f);

                // Statistics panel
                ImGui::BeginChild("Stats", ImVec2(statsWidth, 0), true);
                renderStatistics();
                ImGui::EndChild();

                ImGui::SameLine();

                // Path view
                ImGui::BeginChild("PathView", ImVec2(availWidth - statsWidth - 8, 0), true);
                renderPathView();
                ImGui::EndChild();
            }
        } else {
            ImGui::TextDisabled("No G-code loaded");
            ImGui::TextDisabled("Open a G-code file to view");
        }
    }
    ImGui::End();
}

bool GCodePanel::loadFile(const std::string& path) {
    auto content = file::readText(path);
    if (!content) {
        ToastManager::instance().show(ToastType::Error, "File Read Error",
                                     "Could not read G-code file: " + path);
        return false;
    }

    gcode::Parser parser;
    m_program = parser.parse(*content);

    if (!m_program.commands.empty()) {
        m_filePath = path;

        gcode::Analyzer analyzer;
        m_stats = analyzer.analyze(m_program);

        // Calculate layer range from bounds
        if (!m_program.path.empty()) {
            m_maxLayer = m_program.boundsMax.z;
            m_currentLayer = m_maxLayer;
        }

        return true;
    }

    ToastManager::instance().show(ToastType::Warning, "Empty G-code",
                                 "File contains no valid G-code commands");
    return false;
}

void GCodePanel::clear() {
    m_program = gcode::Program{};
    m_stats = gcode::Statistics{};
    m_filePath.clear();
    m_currentLayer = 0.0f;
    m_maxLayer = 100.0f;
    m_canvas.reset();
}

void GCodePanel::renderToolbar() {
    if (ImGui::Button("Open")) {
        if (m_fileDialog) {
            m_fileDialog->showOpen("Open G-code", FileDialog::gcodeFilters(),
                                   [this](const std::string& path) {
                                       if (!path.empty()) {
                                           loadFile(path);
                                       }
                                   });
        }
    }

    if (hasGCode()) {
        ImGui::SameLine();
        if (ImGui::Button("Close")) {
            clear();
        }

        ImGui::SameLine();
        ImGui::Separator();
        ImGui::SameLine();

        float availWidth = ImGui::GetContentRegionAvail().x;

        // Wrap checkboxes to new line at narrow widths
        if (availWidth < 350.0f) {
            // Not enough space, put checkboxes on new line
            ImGui::NewLine();
        }

        ImGui::Checkbox("Travel", &m_showTravel);
        ImGui::SameLine();
        ImGui::Checkbox("Extrusion", &m_showExtrusion);
    }
}

void GCodePanel::renderStatistics() {
    if (!hasGCode())
        return;

    ImGui::Text("%s Statistics", Icons::Info);
    ImGui::Separator();

    // File info
    if (ImGui::CollapsingHeader("File", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        std::string filename = m_filePath;
        size_t pos = filename.find_last_of("/\\");
        if (pos != std::string::npos) {
            filename = filename.substr(pos + 1);
        }
        ImGui::TextWrapped("File: %s", filename.c_str());
        ImGui::Text("Lines: %d", m_stats.lineCount);
        ImGui::Text("Commands: %d", m_stats.commandCount);
        ImGui::Unindent();
    }

    // Time estimate
    if (ImGui::CollapsingHeader("Time", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        int hours = static_cast<int>(m_stats.estimatedTime / 60);
        int mins = static_cast<int>(m_stats.estimatedTime) % 60;
        if (hours > 0) {
            ImGui::Text("Estimated: %dh %dm", hours, mins);
        } else {
            ImGui::Text("Estimated: %dm", mins);
        }
        ImGui::Unindent();
    }

    // Distance
    if (ImGui::CollapsingHeader("Distance", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        ImGui::Text("Total: %.1f mm", static_cast<double>(m_stats.totalPathLength));
        ImGui::Text("Rapid: %.1f mm", static_cast<double>(m_stats.rapidPathLength));
        ImGui::Text("Cutting: %.1f mm", static_cast<double>(m_stats.cuttingPathLength));
        ImGui::Unindent();
    }

    // Bounds
    if (ImGui::CollapsingHeader("Bounds", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        const auto& bMin = m_stats.boundsMin;
        const auto& bMax = m_stats.boundsMax;
        ImGui::Text("X: %.1f - %.1f", static_cast<double>(bMin.x), static_cast<double>(bMax.x));
        ImGui::Text("Y: %.1f - %.1f", static_cast<double>(bMin.y), static_cast<double>(bMax.y));
        ImGui::Text("Z: %.1f - %.1f", static_cast<double>(bMin.z), static_cast<double>(bMax.z));
        double sizeX = static_cast<double>(bMax.x - bMin.x);
        double sizeY = static_cast<double>(bMax.y - bMin.y);
        double sizeZ = static_cast<double>(bMax.z - bMin.z);
        ImGui::Text("Size: %.1f x %.1f x %.1f", sizeX, sizeY, sizeZ);
        ImGui::Unindent();
    }

    // Toolpath
    if (ImGui::CollapsingHeader("Toolpath", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent();
        ImGui::Text("Segments: %zu", m_program.path.size());
        ImGui::Text("Tool Changes: %d", m_stats.toolChangeCount);
        ImGui::Unindent();
    }
}

void GCodePanel::renderPathView() {
    if (!hasGCode())
        return;

    // Layer slider
    renderLayerSlider();

    ImGui::Separator();

    // Canvas for 2D path rendering
    auto area = m_canvas.begin();
    if (!area)
        return;

    // Background
    area.drawList->AddRectFilled(area.pos,
                                 ImVec2(area.pos.x + area.size.x, area.pos.y + area.size.y),
                                 IM_COL32(30, 30, 30, 255));

    // Calculate scale to fit paths
    const auto& boundsMin = m_program.boundsMin;
    const auto& boundsMax = m_program.boundsMax;
    float modelWidth = boundsMax.x - boundsMin.x;
    float modelHeight = boundsMax.y - boundsMin.y;
    if (modelWidth < 0.001f)
        modelWidth = 1.0f;
    if (modelHeight < 0.001f)
        modelHeight = 1.0f;

    float scaleX = (area.size.x - 20) / modelWidth;
    float scaleY = (area.size.y - 20) / modelHeight;
    float scale = m_canvas.effectiveScale(std::min(scaleX, scaleY));

    float offsetX =
        area.pos.x + area.size.x / 2 - (boundsMin.x + modelWidth / 2) * scale + m_canvas.panX;
    float offsetY =
        area.pos.y + area.size.y / 2 + (boundsMin.y + modelHeight / 2) * scale + m_canvas.panY;

    // Draw paths
    for (const auto& segment : m_program.path) {
        // Filter by layer
        if (segment.end.z > m_currentLayer)
            continue;

        bool isTravel = segment.isRapid;
        if (isTravel && !m_showTravel)
            continue;
        if (!isTravel && !m_showExtrusion)
            continue;

        ImVec2 p1(offsetX + segment.start.x * scale, offsetY - segment.start.y * scale);
        ImVec2 p2(offsetX + segment.end.x * scale, offsetY - segment.end.y * scale);

        ImU32 color;
        if (isTravel) {
            color = IM_COL32(100, 100, 100, 128); // Gray for travel
        } else {
            // Color by height
            float zRange = boundsMax.z - boundsMin.z + 0.001f;
            float t = (segment.end.z - boundsMin.z) / zRange;
            int r = static_cast<int>(255 * (1.0f - t));
            int b = static_cast<int>(255 * t);
            color = IM_COL32(r, 100, b, 255);
        }

        area.drawList->AddLine(p1, p2, color, isTravel ? 1.0f : 1.5f);
    }

    // Border
    area.drawList->AddRect(area.pos, ImVec2(area.pos.x + area.size.x, area.pos.y + area.size.y),
                           IM_COL32(60, 60, 60, 255));

    // Handle input for pan/zoom
    m_canvas.handleInput("gcode_canvas");
}

void GCodePanel::renderLayerSlider() {
    ImGui::Text("Layer Height:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderFloat("##Layer", &m_currentLayer, 0.0f, m_maxLayer, "%.2f mm");
}

} // namespace dw
