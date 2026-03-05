// Digital Workshop - Tool Recommender Tests

#include <gtest/gtest.h>

#include "core/carve/tool_recommender.h"

namespace {

using namespace dw;
using namespace dw::carve;

// Helper: create a V-bit geometry
VtdbToolGeometry makeVBit(f64 includedAngle, f64 fluteLength = 20.0) {
    VtdbToolGeometry g;
    g.id = "vbit-" + std::to_string(static_cast<int>(includedAngle));
    g.tool_type = VtdbToolType::VBit;
    g.units = VtdbUnits::Metric;
    g.included_angle = includedAngle;
    g.flute_length = fluteLength;
    g.diameter = 6.35;
    return g;
}

// Helper: create a ball nose geometry
VtdbToolGeometry makeBallNose(f64 diameter, f64 tipRadius,
                               f64 fluteLength = 20.0) {
    VtdbToolGeometry g;
    g.id = "bn-" + std::to_string(static_cast<int>(diameter));
    g.tool_type = VtdbToolType::BallNose;
    g.units = VtdbUnits::Metric;
    g.diameter = diameter;
    g.tip_radius = tipRadius;
    g.flute_length = fluteLength;
    return g;
}

// Helper: create a tapered ball nose geometry
VtdbToolGeometry makeTBN(f64 diameter, f64 tipRadius,
                          f64 includedAngle = 15.0,
                          f64 fluteLength = 20.0) {
    VtdbToolGeometry g;
    g.id = "tbn-" + std::to_string(static_cast<int>(diameter));
    g.tool_type = VtdbToolType::TaperedBallNose;
    g.units = VtdbUnits::Metric;
    g.diameter = diameter;
    g.tip_radius = tipRadius;
    g.included_angle = includedAngle;
    g.flute_length = fluteLength;
    return g;
}

// Helper: create an end mill geometry
VtdbToolGeometry makeEndMill(f64 diameter, f64 fluteLength = 20.0) {
    VtdbToolGeometry g;
    g.id = "em-" + std::to_string(static_cast<int>(diameter));
    g.tool_type = VtdbToolType::EndMill;
    g.units = VtdbUnits::Metric;
    g.diameter = diameter;
    g.flute_length = fluteLength;
    return g;
}

// Helper: default cutting data
VtdbCuttingData defaultCuttingData() {
    VtdbCuttingData cd;
    cd.id = "cd-default";
    cd.feed_rate = 1000.0;
    cd.plunge_rate = 500.0;
    cd.spindle_speed = 18000;
    return cd;
}

// Helper: build RecommendationInput with curvature only (no islands)
RecommendationInput makeInputNoIslands(f32 minConcaveRadius,
                                        f32 modelDepth = 5.0f) {
    RecommendationInput input;
    input.curvature.minConcaveRadius = minConcaveRadius;
    input.curvature.avgConcaveRadius = minConcaveRadius * 1.5f;
    input.curvature.concavePointCount = 100;
    input.modelDepthMm = modelDepth;
    input.stockThicknessMm = modelDepth + 2.0f;
    return input;
}

// Helper: build RecommendationInput with islands
RecommendationInput makeInputWithIslands(f32 minConcaveRadius,
                                          f32 modelDepth = 5.0f) {
    RecommendationInput input = makeInputNoIslands(minConcaveRadius, modelDepth);

    Island island;
    island.id = 1;
    island.depth = 3.0f;
    island.areaMm2 = 25.0f;
    island.minClearDiameter = 4.0f;
    input.islands.islands.push_back(island);

    return input;
}

// --- Test: V-bit preferred when no islands ---
TEST(ToolRecommender, VBitPreferredNoIslands) {
    ToolRecommender rec;
    rec.addCandidate(makeVBit(30.0), defaultCuttingData());
    rec.addCandidate(makeBallNose(6.0, 3.0), defaultCuttingData());
    rec.addCandidate(makeTBN(6.0, 1.5), defaultCuttingData());

    auto result = rec.recommend(makeInputNoIslands(5.0f));

    ASSERT_FALSE(result.finishing.empty());
    // V-bit should score highest
    EXPECT_EQ(result.finishing[0].geometry.tool_type, VtdbToolType::VBit);
    EXPECT_FALSE(result.needsClearing);
    EXPECT_TRUE(result.clearing.empty());
}

// --- Test: Ball nose recommended when radius fits ---
TEST(ToolRecommender, BallNoseWhenRadiusFits) {
    ToolRecommender rec;
    rec.addCandidate(makeBallNose(6.0, 1.5), defaultCuttingData());

    // Minimum feature radius = 2.1mm, tip radius = 1.5mm (fits)
    auto result = rec.recommend(makeInputNoIslands(2.1f));

    ASSERT_FALSE(result.finishing.empty());
    EXPECT_GT(result.finishing[0].score, 0.0f);
    EXPECT_EQ(result.finishing[0].geometry.tool_type, VtdbToolType::BallNose);
}

// --- Test: Oversized tool rejected ---
TEST(ToolRecommender, OversizedToolRejected) {
    ToolRecommender rec;
    // Tip radius 3.0mm vs minimum feature radius 2.1mm
    rec.addCandidate(makeBallNose(6.0, 3.0), defaultCuttingData());

    auto result = rec.recommend(makeInputNoIslands(2.1f));

    // Ball nose should be rejected (tip too large)
    EXPECT_TRUE(result.finishing.empty());
}

// --- Test: Needs clearing with islands ---
TEST(ToolRecommender, NeedsClearingWithIslands) {
    ToolRecommender rec;
    rec.addCandidate(makeVBit(30.0), defaultCuttingData());
    rec.addCandidate(makeEndMill(3.0), defaultCuttingData());

    auto result = rec.recommend(makeInputWithIslands(5.0f));

    EXPECT_TRUE(result.needsClearing);
    // End mill should appear in clearing list
    ASSERT_FALSE(result.clearing.empty());
    EXPECT_EQ(result.clearing[0].geometry.tool_type, VtdbToolType::EndMill);
    EXPECT_EQ(result.clearing[0].role, ToolRole::Clearing);
}

// --- Test: Short flute length rejects tool ---
TEST(ToolRecommender, DepthLimitRejectsShortTools) {
    ToolRecommender rec;
    // Flute length 3mm but model depth is 10mm
    rec.addCandidate(makeVBit(30.0, 3.0), defaultCuttingData());
    rec.addCandidate(makeBallNose(6.0, 1.5, 3.0), defaultCuttingData());

    auto result = rec.recommend(makeInputNoIslands(5.0f, 10.0f));

    // Both tools should be rejected
    EXPECT_TRUE(result.finishing.empty());
}

// --- Test: Reasoning strings are populated ---
TEST(ToolRecommender, ReasoningStrings) {
    ToolRecommender rec;
    rec.addCandidate(makeVBit(30.0), defaultCuttingData());
    rec.addCandidate(makeBallNose(6.0, 1.5), defaultCuttingData());

    auto result = rec.recommend(makeInputNoIslands(5.0f));

    for (const auto& tc : result.finishing) {
        EXPECT_FALSE(tc.reasoning.empty())
            << "Reasoning should not be empty for tool: "
            << tc.geometry.id;
    }
}

// --- Test: TBN scores between V-bit and ball nose ---
TEST(ToolRecommender, TBNScoresBetweenVBitAndBallNose) {
    ToolRecommender rec;
    rec.addCandidate(makeVBit(30.0), defaultCuttingData());
    rec.addCandidate(makeBallNose(6.0, 1.5), defaultCuttingData());
    rec.addCandidate(makeTBN(6.0, 1.5), defaultCuttingData());

    auto result = rec.recommend(makeInputNoIslands(5.0f));

    ASSERT_GE(result.finishing.size(), 3u);

    // Find scores by type
    f32 vbitScore = 0.0f;
    f32 bnScore = 0.0f;
    f32 tbnScore = 0.0f;
    for (const auto& tc : result.finishing) {
        switch (tc.geometry.tool_type) {
        case VtdbToolType::VBit:
            vbitScore = tc.score;
            break;
        case VtdbToolType::BallNose:
            bnScore = tc.score;
            break;
        case VtdbToolType::TaperedBallNose:
            tbnScore = tc.score;
            break;
        default:
            break;
        }
    }

    EXPECT_GT(vbitScore, tbnScore) << "V-bit should score higher than TBN";
    EXPECT_GT(tbnScore, bnScore) << "TBN should score higher than ball nose";
}

// --- Test: V-bits not used for clearing ---
TEST(ToolRecommender, VBitNotForClearing) {
    ToolRecommender rec;
    rec.addCandidate(makeVBit(30.0), defaultCuttingData());

    auto result = rec.recommend(makeInputWithIslands(5.0f));

    EXPECT_TRUE(result.needsClearing);
    // V-bit should NOT appear in clearing list
    for (const auto& tc : result.clearing) {
        EXPECT_NE(tc.geometry.tool_type, VtdbToolType::VBit);
    }
}

// --- Test: End mill too large for island pocket ---
TEST(ToolRecommender, ClearingToolTooLargeRejected) {
    ToolRecommender rec;
    // End mill diameter 6mm but island minClearDiameter is 4mm
    rec.addCandidate(makeEndMill(6.0), defaultCuttingData());

    auto result = rec.recommend(makeInputWithIslands(5.0f));

    EXPECT_TRUE(result.needsClearing);
    EXPECT_TRUE(result.clearing.empty());
}

// --- Test: Results limited to kMaxResults ---
TEST(ToolRecommender, ResultsTruncated) {
    ToolRecommender rec;
    // Add more than kMaxResults V-bits
    for (int i = 10; i <= 120; i += 10) {
        rec.addCandidate(makeVBit(static_cast<f64>(i)), defaultCuttingData());
    }

    auto result = rec.recommend(makeInputNoIslands(5.0f));

    EXPECT_LE(static_cast<int>(result.finishing.size()),
              ToolRecommender::kMaxResults);
}

// --- Test: Results sorted by score descending ---
TEST(ToolRecommender, ResultsSortedByScore) {
    ToolRecommender rec;
    rec.addCandidate(makeVBit(90.0), defaultCuttingData());
    rec.addCandidate(makeVBit(30.0), defaultCuttingData());
    rec.addCandidate(makeVBit(60.0), defaultCuttingData());

    auto result = rec.recommend(makeInputNoIslands(5.0f));

    for (size_t i = 1; i < result.finishing.size(); ++i) {
        EXPECT_GE(result.finishing[i - 1].score,
                  result.finishing[i].score)
            << "Results should be sorted descending by score";
    }
}

// --- Test: clearCandidates empties the list ---
TEST(ToolRecommender, ClearCandidates) {
    ToolRecommender rec;
    rec.addCandidate(makeVBit(30.0), defaultCuttingData());
    rec.clearCandidates();

    auto result = rec.recommend(makeInputNoIslands(5.0f));
    EXPECT_TRUE(result.finishing.empty());
    EXPECT_TRUE(result.clearing.empty());
}

// --- Test: Empty candidates produces empty result ---
TEST(ToolRecommender, EmptyCandidates) {
    ToolRecommender rec;
    auto result = rec.recommend(makeInputNoIslands(5.0f));

    EXPECT_TRUE(result.finishing.empty());
    EXPECT_TRUE(result.clearing.empty());
    EXPECT_FALSE(result.needsClearing);
}

// --- Test: Clearing prefers flat end mill over ball nose ---
TEST(ToolRecommender, ClearingPrefersFlatEndMill) {
    ToolRecommender rec;
    // Both 3mm tools fit the 4mm island passage
    rec.addCandidate(makeBallNose(3.0, 1.5), defaultCuttingData());
    rec.addCandidate(makeEndMill(3.0), defaultCuttingData());

    auto input = makeInputWithIslands(5.0f);
    auto result = rec.recommend(input);

    ASSERT_GE(result.clearing.size(), 2u);
    // End mill should rank first for clearing
    EXPECT_EQ(result.clearing[0].geometry.tool_type, VtdbToolType::EndMill);
}

// --- Test: Clearing prefers largest tool that fits ---
TEST(ToolRecommender, ClearingPrefersLargestFittingTool) {
    ToolRecommender rec;
    // 2mm and 3mm end mills both fit 4mm passage
    rec.addCandidate(makeEndMill(2.0), defaultCuttingData());
    rec.addCandidate(makeEndMill(3.0), defaultCuttingData());

    auto input = makeInputWithIslands(5.0f);
    auto result = rec.recommend(input);

    ASSERT_GE(result.clearing.size(), 2u);
    // 3mm should score higher (larger = faster clearing)
    EXPECT_GT(result.clearing[0].geometry.diameter,
              result.clearing[1].geometry.diameter);
}

// --- Test: Clearing short flute rejects for deep islands ---
TEST(ToolRecommender, ClearingRejectsShortFlute) {
    ToolRecommender rec;
    // Flute 2mm but island depth is 3mm
    rec.addCandidate(makeEndMill(3.0, 2.0), defaultCuttingData());

    auto input = makeInputWithIslands(5.0f);
    auto result = rec.recommend(input);

    EXPECT_TRUE(result.clearing.empty());
}

// --- Test: Clearing reasoning mentions island count ---
TEST(ToolRecommender, ClearingReasoningMentionsIslands) {
    ToolRecommender rec;
    rec.addCandidate(makeEndMill(3.0), defaultCuttingData());

    auto input = makeInputWithIslands(5.0f);
    auto result = rec.recommend(input);

    ASSERT_FALSE(result.clearing.empty());
    const auto& reasoning = result.clearing[0].reasoning;
    EXPECT_NE(reasoning.find("1/1"), std::string::npos)
        << "Reasoning should mention island coverage: " << reasoning;
}

} // namespace
