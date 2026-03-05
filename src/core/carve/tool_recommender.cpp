// Digital Workshop - Tool Recommender Implementation

#include "tool_recommender.h"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace dw {
namespace carve {

void ToolRecommender::addCandidate(const VtdbToolGeometry& geom,
                                   const VtdbCuttingData& data) {
    m_candidates.push_back({geom, data});
}

void ToolRecommender::clearCandidates() {
    m_candidates.clear();
}

RecommendationResult ToolRecommender::recommend(
    const RecommendationInput& input) const {
    RecommendationResult result;
    result.needsClearing = !input.islands.islands.empty();

    // Score all candidates for finishing
    for (const auto& c : m_candidates) {
        f32 score = 0.0f;
        switch (c.geometry.tool_type) {
        case VtdbToolType::VBit:
            score = scoreVBit(c.geometry, input);
            break;
        case VtdbToolType::BallNose:
            score = scoreBallNose(c.geometry, input);
            break;
        case VtdbToolType::TaperedBallNose:
            score = scoreTBN(c.geometry, input);
            break;
        default:
            break;
        }
        if (score > 0.0f) {
            ToolCandidate tc;
            tc.geometry = c.geometry;
            tc.cuttingData = c.cuttingData;
            tc.role = ToolRole::Finishing;
            tc.score = score;
            tc.reasoning = buildReasoning(
                c.geometry, score, ToolRole::Finishing, input);
            result.finishing.push_back(tc);
        }
    }

    // Score clearing candidates when islands detected
    if (result.needsClearing) {
        for (const auto& c : m_candidates) {
            const f32 score = scoreClearingTool(c.geometry, input);
            if (score > 0.0f) {
                ToolCandidate tc;
                tc.geometry = c.geometry;
                tc.cuttingData = c.cuttingData;
                tc.role = ToolRole::Clearing;
                tc.score = score;
                tc.reasoning = buildReasoning(
                    c.geometry, score, ToolRole::Clearing, input);
                result.clearing.push_back(tc);
            }
        }
    }

    // Sort both lists by score descending
    auto byScore = [](const ToolCandidate& a, const ToolCandidate& b) {
        return a.score > b.score;
    };
    std::sort(result.finishing.begin(), result.finishing.end(), byScore);
    std::sort(result.clearing.begin(), result.clearing.end(), byScore);

    // Trim to max results
    if (static_cast<int>(result.finishing.size()) > kMaxResults) {
        result.finishing.resize(static_cast<size_t>(kMaxResults));
    }
    if (static_cast<int>(result.clearing.size()) > kMaxResults) {
        result.clearing.resize(static_cast<size_t>(kMaxResults));
    }

    return result;
}

f32 ToolRecommender::scoreVBit(const VtdbToolGeometry& geom,
                                const RecommendationInput& input) {
    // Flute length must reach full model depth
    if (geom.flute_length > 0.0 &&
        geom.flute_length < static_cast<f64>(input.modelDepthMm)) {
        return 0.0f;
    }

    // V-bits are primary finishing tool for carving
    f32 score = 0.8f;

    // Prefer included angles that match the carving detail
    const f32 angle = static_cast<f32>(geom.included_angle);
    if (angle <= 30.0f) {
        score += 0.15f; // Fine detail
    } else if (angle <= 60.0f) {
        score += 0.10f; // Good balance
    } else if (angle <= 90.0f) {
        score += 0.05f; // Coarser
    }

    return std::min(score, 1.0f);
}

f32 ToolRecommender::scoreBallNose(const VtdbToolGeometry& geom,
                                    const RecommendationInput& input) {
    if (geom.flute_length > 0.0 &&
        geom.flute_length < static_cast<f64>(input.modelDepthMm)) {
        return 0.0f;
    }

    const f32 tipRadius = static_cast<f32>(geom.tip_radius);
    const f32 minRadius = input.curvature.minConcaveRadius;

    // Tip too large for finest features
    if (minRadius > 0.0f && tipRadius > minRadius) {
        return 0.0f;
    }

    f32 score = 0.6f;

    // Prefer largest tip that fits (faster material removal)
    if (minRadius > 0.0f && tipRadius > 0.0f) {
        const f32 ratio = tipRadius / minRadius;
        score += ratio * 0.2f;
    }

    return std::min(score, 1.0f);
}

f32 ToolRecommender::scoreTBN(const VtdbToolGeometry& geom,
                               const RecommendationInput& input) {
    if (geom.flute_length > 0.0 &&
        geom.flute_length < static_cast<f64>(input.modelDepthMm)) {
        return 0.0f;
    }

    const f32 tipRadius = static_cast<f32>(geom.tip_radius);
    const f32 minRadius = input.curvature.minConcaveRadius;

    if (minRadius > 0.0f && tipRadius > minRadius) {
        return 0.0f;
    }

    // TBN preferred over plain ball nose (more rigid taper)
    f32 score = 0.7f;

    if (minRadius > 0.0f && tipRadius > 0.0f) {
        const f32 ratio = tipRadius / minRadius;
        score += ratio * 0.2f;
    }

    return std::min(score, 1.0f);
}

f32 ToolRecommender::scoreClearingTool(const VtdbToolGeometry& geom,
                                        const RecommendationInput& input) {
    // Clearing prefers flat end mills, then ball nose
    if (geom.tool_type != VtdbToolType::EndMill &&
        geom.tool_type != VtdbToolType::BallNose) {
        return 0.0f;
    }

    const f32 toolDiameter = static_cast<f32>(geom.diameter);
    if (toolDiameter <= 0.0f) {
        return 0.0f;
    }

    // Check flute length vs deepest island
    f32 maxIslandDepth = 0.0f;
    for (const auto& island : input.islands.islands) {
        maxIslandDepth = std::max(maxIslandDepth, island.depth);
    }
    if (geom.flute_length > 0.0 &&
        static_cast<f32>(geom.flute_length) < maxIslandDepth) {
        return 0.0f;
    }

    // Count how many islands this tool can clear (diameter fits)
    int clearable = 0;
    for (const auto& island : input.islands.islands) {
        if (toolDiameter <= island.minClearDiameter) {
            ++clearable;
        }
    }

    if (clearable == 0) {
        return 0.0f;
    }

    // Coverage fraction: what % of islands this tool can clear
    const f32 coverage = static_cast<f32>(clearable) /
                         static_cast<f32>(input.islands.islands.size());

    f32 score = coverage * 0.7f;

    // Flat end mills preferred for clearing (flat bottom = faster)
    if (geom.tool_type == VtdbToolType::EndMill) {
        score += 0.2f;
    }

    // Prefer largest tool that fits (faster clearing)
    f32 minClearDiam = 1e6f;
    for (const auto& island : input.islands.islands) {
        if (toolDiameter <= island.minClearDiameter) {
            minClearDiam = std::min(minClearDiam, island.minClearDiameter);
        }
    }
    if (minClearDiam > 0.0f && minClearDiam < 1e6f) {
        const f32 sizeRatio = toolDiameter / minClearDiam;
        score += sizeRatio * 0.1f;
    }

    return std::min(score, 1.0f);
}

std::string ToolRecommender::buildReasoning(
    const VtdbToolGeometry& geom, f32 /*score*/, ToolRole role,
    const RecommendationInput& input) {
    std::ostringstream oss;

    if (role == ToolRole::Finishing) {
        if (geom.tool_type == VtdbToolType::VBit) {
            oss << "V-bit " << geom.included_angle << " deg";
            oss << " -- primary carving tool, sharp tip reaches fine detail";
        } else if (geom.tool_type == VtdbToolType::TaperedBallNose) {
            oss << "Tapered ball nose R" << geom.tip_radius << "mm";
            oss << " -- rigid taper with rounded tip for smooth surfaces";
        } else {
            oss << "Ball nose R" << geom.tip_radius << "mm";
            oss << " -- smooth curves, good for organic shapes";
        }
        if (input.curvature.minConcaveRadius > 0.0f) {
            oss << ". Min feature radius: "
                << input.curvature.minConcaveRadius << "mm";
        }
    } else {
        // Clearing
        if (geom.tool_type == VtdbToolType::EndMill) {
            oss << "Flat end mill " << geom.diameter << "mm";
            oss << " -- fast island clearing with flat bottom";
        } else {
            oss << "Ball nose " << geom.diameter << "mm";
            oss << " -- island clearing with rounded profile";
        }
        int clearable = 0;
        for (const auto& island : input.islands.islands) {
            if (static_cast<f32>(geom.diameter) <= island.minClearDiameter) {
                ++clearable;
            }
        }
        oss << ". Clears " << clearable << "/"
            << input.islands.islands.size() << " islands";
    }

    return oss.str();
}

} // namespace carve
} // namespace dw
