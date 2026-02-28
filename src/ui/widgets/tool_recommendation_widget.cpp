// Digital Workshop - Tool Recommendation Widget Implementation

#include "ui/widgets/tool_recommendation_widget.h"

#include "ui/theme.h"

#include <imgui.h>

#include <cstdio>

namespace dw {

void ToolRecommendationWidget::setRecommendation(
    const carve::RecommendationResult& result) {
    m_result = result;
    m_selectedFinishing = 0;
    m_selectedClearing = 0;
}

bool ToolRecommendationWidget::render() {
    bool changed = false;

    // --- Finishing Tool Section ---
    ImGui::SeparatorText("Finishing Tool");

    if (m_result.finishing.empty()) {
        ImGui::TextDisabled("No suitable finishing tools found");
    } else {
        for (int i = 0; i < static_cast<int>(m_result.finishing.size()); ++i) {
            ImGui::PushID(i);
            if (renderToolCard(m_result.finishing[static_cast<size_t>(i)],
                               i == m_selectedFinishing, i)) {
                if (m_selectedFinishing != i) {
                    m_selectedFinishing = i;
                    changed = true;
                }
            }
            ImGui::PopID();
        }
    }

    // --- Clearing Tool Section (only when islands detected) ---
    if (m_result.needsClearing) {
        ImGui::Spacing();
        ImGui::SeparatorText("Clearing Tool (for island regions)");

        // Island summary
        if (!m_result.clearing.empty()) {
            const auto& islands = m_result.finishing.empty()
                ? std::vector<carve::ToolCandidate>()
                : std::vector<carve::ToolCandidate>();

            // We don't have direct island data here, show clearing options
            ImGui::TextDisabled("Select a clearing tool for buried regions");
        }

        if (m_result.clearing.empty()) {
            ImGui::TextDisabled("No suitable clearing tools found");
        } else {
            for (int i = 0; i < static_cast<int>(m_result.clearing.size()); ++i) {
                ImGui::PushID(1000 + i); // Offset to avoid ID collision
                if (renderToolCard(m_result.clearing[static_cast<size_t>(i)],
                                   i == m_selectedClearing, i)) {
                    if (m_selectedClearing != i) {
                        m_selectedClearing = i;
                        changed = true;
                    }
                }
                ImGui::PopID();
            }
        }
    }

    return changed;
}

bool ToolRecommendationWidget::renderToolCard(
    const carve::ToolCandidate& tool, bool selected, int /*index*/) {
    bool clicked = false;

    // Card background with selection highlight
    const ImVec4 bgColor = selected
        ? ImVec4(0.20f, 0.35f, 0.55f, 0.60f) // Selected highlight
        : ImVec4(0.15f, 0.15f, 0.18f, 0.40f); // Default subtle bg

    ImGui::PushStyleColor(ImGuiCol_ChildBg, bgColor);
    const float cardWidth = ImGui::GetContentRegionAvail().x;

    ImGui::BeginGroup();

    // Selectable overlay for click detection
    const ImVec2 cursorStart = ImGui::GetCursorPos();
    if (ImGui::Selectable("##card", selected,
                          ImGuiSelectableFlags_AllowOverlap,
                          ImVec2(cardWidth, 0.0f))) {
        clicked = true;
    }
    const ImVec2 cursorEnd = ImGui::GetCursorPos();
    ImGui::SetCursorPos(cursorStart);

    ImGui::BeginGroup();

    // Row 1: Tool type badge + name
    {
        const unsigned int badgeCol = toolTypeBadgeColor(tool.geometry.tool_type);
        ImGui::PushStyleColor(ImGuiCol_Button,
            ImGui::ColorConvertU32ToFloat4(badgeCol));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
            ImGui::ColorConvertU32ToFloat4(badgeCol));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,
            ImGui::ColorConvertU32ToFloat4(badgeCol));
        ImGui::SmallButton(toolTypeLabel(tool.geometry.tool_type));
        ImGui::PopStyleColor(3);

        ImGui::SameLine();
        if (!tool.geometry.name_format.empty()) {
            ImGui::TextUnformatted(tool.geometry.name_format.c_str());
        } else {
            ImGui::Text("%s %.1fmm",
                        toolTypeLabel(tool.geometry.tool_type),
                        tool.geometry.diameter);
        }
    }

    // Row 2: Key specs in columns
    {
        char specBuf[128];
        if (tool.geometry.tool_type == VtdbToolType::VBit) {
            std::snprintf(specBuf, sizeof(specBuf),
                "Dia: %.2fmm  Angle: %.0f deg",
                tool.geometry.diameter,
                tool.geometry.included_angle);
        } else if (tool.geometry.tool_type == VtdbToolType::BallNose ||
                   tool.geometry.tool_type == VtdbToolType::TaperedBallNose) {
            std::snprintf(specBuf, sizeof(specBuf),
                "Dia: %.2fmm  Tip R: %.2fmm",
                tool.geometry.diameter,
                tool.geometry.tip_radius);
        } else {
            std::snprintf(specBuf, sizeof(specBuf),
                "Dia: %.2fmm  Flutes: %d",
                tool.geometry.diameter,
                tool.geometry.num_flutes);
        }
        ImGui::TextUnformatted(specBuf);
    }

    // Row 3: Feeds/speeds
    {
        char feedBuf[128];
        std::snprintf(feedBuf, sizeof(feedBuf),
            "F%.0f  S%d  SO%.0f%%",
            tool.cuttingData.feed_rate,
            tool.cuttingData.spindle_speed,
            tool.cuttingData.stepover * 100.0);
        ImGui::TextColored(
            ImGui::ColorConvertU32ToFloat4(Theme::Colors::TextDim),
            "%s", feedBuf);
    }

    // Row 4: Reasoning (italic-style dim text)
    if (!tool.reasoning.empty()) {
        ImGui::TextColored(
            ImGui::ColorConvertU32ToFloat4(Theme::Colors::TextDim),
            "%s", tool.reasoning.c_str());
    }

    ImGui::EndGroup();

    // Restore cursor past the selectable
    ImGui::SetCursorPos(cursorEnd);
    ImGui::EndGroup();
    ImGui::PopStyleColor(); // ChildBg

    ImGui::Spacing();

    return clicked;
}

const carve::ToolCandidate* ToolRecommendationWidget::selectedFinishing() const {
    if (m_result.finishing.empty()) {
        return nullptr;
    }
    const auto idx = static_cast<size_t>(m_selectedFinishing);
    if (idx >= m_result.finishing.size()) {
        return nullptr;
    }
    return &m_result.finishing[idx];
}

const carve::ToolCandidate* ToolRecommendationWidget::selectedClearing() const {
    if (m_result.clearing.empty()) {
        return nullptr;
    }
    const auto idx = static_cast<size_t>(m_selectedClearing);
    if (idx >= m_result.clearing.size()) {
        return nullptr;
    }
    return &m_result.clearing[idx];
}

unsigned int ToolRecommendationWidget::toolTypeBadgeColor(VtdbToolType type) {
    // ABGR packed colors matching tool categories
    switch (type) {
    case VtdbToolType::VBit:            return 0xFF55AA55; // Green
    case VtdbToolType::BallNose:        return 0xFFCC8855; // Blue
    case VtdbToolType::TaperedBallNose: return 0xFFAA55AA; // Purple
    case VtdbToolType::EndMill:         return 0xFF5599DD; // Orange
    default:                            return Theme::Colors::Secondary;
    }
}

const char* ToolRecommendationWidget::toolTypeLabel(VtdbToolType type) {
    switch (type) {
    case VtdbToolType::VBit:            return "V-Bit";
    case VtdbToolType::BallNose:        return "Ball Nose";
    case VtdbToolType::TaperedBallNose: return "TBN";
    case VtdbToolType::EndMill:         return "End Mill";
    case VtdbToolType::Radiused:        return "Radiused";
    case VtdbToolType::Drill:           return "Drill";
    default:                            return "Tool";
    }
}

} // namespace dw
