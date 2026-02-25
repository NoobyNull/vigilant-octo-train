# Phase 5: Job Streaming - Research

**Researched:** 2026-02-24
**Domain:** CNC job progress tracking, feed rate deviation detection, time estimation
**Confidence:** HIGH

## Summary

Phase 5 adds job progress visibility during G-code streaming. The existing infrastructure is well-suited: `StreamProgress` already tracks `totalLines`, `ackedLines`, `elapsedSeconds`, and `errorCount`. The `GCodePanel` already has a basic progress bar with line-rate ETA. The `CncToolPanel` holds calculator results (`CalcResult` with `feed_rate`). `MachineStatus` provides real-time `feedRate` from GRBL status reports at 5Hz.

The work splits into two concerns: (1) enhancing the existing progress display with elapsed time, remaining time, line counts, and progress percentage as formal displays rather than a progress bar overlay string, and (2) adding feed deviation detection by comparing `MachineStatus.feedRate` against `CalcResult.feed_rate` from the tool panel.

**Primary recommendation:** Add a `CncJobPanel` that receives both `StreamProgress` and `MachineStatus` callbacks, holds a pointer to `CncToolPanel` (or receives the recommended feed rate directly), and renders all job progress info plus feed deviation warnings. This keeps `GCodePanel` focused on viewing/sending and `CncStatusPanel` focused on machine state.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- Compare actual feed rate from GRBL status to the calculator's recommended feed rate
- Visual warning when deviation exceeds 20% (red highlight or warning icon)
- This requires Phase 4's tool integration to know what the recommended feed is
- Elapsed time, estimated remaining time, current line / total lines, progress %
- Remaining time should adjust dynamically based on current feed rate, not static estimate
- Progress display should be integrated into the CNC status panel or a dedicated job panel

### Claude's Discretion
- Whether job progress lives in CncStatusPanel or a new CncJobPanel
- Progress bar style (linear bar, percentage text, both)
- Time format (HH:MM:SS, minutes only, adaptive)
- How to calculate remaining time (feed-rate-weighted, simple linear extrapolation)
- Feed deviation warning presentation (inline, toast, overlay)

### Deferred Ideas (OUT OF SCOPE)
None
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| TAC-04 | Running feed rate compared to calculator recommendation with visual warning if >20% deviation | MachineStatus.feedRate (5Hz from GRBL) vs CalcResult.feed_rate from CncToolPanel; deviation = abs(actual - recommended) / recommended |
| TAC-05 | Job elapsed time displayed and updated during streaming | StreamProgress.elapsedSeconds already available; format as HH:MM:SS |
| TAC-06 | Estimated remaining time from line progress and current feed rate | Use feed-rate-weighted estimation: track actual vs commanded ratio to scale simple line-rate ETA |
| TAC-07 | Current line number and total line count during streaming | StreamProgress.ackedLines / StreamProgress.totalLines already available |
| TAC-08 | Job progress percentage | StreamProgress.ackedLines / StreamProgress.totalLines * 100 |
</phase_requirements>

## Architecture Patterns

### Recommended: New CncJobPanel

Create a dedicated `CncJobPanel` rather than overloading `CncStatusPanel`:

**Rationale:**
- `CncStatusPanel` shows machine state (DRO, position, feed/spindle) — always relevant
- Job progress is only relevant during streaming — separate panel can show/hide contextually
- Feed deviation comparison needs access to calculator results — cleaner as dedicated panel
- `GCodePanel` already has a basic progress bar in Send mode — `CncJobPanel` provides the comprehensive view

**Data flow:**
```
CncController (IO thread)
  -> MainThreadQueue callbacks
    -> CncJobPanel::onProgressUpdate(StreamProgress)
    -> CncJobPanel::onStatusUpdate(MachineStatus)

CncToolPanel (UI thread)
  -> CncJobPanel::setRecommendedFeedRate(float)
  (or CncJobPanel holds pointer to CncToolPanel and reads CalcResult)
```

### Pattern: Feed Deviation Detection

```cpp
// In CncJobPanel, called on each status update:
void checkFeedDeviation() {
    if (m_recommendedFeedRate <= 0.0f || !m_streaming) return;

    float actual = m_machineStatus.feedRate;
    float recommended = m_recommendedFeedRate;
    float deviation = std::abs(actual - recommended) / recommended;

    m_feedDeviation = deviation;
    m_feedDeviationWarning = (deviation > 0.20f); // >20%
}
```

**Important considerations:**
- GRBL reports `feedRate` as 0 when idle or decelerating between segments — only check during `MachineState::Run`
- Feed overrides affect actual feed rate — the deviation check should account for the override percentage: `effectiveRecommended = recommended * (feedOverride / 100.0f)`
- Rapid moves (G0) have no commanded feed rate — skip deviation check during rapids (GRBL may report max rate)
- The `feedRate` in GRBL status is instantaneous — may spike/dip during acceleration. Use a short moving average (3-5 samples) to avoid flicker

### Pattern: Dynamic Time Estimation

The existing `renderProgressBar()` uses simple line-rate ETA:
```
rate = ackedLines / elapsed
remaining = (totalLines - ackedLines) / rate
```

This is adequate for TAC-06's requirement of "adjusts based on current feed rate." The line completion rate naturally reflects the actual feed rate — when feed override is reduced, lines take longer, ETA adjusts. No separate feed-rate-weighted calculation is needed.

For a smoother estimate, use exponential moving average of recent line completion times:
```cpp
float recentRate = recentLines / recentWindow; // e.g., last 10 seconds
float overallRate = ackedLines / elapsed;
float blendedRate = 0.7f * recentRate + 0.3f * overallRate; // Bias toward recent
float eta = remainingLines / blendedRate;
```

### Pattern: Time Formatting

Use adaptive format:
- Under 1 minute: "42s"
- Under 1 hour: "12:34" (MM:SS)
- Over 1 hour: "1:23:45" (H:MM:SS)

### Anti-Patterns to Avoid
- **Polling CalcResult from CncToolPanel every frame:** Instead, push the recommended feed rate when it changes (on tool/material selection)
- **Showing feed deviation during idle/jog/hold:** Only meaningful during `MachineState::Run` while streaming
- **Rebuilding progress string every frame:** Cache and update only on callback receipt

## Existing Code Integration Points

### StreamProgress (cnc_types.h:74-79)
Already has: `totalLines`, `ackedLines`, `errorCount`, `elapsedSeconds`. No changes needed.

### CncCallbacks (cnc_types.h:158-167)
Already has: `onProgressUpdate(StreamProgress)`, `onStatusUpdate(MachineStatus)`. The new panel needs to be wired into both.

### application_wiring.cpp (line 555-588)
CNC callbacks are wired here. New panel needs to be added to the callback lambdas alongside existing panels (csp, jogp, conp, wcsp).

### GCodePanel::renderProgressBar() (gcode_panel.cpp:575-598)
Existing basic progress bar. Can remain as-is (compact inline view in Send mode) or be simplified since CncJobPanel provides the detailed view.

### CncToolPanel (cnc_tool_panel.h)
Holds `CalcResult m_calcResult` and `bool m_hasCalcResult`. Need to expose `getCalcResult()` or `getRecommendedFeedRate()` for the job panel.

### UIManager (ui_manager.h/cpp)
Need to add `CncJobPanel` following the same pattern as other CNC panels: unique_ptr member, accessor, render in CNC workspace mode, show/hide toggle.

## Common Pitfalls

### Pitfall 1: Feed Rate 0 During Deceleration
**What:** GRBL reports `feedRate = 0` when decelerating between segments or at segment boundaries.
**Prevention:** Only flag deviation when `feedRate > 0` AND `MachineState::Run`. Apply a short debounce (ignore zero readings for ~0.5s).

### Pitfall 2: Feed Override Not Accounted For
**What:** User sets 50% feed override, actual feed is 50% of commanded, but panel shows 50% deviation warning.
**Prevention:** Adjust recommended feed by override percentage: `adjusted = recommended * (status.feedOverride / 100.0f)`.

### Pitfall 3: ETA Instability at Start
**What:** In the first few seconds, ETA swings wildly because few lines have been completed.
**Prevention:** Don't show ETA until at least 5 lines or 3 seconds have passed.

### Pitfall 4: Thread Safety on CalcResult Access
**What:** CncToolPanel updates CalcResult on main thread, CncJobPanel reads it on main thread — both are main thread, no issue. But if someone later makes CncJobPanel read directly from CncToolPanel pointer, ensure both are on the same thread.
**Prevention:** Keep all UI reads/writes on main thread (already the pattern).

## Sources

### Primary (HIGH confidence)
- Direct codebase analysis: `src/core/cnc/cnc_controller.h`, `cnc_controller.cpp`, `cnc_types.h`
- Direct codebase analysis: `src/ui/panels/gcode_panel.h`, `gcode_panel.cpp`
- Direct codebase analysis: `src/ui/panels/cnc_status_panel.h`, `cnc_status_panel.cpp`
- Direct codebase analysis: `src/ui/panels/cnc_tool_panel.h`, `cnc_tool_panel.cpp`
- Direct codebase analysis: `src/app/application_wiring.cpp`
- Direct codebase analysis: `src/core/cnc/tool_calculator.h`

## Metadata

**Confidence breakdown:**
- Architecture: HIGH — all integration points verified in codebase
- Feed deviation: HIGH — GRBL status report format and callback chain verified
- Time estimation: HIGH — existing implementation reviewed, enhancement straightforward
- Pitfalls: HIGH — derived from GRBL protocol knowledge and codebase patterns

**Research date:** 2026-02-24
**Valid until:** 2026-03-24
