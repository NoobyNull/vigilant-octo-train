---
phase: 16-tool-recommendation
verified: 2026-02-28T12:15:00Z
status: gaps_found
score: 4/6 must-haves verified
re_verification: false
gaps:
  - truth: "System automatically queries the tool database (DC-09)"
    status: failed
    reason: "ToolRecommender uses addCandidate() API — there is no code path that calls ToolDatabase.findAllGeometries() and feeds results into ToolRecommender. The wiring from database to recommender does not exist in any panel, wizard, or application file. The engine is built but not connected to the real database."
    artifacts:
      - path: "src/core/carve/tool_recommender.h"
        issue: "addCandidate() API is correct for testability but no caller exists outside tests"
    missing:
      - "A code path that queries ToolDatabase.findAllGeometries(), iterates results, and calls ToolRecommender.addCandidate() for each tool — whether in a Direct Carve wizard, a panel, or a helper function in the carve subsystem"
      - "Update REQUIREMENTS-DIRECT-CARVE.md to check DC-09, DC-10, DC-11 (currently still [ ])"
  - truth: "REQUIREMENTS-DIRECT-CARVE.md reflects completed status for DC-09, DC-10, DC-11"
    status: failed
    reason: "The requirements file still marks DC-09, DC-10, DC-11 as [ ] (Planned) and 'Planned' in the traceability table, despite SUMMARY claiming requirements-completed: [DC-09, DC-10, DC-11]. DC-12 and DC-13 are correctly marked [x] Complete."
    artifacts:
      - path: ".planning/REQUIREMENTS-DIRECT-CARVE.md"
        issue: "Lines 26-28 and 70-72 show DC-09/10/11 as unchecked/Planned"
    missing:
      - "Mark DC-09, DC-10, DC-11 as [x] and update traceability table to Complete"
human_verification:
  - test: "Widget card rendering"
    expected: "Tool cards show colored type badge, specs, feeds/speeds row, and reasoning text in correct layout"
    why_human: "ImGui rendering cannot be verified without running the app"
  - test: "Selection state"
    expected: "Clicking a finishing or clearing card updates selection and returns true from render()"
    why_human: "UI interaction requires live app"
---

# Phase 16: Tool Recommendation Verification Report

**Phase Goal:** System automatically recommends appropriate tools from the database based on model geometry analysis, with clear reasoning for each suggestion
**Verified:** 2026-02-28T12:15:00Z
**Status:** gaps_found
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Scoring engine exists with V-bit, ball nose, TBN, clearing logic | VERIFIED | `tool_recommender.cpp` 269 lines, all 4 scoring functions implemented |
| 2 | System recommends based on model geometry analysis (curvature, islands) | VERIFIED | `recommend()` accepts `RecommendationInput{curvature, islands}`, depth/radius gates applied |
| 3 | System queries the actual tool database (DC-09) | FAILED | No caller feeds `ToolDatabase.findAllGeometries()` into `ToolRecommender.addCandidate()` |
| 4 | Clear reasoning string attached to each candidate | VERIFIED | `buildReasoning()` produces human-readable strings; `ReasoningStrings` test confirms non-empty |
| 5 | UI widget exists showing tool details, feeds/speeds, reasoning | VERIFIED | `tool_recommendation_widget.cpp` 225 lines, cards render badge/specs/feeds/reasoning |
| 6 | Requirements file updated to reflect completion | FAILED | DC-09, DC-10, DC-11 remain `[ ]` Planned in `REQUIREMENTS-DIRECT-CARVE.md` |

**Score:** 4/6 truths verified

---

## Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `src/core/carve/tool_recommender.h` | ToolRecommender class, scoring declarations | VERIFIED | 92 lines, ToolRole enum, 4 static scorers, addCandidate/clearCandidates/recommend API |
| `src/core/carve/tool_recommender.cpp` | Scoring implementation | VERIFIED | 269 lines, all scoring functions substantive (no stubs) |
| `src/ui/widgets/tool_recommendation_widget.h` | Widget class with selection state | VERIFIED | 41 lines, setRecommendation/render/selectedFinishing/selectedClearing |
| `src/ui/widgets/tool_recommendation_widget.cpp` | Card rendering with feeds/speeds/reasoning | VERIFIED | 225 lines, substantive card layout with type badge, specs, feeds, reasoning rows |
| `tests/test_tool_recommender.cpp` | 17 tests covering all scoring paths | VERIFIED | 353 lines, 17 tests — all pass (confirmed via ctest) |

---

## Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| `tool_recommendation_widget.h` | `tool_recommender.h` | `#include "core/carve/tool_recommender.h"` | WIRED | Line 7, direct include |
| `tool_recommender.cpp` | `src/CMakeLists.txt` | source listing | WIRED | Line 120: `core/carve/tool_recommender.cpp` |
| `tool_recommendation_widget.cpp` | `src/CMakeLists.txt` | source listing | WIRED | Line 173: `ui/widgets/tool_recommendation_widget.cpp` |
| `test_tool_recommender.cpp` | `tests/CMakeLists.txt` | source listing | WIRED | Lines 73, 152 |
| `ToolDatabase.findAllGeometries()` | `ToolRecommender.addCandidate()` | integration caller | NOT_WIRED | No file in `src/` calls both. Widget is ORPHANED — not used by any panel or wizard yet (Phase 19 integration pending) |

---

## Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|------------|-------------|--------|----------|
| DC-09 | 16-01-PLAN | Automatic tool recommendation queries the tool database | BLOCKED | Engine exists but no code path feeds `ToolDatabase` results into `ToolRecommender`; requirements file still shows `[ ]` |
| DC-10 | 16-01-PLAN | V-bit recommended for taper-only finishing | VERIFIED | `scoreVBit()` with 0.8 base score; `VBitPreferredNoIslands` test passes |
| DC-11 | 16-01-PLAN | Ball nose / TBN recommended when tip radius fits min feature radius | VERIFIED | `scoreBallNose()`/`scoreTBN()` reject when `tipRadius > minConcaveRadius`; `OversizedToolRejected` test passes |
| DC-12 | 16-02-PLAN | Clearing tool only for island regions, diameter sized to geometry | VERIFIED | `scoreClearingTool()` checks `toolDiameter <= island.minClearDiameter`; `ClearingToolTooLargeRejected` test passes |
| DC-13 | 16-02-PLAN | Recommendation displays feed rate, spindle speed, stepover | VERIFIED | Widget row 3: `"F%.0f  S%d  SO%.0f%%"` from `cuttingData.feed_rate/.spindle_speed/.stepover` |

**Orphaned requirements from REQUIREMENTS-DIRECT-CARVE.md mapped to Phase 16:** None beyond DC-09 through DC-13.

---

## Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| `tool_recommendation_widget.cpp` | 49-52 | Dead code — `const auto& islands` assigned and immediately unused; body prints static string regardless | Warning | Island count/area summary promised in plan not rendered; misleading comment "We don't have direct island data here" |
| `tool_recommendation_widget.cpp` | 83-84 | Hardcoded card background colors `ImVec4(0.20f, 0.35f, 0.55f, 0.60f)` and `ImVec4(0.15f, 0.15f, 0.18f, 0.40f)` | Warning | Violates project rule "No hardcoded values in visual/GUI code" (CLAUDE.md); plan stated "No hardcoded colors — use theme constants or calculate from tool type" |
| `tool_recommendation_widget.cpp` | 205-208 | Hardcoded ABGR badge colors `0xFF55AA55`, `0xFFCC8855`, `0xFFAA55AA`, `0xFF5599DD` | Warning | Same violation — badge colors should be derived from `Theme::Colors` constants |

**Note on badge colors:** `toolTypeBadgeColor()` falls back to `Theme::Colors::Secondary` for unknown types, which shows intent to use the theme, but the 4 primary cases are hardcoded literals instead of constants.

---

## Human Verification Required

### 1. Card Layout Rendering

**Test:** Load the app, navigate to any UI path that renders `ToolRecommendationWidget::render()` with populated finishing and clearing candidates.
**Expected:** Each tool card shows: colored type badge on left, tool name/type text, specs row (diameter + angle or tip radius), feeds/speeds row with F/S/SO values, reasoning text in dim color.
**Why human:** ImGui rendering cannot be verified programmatically.

### 2. Selection State Behavior

**Test:** Click different finishing and clearing tool cards.
**Expected:** Selected card highlights with blue tint; `selectedFinishing()` and `selectedClearing()` return the clicked candidate; `render()` returns `true` on first click of a new card and `false` on re-click of already-selected.
**Why human:** UI interaction requires live app.

---

## Gaps Summary

Two gaps block full goal achievement:

**Gap 1 — Database integration missing (DC-09):** The `ToolRecommender` engine works correctly in isolation (proven by 17 passing tests), but the "automatic" part of the goal requires that the app actually feeds tools from `ToolDatabase.findAllGeometries()` into `addCandidate()` before calling `recommend()`. No such code path exists anywhere in `src/`. The `ToolRecommendationWidget` is likewise orphaned — compiled into the binary but not used by any panel or wizard. Phase 19 (Direct Carve Wizard) is the intended integration point, but that means DC-09 is not yet satisfied as stated.

**Gap 2 — Requirements file not updated:** `REQUIREMENTS-DIRECT-CARVE.md` still shows DC-09, DC-10, DC-11 as `[ ]` Planned. DC-12 and DC-13 were correctly updated to `[x]` Complete. The missing updates are a bookkeeping error rather than a functional gap, but the requirements file is the contract.

**Anti-pattern note:** Two hardcoded color patterns in the widget violate the project's explicit "no hardcoded values in visual/GUI code" rule (CLAUDE.md). These are warnings, not blockers — the widget compiles and renders — but the plan itself stated "No hardcoded colors" and the implementation ignored that constraint.

---

*Verified: 2026-02-28T12:15:00Z*
*Verifier: Claude (gsd-verifier)*
