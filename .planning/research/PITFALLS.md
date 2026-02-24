# Domain Pitfalls: CNC Controller Suite

**Domain:** GRBL-based CNC control software (C++17 desktop, serial communication)
**Researched:** 2026-02-24
**Confidence:** HIGH (verified against GRBL wiki, gnea/grbl issues, community reports)

---

## Critical Pitfalls

Mistakes that cause hardware damage, data loss, or require architectural rewrites.

---

### CRITICAL-1: EEPROM Write Causes Serial Data Loss

**What goes wrong:** When GRBL writes to EEPROM (settings changes via `$x=`, `G10 L2/L20`, `G28.1`, `G30.1`, `$Nx=`, `$I=`, `$RST=`), the AVR disables ALL interrupts including the serial RX ISR for up to 20ms. Any bytes sent during this window are silently dropped. If using character-counting protocol, the sender's byte count drifts permanently out of sync with GRBL's actual buffer state, causing eventual buffer overflow or stream stalls.

**Why it happens:** AVR hardware requirement for EEPROM writes. Not a firmware bug -- fundamental hardware limitation.

**Consequences:** Lost G-code lines mid-stream. Buffer accounting permanently wrong. Machine executes unexpected moves. Potential workpiece/tool damage.

**Warning signs:** Occasional `error:` responses when streaming programs containing `G10` commands. Random stalls during settings changes. Programs that work in `$C` check mode but fail during real execution.

**Prevention:**
1. Maintain a list of EEPROM-writing commands: `G10 L2`, `G10 L20`, `G28.1`, `G30.1`, `$x=`, `$I=`, `$Nx=`, `$RST=`
2. When any EEPROM command is detected, fall back to send-response protocol (wait for `ok` before sending the next line)
3. Add a 50ms delay after receiving `ok` for EEPROM write commands before resuming character-counting
4. Never stream `$` settings commands with character-counting protocol -- always use send-response

**Phase:** Serial Communication / Streaming Engine (must be in the core streaming implementation from day one)

**Sources:**
- [GRBL Known Issues - gnea/grbl Wiki](https://github.com/gnea/grbl/wiki/Known-Issues)
- [GRBL Interfacing Wiki](https://github.com/grbl/grbl/wiki/Interfacing-with-Grbl)
- [EEPROM Write Issue #165](https://github.com/gnea/grbl/issues/165)

---

### CRITICAL-2: Character-Counting Buffer Desync After Errors

**What goes wrong:** In character-counting mode, when GRBL encounters a parse error in a buffered line, it discards that line and returns `error:N`, but continues processing the remaining lines already in the RX buffer. The GUI cannot abort those buffered commands -- they will execute regardless. If the error caused an unexpected state (wrong coordinate system, missing feed rate), subsequent buffered lines execute in that incorrect state.

**Why it happens:** Character-counting pre-loads GRBL's 128-byte RX buffer with multiple commands. Once bytes are in the buffer, only a soft-reset can stop them. The GUI has already sent them and cannot retract them.

**Consequences:** Machine executes G-code lines the user thinks were stopped. Potential rapid moves at wrong position. Tool crashes into workpiece or fixtures.

**Warning signs:** Errors during streaming where the machine keeps moving after the GUI shows "stopped." Line numbers in error callbacks don't match expected sequence.

**Prevention:**
1. Pre-validate all G-code with `$C` check mode before streaming (run entire program in check mode first)
2. On any `error:` response during streaming, immediately issue feed hold (`!`) then soft reset (`0x18`) -- do NOT simply stop sending
3. Track the number of in-flight lines and warn the user that N lines may execute after an error
4. Consider offering a "safe streaming" mode that uses send-response protocol (slower but fully controllable)

**Relevant to existing code:** The current `CncController::processResponse` sets `m_streaming = false` on alarm but does NOT issue a soft reset on `error:`. Lines already in the GRBL buffer will continue executing. This needs fixing.

**Phase:** Streaming Engine (core safety feature)

**Sources:**
- [GRBL v1.1 Interface - Streaming Protocols](https://github.com/gnea/grbl/wiki/Grbl-v1.1-Interface)
- [Buffer Overflow Issue #133 - CNCjs](https://github.com/cncjs/cncjs/issues/133)

---

### CRITICAL-3: Software E-Stop Is Not a Real E-Stop

**What goes wrong:** GUI provides a big red "E-STOP" button that sends GRBL soft-reset (`0x18`). Users trust it like a hardware E-stop. But: if the serial connection is lost (USB disconnect, driver crash, OS freeze), the button does nothing. The machine keeps running. Spindle keeps spinning. If GRBL firmware crashes, the button does nothing.

**Why it happens:** Developers conflate "soft reset" with "emergency stop." A real E-stop must be a hardware circuit that physically disconnects motor power and spindle power independent of any software.

**Consequences:** User relies on software E-stop during an emergency. Connection is lost. Machine causes damage or injury.

**Warning signs:** Users referring to the software button as their "E-stop." No documentation distinguishing software stop from hardware E-stop. No prompt to verify hardware E-stop exists.

**Prevention:**
1. NEVER label any software button "E-STOP" or "Emergency Stop" -- use "Soft Reset" or "Abort"
2. Display a persistent warning if no hardware E-stop is configured/detected
3. On first connection, show a safety checklist that includes verifying hardware E-stop function
4. Document clearly: "Software controls cannot substitute for hardware emergency stop circuits"
5. The soft-reset button should be prominent and accessible but clearly labeled as a software function

**Phase:** Safety Features / UI Design (address in initial UI design, enforce throughout)

**Sources:**
- [CNCZone E-Stop Discussion](https://www.cnczone.com/forums/safety-zone/360406-cnc-cam.html)
- [LinuxCNC Emergency Stop Circuit](https://forum.linuxcnc.org/27-driver-boards/37025-emergency-stop-circuit)

---

### CRITICAL-4: Unsafe Resume After Feed Hold / Alarm

**What goes wrong:** After a feed hold or alarm, the user presses "Resume" and the machine crashes into the workpiece. Common causes: (a) the tool was manually moved during the hold but the GUI doesn't re-probe position, (b) spindle wasn't given time to reach speed before cutting resumes, (c) coolant wasn't re-enabled, (d) the program counter resumed at the wrong line after the GUI lost sync during the hold.

**Why it happens:** Resume after interruption is one of the most complex state transitions in CNC control. GRBL handles some of it (spindle/coolant restore with parking enabled), but the GUI must handle program counter tracking, user verification, and state validation.

**Consequences:** Tool crash. Broken bits. Ruined workpieces. Potential injury from unexpected rapid moves.

**Warning signs:** Resume works fine in simple tests but fails during real jobs with tool changes, coordinate system switches, or mid-program holds.

**Prevention:**
1. Before resume, always query current position via `?` and compare with expected position. If drift > threshold, warn user
2. Require explicit user confirmation before resume: "Spindle will start. Tool is at X,Y,Z. Continue?"
3. After alarm reset, require re-homing before allowing any movement
4. Track modal state (G54/G55, G90/G91, feed rate, spindle speed) and restore it before resuming
5. Implement a "resume from line N" feature with automatic modal state reconstruction from program start to line N
6. Add a configurable spindle spin-up delay before cutting resumes

**Phase:** Job Management / Safety Features (complex feature, needs dedicated phase)

**Sources:**
- [CNC Restart Safety Protocol Discussion](https://cnccode.com/2026/02/11/cnc-restart-safety-protocol-2026-how-to-restart-any-program-without-crashing-after-feed-hold-alarm-or-power-loss/)
- [GRBL Feed Hold / Cycle Start Commands](https://github.com/gnea/grbl/blob/master/doc/markdown/commands.md)

---

### CRITICAL-5: Arduino DTR Auto-Reset on Serial Port Open

**What goes wrong:** Opening a serial port on Arduino toggles the DTR line, which resets the Arduino. If the user disconnects and reconnects the GUI (or the GUI restarts while the machine is running), opening the serial port hard-resets GRBL. The machine immediately stops. Stepper motors lose position. If the spindle is running and the tool is engaged, the sudden stop can break the bit or damage the workpiece.

**Why it happens:** Arduino's bootloader design uses DTR toggle as a reset signal for convenient firmware uploading. This is catastrophic for CNC control.

**Consequences:** Unexpected machine reset mid-job. Position loss. Broken tools.

**Warning signs:** Machine resets every time the GUI connects. GRBL banner message appears in console on every connection.

**Prevention:**
1. On Linux: set `stty -F /dev/ttyXXX -hupcl` before opening the port to disable hangup-on-close
2. On connection, detect if GRBL was already running (query status before doing anything) rather than assuming a fresh start
3. Warn users about the DTR reset behavior in documentation
4. Consider adding a "reconnect without reset" option that suppresses DTR toggle
5. For the serial port open sequence: set DTR to false immediately after open if the platform API allows it

**Relevant to existing code:** The current `SerialPort::open()` does `O_NOCTTY` but does not suppress DTR. The `connect()` method sends `CMD_SOFT_RESET` unconditionally -- this is fine for first connection but dangerous for reconnection.

**Phase:** Serial Communication (must be addressed in initial serial port implementation)

**Sources:**
- [Disable Arduino Auto-Reset](https://astroscopic.com/blog/disable-arduinos-auto-reset-connection)
- [Arduino Playground - Disabling Auto Reset](https://playground.arduino.cc/Main/DisablingAutoResetOnSerialConnection/)

---

## Moderate Pitfalls

Mistakes that cause poor UX, intermittent bugs, or significant debugging time.

---

### MODERATE-1: Status Polling Rate Problems (Too Fast or Too Slow)

**What goes wrong:** Polling GRBL status with `?` too frequently (>10Hz) wastes serial bandwidth and can interfere with streaming. Polling too infrequently (<2Hz) makes the DRO (digital readout) feel laggy and unresponsive. Polling on the UI thread blocks rendering.

**Why it happens:** Developers either hardcode a polling rate without testing, or tie polling to the frame rate (which varies), or poll from the UI thread.

**Prevention:**
1. Poll at 5Hz (200ms interval) during idle and streaming. This is the GRBL-recommended rate
2. NEVER poll from the UI thread. The existing architecture (IO thread with `MainThreadQueue` callbacks) is correct
3. The `?` command is a real-time command that bypasses the serial buffer -- do NOT count it toward character-counting buffer usage
4. During jogging, consider increasing to 10Hz for more responsive position feedback, but monitor for serial congestion
5. Decouple display refresh rate from poll rate -- interpolate position between status updates for smooth DRO display

**Relevant to existing code:** Current implementation polls at 5Hz (`STATUS_POLL_MS = 200`). This is correct. The `?` is sent via `writeByte` which bypasses the stream mutex -- also correct. The poll timing uses `remaining -= 10` approximation in `readLine` which could drift; consider using monotonic clock.

**Phase:** Real-time Feedback (UI phase)

**Sources:**
- [GRBL v1.1 Interface - Status Reporting](https://github.com/gnea/grbl/wiki/Grbl-v1.1-Interface)

---

### MODERATE-2: Thread Safety Violations in Serial + UI Architecture

**What goes wrong:** Serial port operations called from multiple threads without synchronization. Common patterns: (a) UI thread calls `writeByte()` for real-time commands while IO thread is in `write()` for streaming, (b) status callbacks modify UI state directly from IO thread, (c) disconnect called from UI thread while IO thread is mid-read.

**Why it happens:** Real-time commands (`!`, `~`, `?`, overrides) need to be sent immediately, bypassing the normal command queue. Developers add direct port writes from the UI thread, creating a race with the IO thread's writes.

**Consequences:** Corrupted serial output (interleaved bytes from two threads). Sporadic crashes. Data races on shared state.

**Warning signs:** Intermittent connection drops. Garbled GRBL responses. TSAN/helgrind warnings.

**Prevention:**
1. All serial port writes must go through a single thread (the IO thread). Real-time commands should be posted to an atomic flag or lock-free queue that the IO thread checks each loop iteration
2. Use `std::atomic` for simple flags (already done for `m_streaming`, `m_held`)
3. Callbacks to the UI must always go through `MainThreadQueue` (already done in existing code)
4. The `disconnect()` method must signal the IO thread to stop and join it -- never close the port while the IO thread might be reading

**Relevant to existing code:** EXISTING BUG: `feedHold()`, `cycleStart()`, `softReset()`, `setFeedOverride()`, etc. call `m_port.writeByte()` directly. These are called from the UI thread but `m_port` is also used by the IO thread. `SerialPort` is not thread-safe. The `::write()` POSIX call is not guaranteed atomic for concurrent callers. This needs to be fixed by routing real-time commands through the IO thread via an atomic command queue or by adding a mutex around all port writes.

**Phase:** Serial Communication (architectural fix needed before adding more features)

---

### MODERATE-3: Buffer Size Mismatch Across GRBL Variants

**What goes wrong:** The code hardcodes `RX_BUFFER_SIZE = 128` (standard GRBL on Arduino Uno). But GRBL-Mega uses 256 bytes, GRBL-ESP32 uses up to 10240 bytes, FluidNC varies, and grblHAL varies. Using 128 bytes on a larger-buffer controller wastes throughput. Using a larger assumed size on a 128-byte controller causes buffer overflow.

**Why it happens:** No standard way to query buffer size from GRBL. Developers pick the safe minimum and forget about it.

**Prevention:**
1. Default to 128 bytes (safe minimum for all variants)
2. Parse the `$I` build info or startup banner to detect the GRBL variant (FluidNC, grblHAL, Mega, ESP32)
3. Allow user override in settings with clear documentation of what each variant supports
4. For ESP32/FluidNC, consider parsing the `[MSG:INFO]` startup messages that sometimes report buffer size
5. Use the GRBL 1.1 `$10=` report mask to get buffer reports in `Bf:` field of status messages, which reports available buffer space

**Phase:** Streaming Engine / Settings

**Sources:**
- [Buffer Size Discussion - gnea/grbl #302](https://github.com/gnea/grbl/issues/302)
- [GRBL-Mega Buffer Size - CNCjs #115](https://github.com/cncjs/cncjs/issues/115)
- [LaserGRBL Buffer Issue #379](https://github.com/arkypita/LaserGRBL/issues/379)

---

### MODERATE-4: Firmware Settings Modification Without Validation or Backup

**What goes wrong:** User changes `$100=250` (steps/mm for X) to `$100=2500` (typo). Now the machine thinks 1mm of travel requires 10x the steps. Rapid moves exceed stepper capability, causing lost steps. Or worse: user sets `$22=0` (disables homing) then tries to use soft limits, which requires homing. GRBL enters an undefined state.

**Why it happens:** GRBL settings are raw key-value pairs with minimal validation. Most GUIs present them as a flat list of text fields with no range checking, dependency validation, or undo.

**Consequences:** Machine moves incorrectly. Lost steps. Soft limit violations. GRBL locks up requiring EEPROM wipe.

**Warning signs:** Users reporting "my machine went crazy after I changed settings." Support requests about machines moving 10x too fast or too slow.

**Prevention:**
1. Maintain a metadata table for all `$x` settings: name, description, type, min/max range, unit, dependencies
2. Validate input before sending: range check, type check, sanity check (e.g., steps/mm should be 1-10000)
3. Auto-backup current settings (`$$` dump) before any modification. Store in a timestamped file
4. Provide "Restore last backup" and "Restore factory defaults" (`$RST=*`) buttons
5. Warn about dangerous settings: `$22` (homing), `$20/$21` (soft/hard limits), `$100-$102` (steps/mm)
6. Group related settings with explanatory labels, not just raw `$N=V` pairs
7. Send settings changes with send-response protocol, never character-counting (EEPROM write issue)

**Phase:** Firmware Settings Management

**Sources:**
- [GRBL v1.1 Configuration](https://github.com/gnea/grbl/wiki/Grbl-v1.1-Configuration)
- [GRBL Settings Basics - All3DP](https://all3dp.com/2/gbrl-settings-gbrl-configuration/)

---

### MODERATE-5: WCO (Work Coordinate Offset) Tracking Failure

**What goes wrong:** GRBL 1.1 optimizes bandwidth by only sending WCO (Work Coordinate Offset) periodically (every 10-30 status reports), not in every report. The GUI must cache the last WCO and compute `WPos = MPos - WCO`. If the GUI misses a WCO update (e.g., during reconnect) or fails to cache it, displayed work position is wrong. User zeros to wrong position, crashes tool.

**Why it happens:** GRBL 1.1 changed from sending WPos every report to sending WCO only periodically. GUIs written for GRBL 0.9 expect WPos in every report.

**Prevention:**
1. Cache the last WCO vector. On every status update, compute WPos from MPos and cached WCO
2. After connection or soft-reset, request a fresh status report and wait for a WCO-containing response before displaying coordinates
3. If WCO has never been received, display "WPos: --" rather than showing MPos as WPos
4. Use `$10=` to configure which fields appear in status reports if needed

**Phase:** Real-time Feedback (DRO implementation)

**Sources:**
- [GRBL v1.1 Interface](https://github.com/gnea/grbl/wiki/Grbl-v1.1-Interface)

---

### MODERATE-6: USB Disconnect Not Detected Promptly

**What goes wrong:** USB cable is unplugged or comes loose. The serial port file descriptor remains valid on Linux (no immediate error). Writes succeed (go to kernel buffer). Reads timeout. The GUI shows "Connected" for seconds or minutes while the machine is actually disconnected and potentially running uncontrolled.

**Why it happens:** POSIX serial semantics don't provide instant disconnect notification. `write()` returns success because it writes to the kernel buffer, not the device. `poll()` may not report `POLLHUP` immediately.

**Prevention:**
1. If N consecutive `readLine()` calls timeout with no response (especially after `?` status queries), declare connection lost
2. Check for `POLLHUP` and `POLLERR` in poll results (the current code only checks `POLLIN`)
3. Monitor `errno` after failed writes for `EIO`, `ENXIO`, or `ENODEV`
4. Periodically verify the device file still exists (`/dev/ttyUSB0` disappears on unplug)
5. Display connection quality indicator (last successful response time) in the UI

**Relevant to existing code:** The current `readLine()` checks `POLLIN` but does not check `POLLHUP`/`POLLERR`. The IO thread loop does not count consecutive timeouts.

**Phase:** Serial Communication (connection health monitoring)

---

## Minor Pitfalls

Issues that cause user frustration or minor bugs.

---

### MINOR-1: Line Ending Mismatch (CR vs LF vs CRLF)

**What goes wrong:** GRBL expects `\n` (LF) line endings. Some G-code files use `\r\n` (CRLF from Windows). If the GUI doesn't strip `\r`, GRBL counts it as a buffer byte, throwing off character-counting by 1 byte per line. Over a long program, this accumulates and causes buffer overflow.

**Prevention:**
1. Strip all `\r` from G-code lines before streaming (already done in `readLine()` for responses, must also be done for outgoing lines)
2. Normalize line endings when loading G-code files
3. The character count for buffer tracking must match exactly what is sent over the wire

**Phase:** Streaming Engine (G-code pre-processing)

---

### MINOR-2: G-code Comments and Whitespace Wasting Buffer Space

**What goes wrong:** G-code files often contain comments `(like this)` or `; like this` and excessive whitespace. These consume GRBL's 128-byte buffer but produce no motion. Long comment lines can exceed the buffer entirely, causing errors.

**Prevention:**
1. Strip comments and leading/trailing whitespace before streaming
2. Strip inline comments: everything after `;` and content within `(` and `)`
3. Remove blank lines
4. Warn if any single line exceeds 70 characters (GRBL max line length is ~80 characters after buffer overhead)

**Phase:** Streaming Engine (G-code pre-processing)

---

### MINOR-3: Jog Commands Not Properly Cancelled

**What goes wrong:** Jogging uses `$J=` commands. If the user releases a jog button but the cancel command (`0x85`) is delayed or lost, the machine continues jogging. If jogging into a limit, the machine alarms.

**Prevention:**
1. Send jog cancel (`0x85`) immediately on button release -- use a real-time command, not the stream queue
2. Use short jog distances with continuous re-issuing while button is held, rather than one long jog
3. After jog cancel, wait for GRBL to report Idle state before allowing new jog commands
4. Implement keyboard repeat rate limiting for jog hotkeys to prevent command flooding

**Phase:** Jogging / Manual Control

---

### MINOR-4: Macro Execution During Streaming

**What goes wrong:** User triggers a macro (e.g., tool change sequence, probe routine) while a streaming job is active. Macro commands interleave with the stream, corrupting the program flow. GRBL receives an unexpected mix of commands.

**Prevention:**
1. Disable macro execution during active streaming (grey out macro buttons)
2. Only allow real-time overrides (feed, spindle, rapid) during streaming
3. Allow macros during feed-hold state only if the macro is a safe inspection routine
4. Clearly indicate in the UI when streaming is active and which controls are available

**Phase:** Macro System

---

### MINOR-5: Incorrect Baud Rate or Port Selection UX

**What goes wrong:** User selects wrong baud rate (9600 instead of 115200). Connection appears to succeed (serial port opens) but communication produces garbage. User sees garbled text, assumes the controller is broken.

**Prevention:**
1. Default to 115200 baud (GRBL standard)
2. Auto-detect: try connecting at 115200, check for valid GRBL response, fall back to other rates
3. If connection succeeds but no valid GRBL banner is received within 5 seconds, suggest checking baud rate
4. Remember last successful port + baud rate per device serial number if available

**Phase:** Connection Management

---

### MINOR-6: Visualizer Desync with Machine Position

**What goes wrong:** The G-code visualizer shows the toolpath with a position indicator, but it drifts from actual machine position during overrides, feed holds, or errors. Users trust the visualizer position and make decisions based on incorrect information.

**Prevention:**
1. Drive visualizer position from actual machine status reports, not from "lines sent" count
2. Interpolate between status updates using known feed rate and direction
3. Clearly indicate when visualizer position is estimated vs. confirmed
4. On feed hold or error, snap visualizer to last confirmed position

**Phase:** Real-time Feedback / Visualizer

---

## Phase-Specific Warnings

| Phase Topic | Likely Pitfall | Severity | Mitigation |
|---|---|---|---|
| Serial Communication | Thread safety on port writes (MODERATE-2) | HIGH | Route all writes through IO thread. Fix existing `feedHold()`/`cycleStart()` direct write bug |
| Serial Communication | DTR auto-reset (CRITICAL-5) | HIGH | Suppress DTR on open. Add reconnect-without-reset mode |
| Serial Communication | USB disconnect detection (MODERATE-6) | MEDIUM | Check POLLHUP/POLLERR. Count consecutive timeouts |
| Streaming Engine | EEPROM write data loss (CRITICAL-1) | HIGH | Detect EEPROM commands, fall back to send-response |
| Streaming Engine | Buffer desync on error (CRITICAL-2) | HIGH | Pre-validate with $C. Issue soft reset on error during stream |
| Streaming Engine | Line ending / comment stripping (MINOR-1, MINOR-2) | LOW | Pre-process G-code before streaming |
| Streaming Engine | Buffer size variants (MODERATE-3) | MEDIUM | Default 128, detect variant, allow override |
| Real-time Feedback | WCO tracking (MODERATE-5) | MEDIUM | Cache WCO, compute WPos, handle missing WCO gracefully |
| Real-time Feedback | Poll rate (MODERATE-1) | LOW | Already correct at 5Hz. Monitor for UI interpolation needs |
| Safety Features | Software E-stop labeling (CRITICAL-3) | HIGH | Never label software button as E-stop. Document hardware requirement |
| Job Management | Unsafe resume (CRITICAL-4) | HIGH | Position verification, user confirmation, modal state tracking |
| Firmware Settings | No validation/backup (MODERATE-4) | MEDIUM | Range validation, auto-backup, grouped UI |
| Macros | Execution during streaming (MINOR-4) | LOW | Disable macro buttons during stream |
| Jogging | Cancel timing (MINOR-3) | LOW | Use short jog segments, immediate cancel |

---

## Existing Code Issues Identified During Research

These are specific bugs or risks found in the current `/data/DW/src/core/cnc/` implementation:

1. **Thread safety violation in real-time commands:** `feedHold()`, `cycleStart()`, `softReset()`, and all override methods call `m_port.writeByte()` directly from the calling thread (UI thread). The IO thread also calls `m_port.write()` and `m_port.writeByte()`. `SerialPort` has no internal synchronization. This is a data race.

2. **No soft-reset on streaming error:** `processResponse()` handles `error:` by incrementing ack count and error count but does not stop the machine. Buffered lines in GRBL's RX buffer continue executing.

3. **No EEPROM command detection:** The streaming engine has no awareness of EEPROM-writing G-code commands. `G10` in a streamed program will cause data loss with character-counting.

4. **No USB disconnect detection:** `readLine()` returns `nullopt` on timeout but doesn't distinguish between "no data yet" and "device gone." `POLLHUP`/`POLLERR` are not checked. The IO thread will loop indefinitely on a disconnected port.

5. **No DTR suppression:** `SerialPort::open()` does not suppress DTR, causing Arduino reset on every connection.

6. **`readLine()` timing drift:** The `remaining -= 10` approximation for timeout tracking can accumulate significant error over the polling loop, especially when multiple partial reads occur.

7. **`write()` partial write not handled:** `SerialPort::write()` checks if `written == data.size()` but does not retry on partial writes. POSIX `write()` can legally return less than the requested amount.

---

## Sources

- [GRBL Known Issues - gnea/grbl Wiki](https://github.com/gnea/grbl/wiki/Known-Issues)
- [GRBL v1.1 Interface - gnea/grbl Wiki](https://github.com/gnea/grbl/wiki/Grbl-v1.1-Interface)
- [GRBL v1.1 Configuration - gnea/grbl Wiki](https://github.com/gnea/grbl/wiki/Grbl-v1.1-Configuration)
- [GRBL Interfacing Wiki - grbl/grbl](https://github.com/grbl/grbl/wiki/Interfacing-with-Grbl)
- [Buffer Overflow Issue #133 - cncjs/cncjs](https://github.com/cncjs/cncjs/issues/133)
- [Buffer Size Issue #302 - gnea/grbl](https://github.com/gnea/grbl/issues/302)
- [EEPROM Write Issue #165 - gnea/grbl](https://github.com/gnea/grbl/issues/165)
- [Character Counting Issue #74 - LibLaserCut](https://github.com/t-oster/LibLaserCut/issues/74)
- [Serial Transmission Errors Discussion #822 - gnea/grbl](https://github.com/gnea/grbl/issues/822)
- [CNCjs vs UGS - Sienci Forum](https://forum.sienci.com/t/cncjs-versus-ugs/862)
- [Arduino Auto-Reset Disable](https://playground.arduino.cc/Main/DisablingAutoResetOnSerialConnection/)
- [LinuxCNC E-Stop Circuit Discussion](https://forum.linuxcnc.org/27-driver-boards/37025-emergency-stop-circuit)
