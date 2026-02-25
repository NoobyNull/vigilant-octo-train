# Phase 1: Fix Foundation - Context

**Gathered:** 2026-02-24
**Status:** Ready for planning

<domain>
## Phase Boundary

Resolve thread safety, error handling, and disconnect detection bugs in the existing CncController and SerialPort code. No new features — only fixes to make the controller reliable enough to build on. Requirements: FND-01, FND-02, FND-03, FND-04.

</domain>

<decisions>
## Implementation Decisions

### Priority Order
- Safety is paramount — every fix should prioritize preventing dangerous machine behavior over convenience
- Ease of use is secondary — fixes should result in clear, actionable feedback to the operator

### Thread Safety (FND-01)
- Real-time commands (feedHold, cycleStart, overrides) must not write directly to SerialPort from the UI thread
- Route all serial writes through the IO thread to eliminate data races
- The fix must not add latency to safety-critical commands (feed hold must still be near-instant)

### Error Recovery (FND-02)
- On streaming error: issue soft reset (0x18) immediately to flush GRBL's buffer — do not let queued commands continue executing
- After soft reset: transition to a clear error state in UI, not silently recover
- Operator must acknowledge the error before the controller allows new operations

### Disconnect Detection (FND-03)
- USB disconnect must be detected within 2 seconds and reported clearly to the UI
- No auto-reconnect — manual reconnect only (safety: machine state is unknown after disconnect)
- Connection loss during a streaming job must trigger the same error handling as FND-02

### DTR Suppression (FND-04)
- Suppress DTR on serial port open by default to prevent Arduino board reset
- This is the safe default — losing machine position is dangerous
- No configuration needed; DTR suppression is always-on

### Claude's Discretion
- Exact implementation pattern for command queue vs mutex vs single-writer for thread safety
- Poll interval and timeout thresholds for disconnect detection
- Whether to use POLLHUP, consecutive timeout counter, or both for disconnect detection
- Internal error state machine transitions

</decisions>

<specifics>
## Specific Ideas

- Research identified 7 existing bugs in CncController (documented in .planning/research/PITFALLS.md)
- The thread safety violation is in feedHold(), cycleStart(), and override methods that call SerialPort directly from the UI thread
- Current readLine() doesn't check POLLHUP/POLLERR flags
- Current SerialPort::open() doesn't suppress DTR
- EEPROM write data loss (GRBL drops serial bytes for ~20ms during EEPROM write) — note this but it may be Phase 7 scope if it only affects settings writes

</specifics>

<deferred>
## Deferred Ideas

- EEPROM-aware streaming protocol switching — may belong in Phase 5 (Job Streaming) or Phase 7 (Firmware Settings) rather than here
- Transport abstraction (SerialPort → Transport interface for future WebSocket) — not needed for bug fixes

</deferred>

---

*Phase: 01-fix-foundation*
*Context gathered: 2026-02-24*
