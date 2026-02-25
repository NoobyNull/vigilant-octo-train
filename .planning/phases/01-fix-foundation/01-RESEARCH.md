# Phase 1: Fix Foundation - Research

**Researched:** 2026-02-24
**Domain:** GRBL CNC controller serial communication (C++17, POSIX serial, threading)
**Confidence:** HIGH

## Summary

Phase 1 addresses four specific bugs in the existing CncController and SerialPort code. All four are well-understood problems with clear solutions grounded in POSIX serial APIs and established GRBL sender patterns. The existing code architecture (IO thread + MainThreadQueue callbacks) is sound; the fixes are targeted corrections to the serial port layer and command dispatch path.

The thread safety fix (FND-01) is the most architecturally significant: all real-time command methods (`feedHold()`, `cycleStart()`, `softReset()`, overrides) currently write directly to the serial port from the calling (UI) thread while the IO thread simultaneously reads/writes the same file descriptor. The fix routes these through a lock-free command queue consumed by the IO thread.

The remaining three fixes (FND-02, FND-03, FND-04) are localized changes to `processResponse()`, `readLine()`, and `open()` respectively.

**Primary recommendation:** Fix FND-01 first (thread safety) since it changes the command dispatch architecture that FND-02's soft-reset-on-error depends on.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- Safety is paramount -- every fix should prioritize preventing dangerous machine behavior over convenience
- Ease of use is secondary -- fixes should result in clear, actionable feedback to the operator
- Real-time commands (feedHold, cycleStart, overrides) must not write directly to SerialPort from the UI thread
- Route all serial writes through the IO thread to eliminate data races
- The fix must not add latency to safety-critical commands (feed hold must still be near-instant)
- On streaming error: issue soft reset (0x18) immediately to flush GRBL's buffer
- After soft reset: transition to a clear error state in UI, not silently recover
- Operator must acknowledge the error before the controller allows new operations
- USB disconnect must be detected within 2 seconds and reported clearly to the UI
- No auto-reconnect -- manual reconnect only (safety: machine state is unknown after disconnect)
- Connection loss during a streaming job must trigger the same error handling as FND-02
- Suppress DTR on serial port open by default to prevent Arduino board reset
- This is the safe default -- losing machine position is dangerous
- No configuration needed; DTR suppression is always-on

### Claude's Discretion
- Exact implementation pattern for command queue vs mutex vs single-writer for thread safety
- Poll interval and timeout thresholds for disconnect detection
- Whether to use POLLHUP, consecutive timeout counter, or both for disconnect detection
- Internal error state machine transitions

### Deferred Ideas (OUT OF SCOPE)
- EEPROM-aware streaming protocol switching -- may belong in Phase 5 (Job Streaming) or Phase 7 (Firmware Settings) rather than here
- Transport abstraction (SerialPort -> Transport interface for future WebSocket) -- not needed for bug fixes
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|-----------------|
| FND-01 | Real-time commands dispatched thread-safely via IO thread | Lock-free command queue pattern; `std::atomic` flag or SPSC queue checked each IO loop iteration |
| FND-02 | Streaming engine issues soft reset on unrecoverable error | Modify `processResponse()` to send `0x18` on `error:` during streaming, flush buffer state, notify UI |
| FND-03 | Serial port detects USB disconnect via POLLHUP/POLLERR | Check `revents` for POLLHUP/POLLERR in `readLine()`, count consecutive timeouts, verify device file exists |
| FND-04 | Serial port suppresses DTR on open | Clear HUPCL flag in termios config and explicitly lower DTR via TIOCMBIC ioctl after open |
</phase_requirements>

## Standard Stack

### Core (Already In Use)
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| C++17 std::thread | - | IO thread | Already used, correct for dedicated worker thread |
| C++17 std::atomic | - | Thread-safe flags | Already used for `m_running`, `m_streaming`, `m_held` |
| C++17 std::mutex | - | Stream state protection | Already used for `m_streamMutex` |
| POSIX termios | - | Serial port configuration | Linux-standard serial API |
| POSIX poll() | - | Non-blocking serial reads | Already used in `readLine()` |
| MainThreadQueue | - | Worker-to-UI thread callbacks | Project's existing thread-safe callback mechanism |

### No New Dependencies Required

This phase fixes existing code. No new libraries are needed. The POSIX serial API provides everything required for DTR suppression (`TIOCMBIC` ioctl) and disconnect detection (`POLLHUP`/`POLLERR` flags).

## Architecture Patterns

### Pattern 1: Atomic Command Queue for Real-Time Commands

**What:** Replace direct `m_port.writeByte()` calls in UI-thread methods with atomic flags/queue that the IO thread checks each loop iteration.

**When to use:** When the UI thread needs to inject real-time commands without blocking and without data races on the serial file descriptor.

**Recommended approach:** Use a `std::atomic<uint32_t>` bitmask for single-byte commands (feed hold, cycle start, soft reset, jog cancel) and a small mutex-protected queue for multi-byte sequences (override adjustments). The IO thread checks the atomic bitmask at the top of each loop iteration (every ~20ms from the `readLine` timeout) and dispatches immediately.

**Why not a lock-free SPSC queue:** The command set is small and fixed. A bitmask is simpler, faster, and sufficient. Override adjustments (which send multiple bytes) are the only multi-step commands and they're not safety-critical, so a small mutex-protected queue is acceptable.

```cpp
// Atomic bitmask for single-byte real-time commands
std::atomic<uint32_t> m_pendingRtCommands{0};

enum RtCommand : uint32_t {
    RT_FEED_HOLD    = 1 << 0,
    RT_CYCLE_START  = 1 << 1,
    RT_SOFT_RESET   = 1 << 2,
    RT_JOG_CANCEL   = 1 << 3,
};

// UI thread (feedHold, cycleStart, etc.)
void CncController::feedHold() {
    m_pendingRtCommands.fetch_or(RT_FEED_HOLD, std::memory_order_release);
    m_held = true;
}

// IO thread (top of loop)
void CncController::dispatchPendingCommands() {
    uint32_t pending = m_pendingRtCommands.exchange(0, std::memory_order_acquire);
    if (pending & RT_SOFT_RESET)   m_port.writeByte(cnc::CMD_SOFT_RESET);
    if (pending & RT_FEED_HOLD)    m_port.writeByte(cnc::CMD_FEED_HOLD);
    if (pending & RT_CYCLE_START)  m_port.writeByte(cnc::CMD_CYCLE_START);
    if (pending & RT_JOG_CANCEL)   m_port.writeByte(0x85);
}
```

**Latency analysis:** The IO thread's `readLine(20)` call returns within 20ms even if no data arrives. Real-time commands will be dispatched within at most 20ms of being posted. This is acceptable for CNC safety (feed hold at 10,000mm/min covers ~3.3mm in 20ms -- well within mechanical tolerance). If lower latency is needed later, the IO thread can check between poll iterations, but 20ms is standard for GRBL senders.

### Pattern 2: Error-Triggered Soft Reset During Streaming

**What:** When `processResponse()` receives `error:N` while `m_streaming` is true, immediately send soft reset (`0x18`) to flush GRBL's RX buffer, preventing buffered lines from executing in an incorrect state.

**Sequence:**
1. Receive `error:N` during streaming
2. Send `0x18` (soft reset) -- flushes GRBL's RX buffer
3. Set `m_streaming = false`
4. Clear buffer accounting (`m_sentLengths`, `m_bufferUsed`)
5. Drain serial buffers
6. Set error state flag
7. Notify UI via `onError` callback with error details
8. UI must show error and require operator acknowledgment before new operations

### Pattern 3: Disconnect Detection via POLLHUP + Timeout Counter

**What:** Detect USB disconnect through two complementary mechanisms: (1) checking `POLLHUP`/`POLLERR` flags in poll results, and (2) counting consecutive read timeouts after status queries that should produce responses.

**How it works:**
- In `readLine()`: after `poll()` returns, check `pfd.revents & (POLLHUP | POLLERR)` -- if set, return a distinct error (not just `nullopt`)
- In the IO loop: if `requestStatus()` was sent but no status response arrives for N consecutive polls (e.g., 5 consecutive = 1 second at 5Hz polling), declare disconnect
- Additionally: after write failures, check `errno` for `EIO`, `ENXIO`, `ENODEV` which indicate device removal

**API change needed:** `readLine()` currently returns `std::optional<std::string>` which conflates "timeout" with "error" with "disconnect". To distinguish these, either:
- Return an enum/struct with the result type, OR
- Add a `lastError()` method to SerialPort, OR
- Use a tri-state return: `nullopt` = timeout, empty string = error, string = data

**Recommendation:** Add a `SerialPort::ConnectionState` enum and a `connectionState()` getter that the IO thread checks after each `readLine()` call. This keeps the `readLine()` API simple while providing disconnect information.

### Pattern 4: DTR Suppression on Serial Open

**What:** Prevent the serial port from toggling DTR on open/close, which would reset an Arduino-based GRBL controller.

**Implementation:**
```cpp
// In SerialPort::open(), after tcsetattr:

// Clear HUPCL to prevent DTR toggle on close
tty.c_cflag &= ~HUPCL;

// After tcsetattr succeeds, explicitly clear DTR:
int bits = TIOCM_DTR;
ioctl(m_fd, TIOCMBIC, &bits);
```

**Note:** `HUPCL` (hang up on close) must be cleared BEFORE the first `tcsetattr` call to prevent DTR from toggling during configuration. The `TIOCMBIC` ioctl explicitly lowers DTR after open. Both are needed: `HUPCL` prevents DTR toggle on close, `TIOCMBIC` prevents it on open if the driver raised it.

### Anti-Patterns to Avoid
- **Mutex around all serial writes:** Adding a mutex to `SerialPort::write()`/`writeByte()` would make it "thread-safe" but would add lock contention and doesn't solve the architectural issue. Route through IO thread instead.
- **Sleep-based disconnect detection:** Don't use `usleep()` or fixed delays to detect disconnect. Use POSIX poll flags and timeout counters.
- **Silent error recovery:** Don't suppress errors or auto-retry after streaming errors. The user decided: operator must acknowledge.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Lock-free queue | Custom ring buffer | `std::atomic<uint32_t>` bitmask | Command set is small and fixed; bitmask is trivially correct |
| Serial config | Manual bit manipulation | `cfmakeraw()` + flag adjustments | POSIX standard, handles all the edge cases |
| Thread joining | Complex shutdown protocols | `std::atomic<bool>` flag + `join()` | Already working correctly in existing code |

## Common Pitfalls

### Pitfall 1: Soft Reset Clears GRBL State
**What goes wrong:** After sending soft reset (0x18), GRBL resets its parser state. If the code immediately tries to continue operations, GRBL may be in an unexpected state.
**Why it happens:** Soft reset is a full parser reset, not just a buffer flush.
**How to avoid:** After soft reset, wait for the GRBL banner/greeting before allowing new commands. The IO thread should enter a "resetting" state that waits for the banner before transitioning to "connected/idle."
**Warning signs:** Commands fail after error recovery. Status queries return unexpected states.

### Pitfall 2: POLLHUP Not Immediate on All Kernels
**What goes wrong:** On some Linux kernels/USB drivers, `POLLHUP` may not be set immediately when a USB device is unplugged. The file descriptor may remain "valid" for a short time.
**Why it happens:** Kernel USB stack processing delay.
**How to avoid:** Use BOTH `POLLHUP` detection AND consecutive timeout counting. The timeout counter catches cases where `POLLHUP` is delayed.
**Warning signs:** Disconnect detection works on some machines but not others.

### Pitfall 3: Partial Writes on Serial
**What goes wrong:** POSIX `write()` can legally return fewer bytes than requested, especially on slow serial links or when kernel buffers are full.
**Why it happens:** POSIX spec allows partial writes on any file descriptor.
**How to avoid:** Loop on `write()` until all bytes are sent or an error occurs. The current code checks `written == data.size()` but doesn't retry.
**Warning signs:** Truncated commands sent to GRBL. Garbled G-code.

### Pitfall 4: Race Between disconnect() and IO Thread
**What goes wrong:** `disconnect()` is called from the UI thread. It sets `m_running = false` and then joins the IO thread. But if the IO thread is in the middle of a `readLine()` call (blocked in `poll()`), the join blocks until the `poll()` timeout expires.
**Why it happens:** The IO thread's `readLine(20)` blocks for up to 20ms. During that time, the disconnect request is pending.
**How to avoid:** This is already handled correctly -- the 20ms timeout is short enough. But if disconnect latency is a concern, consider using `pthread_cancel` or signaling the poll via a pipe/eventfd (advanced, probably not needed for 20ms).

## Code Examples

### DTR Suppression (POSIX termios + ioctl)
```cpp
#include <sys/ioctl.h>

bool SerialPort::open(const std::string& device, int baudRate) {
    close();
    m_fd = ::open(device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (m_fd < 0) return false;

    struct termios tty {};
    tcgetattr(m_fd, &tty);

    // ... baud rate, raw mode, 8N1 config ...

    // Suppress DTR: clear HUPCL so closing port doesn't toggle DTR
    tty.c_cflag &= ~HUPCL;
    tty.c_cflag |= (CLOCAL | CREAD);

    tcsetattr(m_fd, TCSANOW, &tty);

    // Explicitly lower DTR line after open
    int modemBits = TIOCM_DTR;
    ioctl(m_fd, TIOCMBIC, &modemBits);

    tcflush(m_fd, TCIOFLUSH);
    return true;
}
```

### POLLHUP/POLLERR Detection in readLine
```cpp
std::optional<std::string> SerialPort::readLine(int timeoutMs) {
    // ... existing buffer check ...

    struct pollfd pfd {};
    pfd.fd = m_fd;
    pfd.events = POLLIN;

    int remaining = timeoutMs;
    while (remaining > 0) {
        int ret = poll(&pfd, 1, remaining);
        if (ret < 0) {
            if (errno == EINTR) continue;
            m_connectionState = ConnectionState::Error;
            return std::nullopt;
        }
        if (ret == 0) return std::nullopt; // Timeout (not an error)

        // Check for disconnect/error BEFORE checking POLLIN
        if (pfd.revents & (POLLHUP | POLLERR)) {
            m_connectionState = ConnectionState::Disconnected;
            return std::nullopt;
        }

        if (pfd.revents & POLLIN) {
            char buf[256];
            ssize_t n = ::read(m_fd, buf, sizeof(buf));
            if (n <= 0) {
                m_connectionState = ConnectionState::Disconnected;
                return std::nullopt;
            }
            // ... existing line extraction ...
        }

        remaining -= 10; // TODO: use monotonic clock for accurate timing
    }
    return std::nullopt;
}
```

### Error-Triggered Soft Reset
```cpp
void CncController::processResponse(const std::string& line) {
    // ... existing status report and alarm handling ...

    if (line == "ok" || line.find("error:") == 0) {
        std::lock_guard<std::mutex> lock(m_streamMutex);

        // ... existing buffer accounting ...

        if (!ack.ok && m_streaming) {
            // CRITICAL: Issue soft reset to flush GRBL's RX buffer
            m_port.writeByte(cnc::CMD_SOFT_RESET);
            m_streaming = false;
            m_sentLengths.clear();
            m_bufferUsed = 0;
            m_port.drain();

            // Notify UI of streaming error
            if (m_mtq && m_callbacks.onError) {
                std::string msg = "Streaming error at line " +
                    std::to_string(ack.lineIndex) + ": " + ack.errorMessage +
                    " (soft reset issued, machine stopped)";
                m_mtq->enqueue([cb = m_callbacks.onError, msg]() { cb(msg); });
            }
            return; // Don't process further acks
        }
        // ... rest of existing logic ...
    }
}
```

## Existing Code Analysis

### What's Already Correct
1. **IO thread architecture:** Dedicated IO thread with `MainThreadQueue` callbacks to UI -- textbook correct pattern
2. **Status polling rate:** 5Hz (200ms) is GRBL-recommended
3. **Character-counting streaming:** Buffer accounting logic in `sendNextLines()` is correct
4. **Atomic flags:** `m_running`, `m_streaming`, `m_held` are `std::atomic<bool>` -- correct
5. **GRBL status parser:** `parseStatusReport()` handles all standard fields correctly
6. **WCO handling:** Already computes `WPos = MPos - WCO` correctly

### What Needs Fixing (Mapped to Requirements)
1. **FND-01 - Thread safety:** `feedHold()`, `cycleStart()`, `softReset()`, all override methods, `jogCancel()`, and `unlock()` call `m_port.writeByte()`/`m_port.write()` directly from the calling thread. The IO thread simultaneously uses `m_port`. No synchronization on `SerialPort`.
2. **FND-02 - Soft reset on error:** `processResponse()` handles `error:` by incrementing counters but does NOT issue soft reset. Buffered GRBL lines continue executing.
3. **FND-03 - Disconnect detection:** `readLine()` only checks `POLLIN`, not `POLLHUP`/`POLLERR`. No consecutive timeout counter. No device file existence check.
4. **FND-04 - DTR suppression:** `open()` does not clear `HUPCL` and does not use `TIOCMBIC` to lower DTR.

### Additional Issues to Fix (Bonus, Low Risk)
5. **Partial write handling:** `write()` doesn't retry on partial writes (POSIX allows `write()` to return less than requested)
6. **readLine timing drift:** `remaining -= 10` approximation can accumulate error. Should use monotonic clock.

## Open Questions

1. **Override command dispatch latency**
   - What we know: Override adjustments (feed, spindle) send multiple bytes in sequence. If using a queue, they need to be sent atomically (all bytes for one adjustment together).
   - What's unclear: Whether GRBL handles interleaved override bytes correctly if another real-time command arrives between them.
   - Recommendation: Batch override bytes into a single queue entry to guarantee atomic dispatch. This is a low-risk defensive measure.

2. **Soft reset recovery timing**
   - What we know: After `0x18`, GRBL sends a welcome banner after ~500ms-1s.
   - What's unclear: Exact timing varies by firmware and board speed.
   - Recommendation: After soft reset on error, enter a "resetting" state. Wait for the banner before allowing new operations. Use a generous timeout (3 seconds).

## Sources

### Primary (HIGH confidence)
- Source code analysis: `src/core/cnc/cnc_controller.cpp`, `serial_port.cpp`, `cnc_types.h`
- POSIX termios documentation: `man termios`, `man ioctl_tty`
- `.planning/research/PITFALLS.md` -- domain pitfalls already researched

### Secondary (MEDIUM confidence)
- [GRBL v1.1 Interface](https://github.com/gnea/grbl/wiki/Grbl-v1.1-Interface) -- streaming protocols, real-time commands
- [GRBL Known Issues](https://github.com/gnea/grbl/wiki/Known-Issues) -- EEPROM write, buffer behavior
- [Arduino DTR Disable](https://playground.arduino.cc/Main/DisablingAutoResetOnSerialConnection/) -- DTR suppression techniques

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- already implemented, just fixing bugs
- Architecture: HIGH -- IO thread pattern is sound, command queue is well-understood
- Pitfalls: HIGH -- all issues identified in code review, solutions verified against POSIX APIs

**Research date:** 2026-02-24
**Valid until:** Indefinite (POSIX APIs and GRBL protocol are stable)
