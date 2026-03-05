#pragma once

// Digital Workshop - Tool Recommendation Widget
// Renders scored tool recommendations as selectable ImGui cards.
// Used by the Direct Carve wizard to display finishing and clearing options.

#include "core/carve/tool_recommender.h"

namespace dw {

// Renders tool recommendation results as an ImGui widget.
// Used by the Direct Carve wizard.
class ToolRecommendationWidget {
  public:
    // Set data to display
    void setRecommendation(const carve::RecommendationResult& result);

    // Render the widget. Returns true if user selection changed.
    bool render();

    // Get user's selection (nullptr if no candidates)
    const carve::ToolCandidate* selectedFinishing() const;
    const carve::ToolCandidate* selectedClearing() const;

  private:
    carve::RecommendationResult m_result;
    int m_selectedFinishing = 0;
    int m_selectedClearing = 0;

    // Render a single tool card. Returns true if clicked.
    bool renderToolCard(const carve::ToolCandidate& tool,
                        bool selected, int index);

    // Get badge color for tool type (ABGR packed)
    static unsigned int toolTypeBadgeColor(VtdbToolType type);

    // Get tool type display name
    static const char* toolTypeLabel(VtdbToolType type);
};

} // namespace dw
