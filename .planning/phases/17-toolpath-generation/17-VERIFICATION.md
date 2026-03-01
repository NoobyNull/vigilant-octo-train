---
phase: 17-toolpath-generation
verified: 2026-02-28T12:30:00Z
status: gaps_found
score: 4/5 success criteria verified
re_verification: false
gaps:
  - truth: "Toolpath respects machine profile travel limits and acceleration constraints (DC-19)"
    status: partial
    reason: "Travel limit validation is implemented and tested. Acceleration constraints from machine profile are not implemented — no acceleration-aware motion planning, velocity ramping, or constraint checking against machine profile acceleration values exists anywhere in the toolpath generator."
    artifacts:
      - path: "src/core/carve/toolpath_generator.cpp"
        issue: "validateLimits() checks position bounds only (X/Y/Z travel). No acceleration constraint logic."
      - path: "src/core/carve/toolpath_types.h"
        issue: "ToolpathConfig has feedRateMmMin and plungeRateMmMin but no acceleration parameters."
    missing:
      - "Acceleration constraint checking against machine profile (max acceleration mm/s^2 per axis)"
      - "OR: a documented decision that acceleration is handled by the firmware/grbl and not by the CAM toolpath generator"
---

# Phase 17: Toolpath Generation Verification Report

**Phase Goal:** Heightmap and tool parameters are converted into an efficient machining toolpath with configurable scan strategy, stepover, and surgical clearing for island regions only
**Verified:** 2026-02-28T12:30:00Z
**Status:** gaps_found
**Re-verification:** No — initial verification

## Goal Achievement

### Observable Truths (from ROADMAP Success Criteria)

| # | Truth | Status | Evidence |
|---|-------|--------|----------|
| 1 | Raster scan generates parallel toolpath lines along user-selected axis (X, Y, X-then-Y, Y-then-X) with correct tool offset compensation | VERIFIED | `generateFinishing()` dispatches all 4 ScanAxis values via switch; `toolOffset()` applied post-scan to all cut points; tests ToolpathGen.FlatSurfaceXScan, XThenY, ToolOffset.VBitFlatSurface all pass |
| 2 | Stepover presets (Ultra Fine 1%, Fine 8%, Basic 12%, Rough 25%, Roughing 40%) produce correct line spacing relative to tool tip diameter | VERIFIED | `stepoverPercent()` maps all 5 presets exactly; `customStepoverPct` override path present; test ToolpathGen.StepoverPresets + StepoverSpacing pass |
| 3 | Milling direction (climb/conventional/alternating) controls scan line direction correctly | VERIFIED | `generateScanLines()` switches on MillDirection enum; alternating uses `lineIdx % 2`; tests AlternatingDirection and ClimbDirection pass |
| 4 | Clearing passes are generated only for identified island regions with appropriate tool, not the entire surface | VERIFIED | `generateClearing()` iterates `islands.islands`, delegates to `clearIslandRegion()` which uses island bounding box + mask membership check; empty island list returns empty path; tests ClearingPass.SingleIsland, NoIslandsNoClearingPath, MultipleIslands pass |
| 5 | Toolpath respects safe Z height, machine travel limits, and produces valid motion commands | PARTIAL | Safe Z: all retracts use `config.safeZMm` (verified by SafeZRetract test). Travel limits: `validateLimits()` checks X/Y/Z bounds with per-axis warnings (verified by TravelLimits tests). **Acceleration constraints: not implemented** — DC-19 requires "acceleration constraints" but no acceleration-aware logic exists anywhere in the toolpath generator. |

**Score:** 4/5 success criteria fully verified (SC-5 partially verified)

---

### Required Artifacts

| Artifact | Expected | Status | Details |
|----------|----------|--------|---------|
| `src/core/carve/toolpath_types.h` | ScanAxis/MillDirection/StepoverPreset enums; ToolpathConfig/ToolpathPoint/Toolpath/MultiPassToolpath structs | VERIFIED | 71 lines; all types present including `warnings` field on Toolpath, `leadInMm` on ToolpathConfig, MultiPassToolpath struct |
| `src/core/carve/toolpath_generator.h` | ToolpathGenerator class with generateFinishing, generateClearing, validateLimits, tool offset helpers, clearIslandRegion | VERIFIED | 77 lines; all public and private methods declared including VtdbToolGeometry parameter on generateFinishing |
| `src/core/carve/toolpath_generator.cpp` | Full implementation: scan lines, tool offsets (V-bit/ball-nose/end-mill), island clearing, metrics | VERIFIED | 499 lines (within 800 limit); all functions implemented with real logic, no stubs |
| `tests/test_toolpath_generator.cpp` | 20 tests covering all scan modes, stepover, direction, safe Z, time, offsets, travel limits, clearing passes | VERIFIED | 20 tests declared and all 20 pass (100%) |

---

### Key Link Verification

| From | To | Via | Status | Details |
|------|----|-----|--------|---------|
| `toolpath_generator.cpp` | `Heightmap::atMm()` | Direct call in `generateScanLines()`, `vBitOffset()`, `ballNoseOffset()`, `endMillOffset()`, `clearIslandRegion()` | WIRED | Line 250: `f32 z = heightmap.atMm(x, y)` and multiple offset helpers |
| `toolpath_generator.cpp` | `IslandResult` | `generateClearing()` iterates `islands.islands`, `clearIslandRegion()` checks `islands.islandMask` | WIRED | Lines 76, 82, 146-150: mask membership lookup by island.id |
| `toolpath_generator.cpp` | `VtdbToolGeometry` | `toolOffset()` dispatches via `tool.tool_type` enum to type-specific helpers | WIRED | Lines 325-335: switch on VtdbToolType, full dispatch chain |
| `src/CMakeLists.txt` | `toolpath_generator.cpp` | Source list entry at line 121 | WIRED | `core/carve/toolpath_generator.cpp` in build |
| `tests/CMakeLists.txt` | `test_toolpath_generator.cpp` + `toolpath_generator.cpp` | Lines 74 and 154 | WIRED | Both test file and source dependency registered |
| `ToolpathConfig.leadInMm` | `clearIslandRegion()` ramp logic | Used at lines 157, 169 of generator.cpp | WIRED | Ramp distance sourced from config, not hardcoded |

---

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
|-------------|------------|-------------|--------|----------|
| DC-14 | 17-01 | Raster scan toolpath generates parallel lines along user-selected axis | SATISFIED | generateFinishing() with ScanAxis enum; 4 axis modes tested |
| DC-15 | 17-01 | Stepover configurable as percentage with 5 presets | SATISFIED | stepoverPercent() + customStepoverPct override; all presets tested |
| DC-16 | 17-01 | Milling direction selectable: climb, conventional, alternating | SATISFIED | MillDirection enum; generateScanLines direction logic tested |
| DC-17 | 17-02 | Safe Z configurable with automatic retract between scan lines and around island boundaries | SATISFIED | addRetract() called before every scan line and between islands; safeZMm from config tested |
| DC-18 | 17-03 | Clearing passes generate only for identified island regions (surgical clearing) | SATISFIED | clearIslandRegion() uses island bounding box + mask membership; non-island cells skipped |
| DC-19 | 17-02 | Toolpath respects machine profile travel limits **and acceleration constraints** | PARTIAL | Travel limits: validateLimits() implemented and tested. Acceleration constraints: not implemented. Plan 17-02 design doc mentions only travel limits — acceleration was never designed in. |

**Orphaned requirements:** None. All DC-14 through DC-19 are claimed by plans 17-01, 17-02, 17-03.

---

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
|------|------|---------|----------|--------|
| `toolpath_generator.cpp` | 289 | `constexpr f32 kRapidRateMmMin = 5000.0f` | Info | Rapid traverse rate hardcoded to 5000 mm/min. Not in ToolpathConfig or machine profile. Non-visual/computation code so not a CLAUDE.md violation, but makes rapid time estimates inaccurate for machines with different rapid rates. |
| `toolpath_generator.cpp` | 115 | `const f32 margin = 0.2f` | Info | Island floor margin hardcoded to 0.2mm. Not configurable. No user impact yet (no UI wires to this). |

No blockers. No stubs. No placeholder returns.

---

### Human Verification Required

None required. All phase 17 artifacts are pure computation code with no visual or real-time behavior. All observable behaviors are fully verified by the 20 passing automated tests.

---

### Gaps Summary

**One gap blocking full DC-19 satisfaction: acceleration constraints.**

DC-19 states: "Toolpath respects machine profile travel limits **and acceleration constraints**." The travel limits half is fully implemented (`validateLimits()` with per-axis warnings, tested). The acceleration constraints half was never addressed — Plan 17-02's design only covers travel limits, and neither the `ToolpathConfig` struct nor the `validateLimits()` function nor any other toolpath code references acceleration values, machine profile acceleration limits, or velocity ramping.

This gap is notable but not blocking phase progression in practice: most GRBL/GBRL-HAL firmware handles acceleration internally (the controller knows its own acceleration limits), and many CAM tools do not perform acceleration-aware motion planning. The question is whether the requirement truly means the toolpath generator must enforce acceleration, or whether firmware-handled acceleration satisfies the requirement. This is a **design decision that needs a human call**, not a coding gap.

If the decision is "firmware handles acceleration — toolpath generator need not duplicate this," then DC-19 is satisfied as-is and the requirement text should be clarified. If the decision is "the generator must validate against machine profile acceleration limits," then `validateLimits()` needs to accept and check acceleration parameters.

---

_Verified: 2026-02-28T12:30:00Z_
_Verifier: Claude (gsd-verifier)_
