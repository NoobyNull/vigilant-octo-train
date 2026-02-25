# Phase 6: Job Safety - Context

**Gathered:** 2026-02-24
**Status:** Ready for planning

<domain>
## Phase Boundary

Safe pause/resume/E-stop controls, safe job resume from arbitrary line with modal state reconstruction, sensor status display, and pre-flight checks before streaming. Requirements: SAF-01 through SAF-07.

</domain>

<decisions>
## Implementation Decisions

### Priority: Safety First
- Safety is paramount (established in Phase 1 context)
- E-stop must be the most prominent, impossible-to-miss control
- Resume operations must verify machine state before continuing

### Pause/Resume/E-stop (SAF-01-03)
- Pause sends feed hold (!), Resume sends cycle start (~)
- E-stop sends soft reset (0x18) — requires confirmation dialog when job is running
- Button states must reflect machine state (e.g., Resume only enabled in Hold state)

### Safe Job Resume (SAF-04-05)
- User specifies line number to resume from
- System scans G-code from line 0 to resume point, building modal state preamble
- Preamble includes: G90/G91, coordinate system, spindle, coolant, feed rate, units
- This is the most complex and dangerous feature — must be thoroughly tested

### Sensor Display (SAF-06)
- Show endstop, probe, and door states from GRBL Pn: field
- Read-only display — just show what GRBL reports

### Pre-flight Checks (SAF-07)
- Before streaming: verify connection active, no alarm state
- Optional warnings: tool selected, material selected
- Don't block streaming on optional checks — warn only

### Claude's Discretion
- E-stop button size, color, placement (should be prominent)
- Resume dialog layout (line number input, state preview)
- Whether pre-flight is a modal dialog or inline warnings
- Sensor display format (icons, text, LED-style indicators)
- How to present the modal state preamble to the operator for review before resume

</decisions>

<specifics>
## Specific Ideas

- Research identified that modal state reconstruction is complex and dangerous if wrong
- Arc resume (G2/G3 mid-point) is an edge case — handle gracefully (skip or warn)
- Feed hold is already implemented in Phase 1's thread-safe command dispatch

</specifics>

<deferred>
## Deferred Ideas

- Arc mid-point resume (G2/G3) — edge case, warn and skip for now
- Automatic resume after power loss — would require persistent state storage, future work

</deferred>

---

*Phase: 06-job-safety*
*Context gathered: 2026-02-24*
