---
phase: 15-model-analysis
verified: 2026-02-28T20:15:00Z
status: gaps_found
score: 3/5 must-haves verified
re_verification: false
gaps:
  - truth: "Island detector and analysis overlay tests execute and pass"
    status: failed
    reason: "test_island_detector.cpp and test_analysis_overlay.cpp are never added to DW_TEST_SOURCES in tests/CMakeLists.txt — test cases are not compiled into the test binary and cannot run"
    artifacts:
      - path: "tests/CMakeLists.txt"
        issue: "test_island_detector.cpp and test_analysis_overlay.cpp absent from DW_TEST_SOURCES (lines 5-82); they are written to disk but never registered"
      - path: "tests/test_island_detector.cpp"
        issue: "File exists with 6 real test cases but is unreachable — dw_tests --gtest_list_tests shows no IslandDetector tests"
      - path: "tests/test_analysis_overlay.cpp"
        issue: "File exists with 2 real test cases but is unreachable — dw_tests --gtest_list_tests shows no AnalysisOverlay tests"
    missing:
      - "Add test_island_detector.cpp to DW_TEST_SOURCES in tests/CMakeLists.txt"
      - "Add test_analysis_overlay.cpp to DW_TEST_SOURCES in tests/CMakeLists.txt"

  - truth: "island_detector.cpp and analysis_overlay.cpp are compiled into the main application binary"
    status: failed
    reason: "Both source files are missing from src/CMakeLists.txt. Plan 15-02 listed src/CMakeLists.txt as modified but SUMMARY-02 only mentions tests/CMakeLists.txt. The main digital_workshop binary cannot link IslandResult or generateAnalysisOverlay."
    artifacts:
      - path: "src/CMakeLists.txt"
        issue: "Carve section (lines 113-117) lists only heightmap.cpp, model_fitter.cpp, carve_job.cpp, surface_analysis.cpp — island_detector.cpp and analysis_overlay.cpp are absent"
    missing:
      - "Add core/carve/island_detector.cpp to src/CMakeLists.txt carve section"
      - "Add core/carve/analysis_overlay.cpp to src/CMakeLists.txt carve section"

  - truth: "DC-08: Analysis results are displayed visually on the heightmap preview"
    status: failed
    reason: "generateAnalysisOverlay() is never called from any panel, wizard, or application code. The RGBA buffer it produces is never uploaded to a texture or rendered anywhere. No heightmap preview panel exists in the application. The overlay capability is implemented at the core level but is orphaned — not wired to any display path."
    artifacts:
      - path: "src/core/carve/analysis_overlay.cpp"
        issue: "ORPHANED — exists, substantive, but no caller in src/ outside core/carve/"
    missing:
      - "Wire generateAnalysisOverlay() output to a heightmap preview (texture upload + ImGui::Image call) in a carve panel or wizard — this is the visible display that DC-08 requires"
      - "Note: Phase 18 (carve wizard) is the stated integration point; if DC-08 is deferred to Phase 18 this must be explicitly tracked"

human_verification:
  - test: "Manually verify RGBA overlay output correctness"
    expected: "Islands appear as semi-transparent colored regions, minimum curvature location shows as a bright cyan 3x3 marker, background is grayscale heightmap"
    why_human: "Visual correctness of the pixel buffer cannot be verified programmatically without running the app and displaying the texture"
---

# Phase 15: Model Analysis — Verification Report

**Phase Goal:** Heightmap is analyzed to identify minimum feature radius and island regions that require special tooling, with results displayed visually
**Verified:** 2026-02-28T20:15:00Z
**Status:** GAPS FOUND
**Re-verification:** No — initial verification

---

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
|---|-------|--------|---------|
| 1 | Minimum feature radius analysis computes smallest concave radius from heightmap | VERIFIED | `analyzeCurvature()` in surface_analysis.cpp implements discrete Laplacian with noise filtering; 5 passing tests (CurvatureAnalysis.FlatSurface, SphericalBowl, VGroove, LargeRadius, MinRadiusLocation) |
| 2 | Island detection finds enclosed depressions via V-bit burial mask | VERIFIED | `detectIslands()` in island_detector.cpp implements BFS burial propagation + flood-fill; source code is substantive (276 lines); dependency compiles correctly |
| 3 | Island regions are classified by depth, area, and minimum clearing diameter | VERIFIED | Island struct carries depth, areaMm2, minClearDiameter; classification computed in detectIslands(); `maxDistanceFromRim()` BFS helper computes clearing diameter |
| 4 | Island detector and analysis overlay tests execute and pass | FAILED | test_island_detector.cpp and test_analysis_overlay.cpp exist on disk with 8 substantive tests but are NOT in DW_TEST_SOURCES — ctest has 749 total tests, 0 IslandDetector or AnalysisOverlay tests |
| 5 | DC-08: Analysis results are displayed visually on the heightmap preview | FAILED | generateAnalysisOverlay() produces correct RGBA buffer but is never called from any UI code; no heightmap preview panel exists; island_detector.cpp and analysis_overlay.cpp also missing from src/CMakeLists.txt |

**Score:** 3/5 truths verified

---

## Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `src/core/carve/surface_analysis.h` | CurvatureResult struct, function declarations | VERIFIED | 28 lines, correct struct + 2 function signatures |
| `src/core/carve/surface_analysis.cpp` | Discrete Laplacian curvature analysis | VERIFIED | 132 lines, real implementation with noise threshold, neighbor validation |
| `src/core/carve/island_detector.h` | Island/IslandResult structs, detectIslands API | VERIFIED | 43 lines, structs match plan spec exactly |
| `src/core/carve/island_detector.cpp` | BFS burial mask, flood-fill, classification | VERIFIED (source) / ORPHANED (binary) | 276 lines, substantive implementation; missing from src/CMakeLists.txt |
| `src/core/carve/analysis_overlay.h` | generateAnalysisOverlay() declaration | VERIFIED | 23 lines, correct API signature |
| `src/core/carve/analysis_overlay.cpp` | RGBA overlay generation | VERIFIED (source) / ORPHANED (binary + UI) | 130 lines, substantive; missing from src/CMakeLists.txt; never called from UI |
| `tests/test_surface_analysis.cpp` | 5 curvature tests | VERIFIED | 5 real tests, all passing (CTest #716-720) |
| `tests/test_island_detector.cpp` | 6 island detection tests | STUB (not registered) | File exists with 6 tests but absent from DW_TEST_SOURCES — never runs |
| `tests/test_analysis_overlay.cpp` | 2 overlay tests | STUB (not registered) | File exists with 2 tests but absent from DW_TEST_SOURCES — never runs |

---

## Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| surface_analysis.cpp | tests/test_surface_analysis.cpp | DW_TEST_SOURCES + DW_TEST_DEPS | WIRED | Listed in both; 5 tests pass |
| island_detector.cpp | tests/test_island_detector.cpp | DW_TEST_SOURCES | NOT WIRED | island_detector.cpp in DW_TEST_DEPS; test file absent from DW_TEST_SOURCES |
| analysis_overlay.cpp | tests/test_analysis_overlay.cpp | DW_TEST_SOURCES | NOT WIRED | analysis_overlay.cpp in DW_TEST_DEPS; test file absent from DW_TEST_SOURCES |
| island_detector.cpp | src/CMakeLists.txt | carve source list | NOT WIRED | Missing from main app build |
| analysis_overlay.cpp | src/CMakeLists.txt | carve source list | NOT WIRED | Missing from main app build |
| generateAnalysisOverlay() | heightmap preview display | UI panel / texture upload | NOT WIRED | No caller outside core/carve/; no heightmap preview UI exists |

---

## Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|------------|-------------|--------|---------|
| DC-05 | 15-01 | Minimum feature radius analysis for tool selection guidance | SATISFIED | analyzeCurvature() implemented, tested (5 passing tests), wired to CMakeLists |
| DC-06 | 15-02 | Island detection finds enclosed depressions requiring clearing | PARTIAL | detectIslands() logic is correct and compiles; tests exist but don't run; missing from src/CMakeLists.txt |
| DC-07 | 15-02 | Island classification: depth, area, minimum clearing diameter | PARTIAL | Classification logic verified in code; tests for it exist but don't run |
| DC-08 | 15-02 | Analysis results displayed visually on heightmap preview | NOT SATISFIED | Overlay generation exists but is never called from UI; no heightmap preview panel; this is the phase goal's "displayed visually" condition |

---

## Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| `tests/CMakeLists.txt` | ~70 | test_island_detector.cpp and test_analysis_overlay.cpp not in DW_TEST_SOURCES | BLOCKER | 8 tests silently never run; CI passes with misleading test count |
| `src/CMakeLists.txt` | ~117 | island_detector.cpp and analysis_overlay.cpp absent from carve section | BLOCKER | Main app binary cannot use IslandResult or generateAnalysisOverlay |

No TODO/FIXME/PLACEHOLDER comments found in implementation files. No empty handler stubs found.

---

## Human Verification Required

### 1. Overlay Visual Correctness

**Test:** Build and run; load a model with a known pit. In the (future) carve wizard, trigger analysis and display the heightmap preview.
**Expected:** Islands appear as semi-transparent colored regions; minimum curvature location shows as a bright cyan 3x3 pixel marker; background is grayscale heightmap scaled low-to-high.
**Why human:** The RGBA buffer correctness (color blend, marker position, hue rotation) requires visual inspection. Cannot verify pixel values programmatically without a display surface.

---

## Gaps Summary

Two related gaps block full goal achievement:

**Gap 1 — Missing CMakeLists registrations (Blocker):**
Plan 15-02 said it would modify both `src/CMakeLists.txt` and `tests/CMakeLists.txt`. The SUMMARY-02 only mentions `tests/CMakeLists.txt`. In practice:
- `island_detector.cpp` and `analysis_overlay.cpp` were added to `DW_TEST_DEPS` (so tests can compile) but NOT to `DW_TEST_SOURCES` (test files never registered with ctest).
- Neither source file was added to `src/CMakeLists.txt` (main app binary cannot use them).
The fix is mechanical: two entries in `src/CMakeLists.txt` and two entries in `tests/CMakeLists.txt`.

**Gap 2 — DC-08 visual display not implemented (Goal-level gap):**
The phase goal explicitly states "with results displayed visually." The `generateAnalysisOverlay()` function produces the correct RGBA buffer, but this is never uploaded to a GPU texture or passed to any ImGui display call. No heightmap preview panel exists. The SUMMARY claims this is "ready for heightmap preview integration in carve wizard (Phase 18)" — but that means DC-08 is deferred, not completed. The requirement is not satisfied in Phase 15.

**Scope note:** DC-08 may be intentionally deferred to Phase 18 (carve wizard). If so, the REQUIREMENTS-DIRECT-CARVE.md should map DC-08 to Phase 18 rather than Phase 15, and Phase 18 must explicitly plan the display wiring.

---

_Verified: 2026-02-28T20:15:00Z_
_Verifier: Claude (gsd-verifier)_
