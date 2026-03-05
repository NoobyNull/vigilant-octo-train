#pragma once

// Digital Workshop - Tool Recommender
// Scores and ranks CNC tools for Direct Carve operations.
// V-bits preferred when taper geometry allows access to all features.
// Ball nose / tapered ball nose recommended when minimum feature radius
// exceeds their tip radius.

#include "../cnc/cnc_tool.h"
#include "../types.h"
#include "island_detector.h"
#include "surface_analysis.h"

#include <string>
#include <vector>

namespace dw {
namespace carve {

enum class ToolRole {
    Finishing, // Primary carving pass
    Clearing   // Island clearing pass
};

// A scored tool candidate with reasoning
struct ToolCandidate {
    VtdbToolGeometry geometry;
    VtdbCuttingData cuttingData;
    ToolRole role = ToolRole::Finishing;
    f32 score = 0.0f;           // Higher = better match
    std::string reasoning;      // Human-readable explanation
};

// Input data for recommendation engine
struct RecommendationInput {
    CurvatureResult curvature;
    IslandResult islands;
    f32 modelDepthMm = 0.0f;    // Maximum carve depth
    f32 stockThicknessMm = 0.0f; // Stock material thickness
};

// Complete recommendation output
struct RecommendationResult {
    std::vector<ToolCandidate> finishing;  // Ranked finishing tools
    std::vector<ToolCandidate> clearing;   // Ranked clearing tools (empty if no islands)
    bool needsClearing = false;
};

// Recommend tools from a library of available geometries/cutting data.
// Usage: add candidate tool+data pairs, then call recommend() with analysis input.
class ToolRecommender {
  public:
    // Add a tool+cutting data pair as a candidate
    void addCandidate(const VtdbToolGeometry& geom,
                      const VtdbCuttingData& data);

    // Clear all candidates
    void clearCandidates();

    // Run recommendation against input analysis data
    RecommendationResult recommend(const RecommendationInput& input) const;

    // Maximum number of results per category
    static constexpr int kMaxResults = 5;

  private:
    struct CandidatePair {
        VtdbToolGeometry geometry;
        VtdbCuttingData cuttingData;
    };
    std::vector<CandidatePair> m_candidates;

    // Per-type scoring for finishing role
    static f32 scoreVBit(const VtdbToolGeometry& geom,
                         const RecommendationInput& input);
    static f32 scoreBallNose(const VtdbToolGeometry& geom,
                             const RecommendationInput& input);
    static f32 scoreTBN(const VtdbToolGeometry& geom,
                        const RecommendationInput& input);

    // Clearing tool scoring (end mills, ball nose for island pockets)
    static f32 scoreClearingTool(const VtdbToolGeometry& geom,
                                const RecommendationInput& input);

    // Build reasoning string for a scored tool
    static std::string buildReasoning(const VtdbToolGeometry& geom,
                                      f32 score, ToolRole role,
                                      const RecommendationInput& input);
};

} // namespace carve
} // namespace dw
