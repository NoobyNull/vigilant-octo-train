# Phase 5: Job Streaming - Context

**Gathered:** 2026-02-24
**Status:** Ready for planning

<domain>
## Phase Boundary

Full job progress visibility during G-code streaming with feed deviation warnings. Elapsed/remaining time, line counts, progress percentage, and comparison of running feed rate to calculator recommendations. Requirements: TAC-04, TAC-05, TAC-06, TAC-07, TAC-08.

</domain>

<decisions>
## Implementation Decisions

### Feed Deviation Warning (TAC-04)
- Compare actual feed rate from GRBL status to the calculator's recommended feed rate
- Visual warning when deviation exceeds 20% (red highlight or warning icon)
- This requires Phase 4's tool integration to know what the recommended feed is

### Job Progress Display (TAC-05-08)
- Elapsed time, estimated remaining time, current line / total lines, progress %
- Remaining time should adjust dynamically based on current feed rate, not static estimate
- Progress display should be integrated into the CNC status panel or a dedicated job panel

### Claude's Discretion
- Whether job progress lives in CncStatusPanel or a new CncJobPanel
- Progress bar style (linear bar, percentage text, both)
- Time format (HH:MM:SS, minutes only, adaptive)
- How to calculate remaining time (feed-rate-weighted, simple linear extrapolation)
- Feed deviation warning presentation (inline, toast, overlay)

</decisions>

<specifics>
## Specific Ideas

- The existing StreamProgress struct already tracks acknowledged lines and timing
- CncController already has character-counting streaming with line acknowledgment callbacks
- Phase 4 provides the calculator recommendation to compare against

</specifics>

<deferred>
## Deferred Ideas

None — discussion stayed within phase scope

</deferred>

---

*Phase: 05-job-streaming*
*Context gathered: 2026-02-24*
