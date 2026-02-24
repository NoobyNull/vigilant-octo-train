# Technology Stack: CNC Controller Interface

**Project:** Digital Workshop - CNC Control Suite Milestone
**Researched:** 2026-02-24

## What Already Exists (Inventory)

Before recommending additions, here is what Digital Workshop already has:

| Component | Status | Quality |
|-----------|--------|---------|
| Serial port (POSIX) | Working | Good - raw termios, 8N1, poll-based reads |
| Serial port (Windows) | Stub only | Needs implementation |
| GRBL character-counting streaming | Working | Good - proper buffer tracking with deque |
| Status polling at 5Hz | Working | Good - matches GRBL recommendation |
| Status report parsing | Working | Good - handles MPos, WPos, WCO, FS, Ov |
| Real-time commands | Working | Full set: feed hold, cycle start, soft reset, overrides |
| Feed/rapid/spindle overrides | Working | Correct byte sequences per GRBL spec |
| Jog cancel | Working | 0x85 byte |
| Alarm/error parsing | Working | Human-readable descriptions |
| Callback system | Working | MainThreadQueue-based, thread-safe |
| Machine profiles | Working | GRBL $ settings mapped, JSON serialization |
| Tool database (.vtdb) | Working | Vectric-compatible, full CRUD |
| Feeds & speeds calculator | Working | Janka-based, rigidity derating, power limiting |
| Tool browser panel | Working | Tree view, detail view, calculator integration |

**Assessment:** The low-level communication layer is solid and complete for GRBL serial. What is missing is entirely at the application/integration layer -- not the protocol layer.

## Recommended Stack Additions

### 1. Windows Serial Port (Priority: HIGH)

**Technology:** Win32 CreateFile + DCB + WaitCommEvent
**Why:** The existing POSIX serial code uses raw file descriptors, poll(), termios. The Windows equivalent is CreateFile("COM3",...), SetCommState for DCB config, and overlapped I/O with WaitCommEvent. This matches the existing architecture pattern (no external dependency).

**Do NOT use:** Boost.Asio serial_port -- it would add a massive dependency (Boost) for one small module. The existing POSIX implementation is 270 lines; the Windows equivalent will be similar. libserialport (LGPL) has licensing concerns for a proprietary-friendly codebase.

| Decision | Rationale |
|----------|-----------|
| Hand-rolled Win32 serial | Matches existing POSIX pattern, zero new deps, full control |
| Overlapped I/O | Required for non-blocking reads on Windows, mirrors poll() approach |
| SetupAPI for port enumeration | Standard Windows approach for listing COM ports |

**Confidence:** HIGH -- this is well-documented Win32 API territory with no ambiguity.

### 2. WebSocket Client for FluidNC WiFi (Priority: MEDIUM)

**Technology:** Custom lightweight WebSocket over existing TCP sockets, OR a small header-only library
**Why:** FluidNC (ESP32-based) exposes a WebSocket on port 81 (configurable). The protocol is identical to serial -- same line-oriented gcode, same realtime characters, same responses. A WebSocket transport would let Digital Workshop control WiFi-connected machines.

**Recommended approach:** Abstract the existing `SerialPort` into a `Transport` interface:

```cpp
class Transport {
public:
    virtual ~Transport() = default;
    virtual bool write(const std::string& data) = 0;
    virtual bool writeByte(u8 byte) = 0;
    virtual std::optional<std::string> readLine(int timeoutMs) = 0;
    virtual void drain() = 0;
    virtual bool isOpen() const = 0;
};
```

Then implement `SerialTransport` (wrapping existing SerialPort) and `WebSocketTransport` for FluidNC. The CncController already only calls write/writeByte/readLine/drain on SerialPort, so this refactor is surgical.

For the WebSocket implementation itself: use a minimal single-header library like [IXWebSocket](https://github.com/machinezone/IXWebSocket) or implement the RFC 6455 framing directly (FluidNC only uses text frames, no extensions). Given the simplicity of the FluidNC protocol, a 200-line custom implementation is feasible.

**Confidence:** MEDIUM -- FluidNC WebSocket is well-documented, but the transport abstraction needs careful testing with real hardware. Defer to Phase 2+ of this milestone.

### 3. Job State Persistence (Priority: HIGH)

**Technology:** SQLite3 (already in stack) + JSON (nlohmann/json already in stack)
**Why:** Safe resume after pause/power-loss requires persisting:
- Current G-code file path + hash
- Last acknowledged line number
- Machine state at pause (position, spindle, feed, coolant)
- Active G-code modal state (G90/G91, G20/G21, coordinate system, etc.)
- Work coordinate offsets

This maps naturally to a `job_state` table in the existing SQLite database. The JSON library handles modal state serialization.

**No new dependencies needed.** The existing database infrastructure (ConnectionPool, StatementHelper) handles this directly.

**Confidence:** HIGH -- straightforward use of existing infrastructure.

### 4. G-code Modal State Tracker (Priority: HIGH)

**Technology:** Custom C++ class, no external deps
**Why:** To resume a job mid-program, you must rebuild the machine's modal state (which G90/G91 is active, what coordinate system, what feed rate, etc.) by scanning all G-code lines before the resume point. This is a pure parser that already has a foundation in `gcode_parser.h`.

The tracker maintains:
- Motion mode (G0/G1/G2/G3)
- Distance mode (G90/G91)
- Feed rate mode (G93/G94)
- Units (G20/G21)
- Plane selection (G17/G18/G19)
- Coordinate system (G54-G59)
- Tool length offset (G43/G49)
- Spindle state (M3/M4/M5, S value)
- Coolant state (M7/M8/M9)
- Feed rate (F value)
- Tool number (T value)

This state tracker generates a "preamble" block of G-code commands that restores the machine to the exact modal state at the resume point.

**Confidence:** HIGH -- well-understood G-code semantics, no ambiguity.

### 5. Macro Engine (Priority: MEDIUM)

**Technology:** Stored G-code sequences in SQLite + simple variable substitution
**Why:** GRBL does not support macros natively. All CNC senders (UGS, CNCjs, bCNC, Candle) implement macros at the sender level by intercepting commands like M6 and injecting stored G-code sequences.

The macro system should support:
- **Named macros:** "Homing", "Tool Change", "Z Probe", "Park", custom user macros
- **Variable substitution:** `{tool_change_x}`, `{tool_change_y}`, `{probe_z_feed}`, `{safe_z}`
- **M6 interception:** When streaming encounters M6, pause stream, execute tool-change macro, resume
- **Button-bound macros:** Assign macros to UI buttons (like bCNC's button bar)

Storage: `macros` table in SQLite with columns (id, name, category, gcode_text, variables_json, sort_order). Variables are JSON objects with default values that users can override per machine profile.

**Do NOT build:** A scripting language (Lua, Python). The complexity is not justified for G-code sequence injection. bCNC uses Python macros but that is because bCNC IS a Python app. For a C++ app, stored G-code with variable substitution covers 95% of use cases.

**Confidence:** HIGH -- this is how every major sender implements macros.

### 6. Firmware Settings Interface (Priority: MEDIUM)

**Technology:** GRBL $$ parser + FluidNC YAML parser (both custom, no deps)
**Why:** Reading/writing firmware settings requires:
- **GRBL classic:** Send `$$` to get all settings, parse `$N=V` responses, send `$N=V` to change
- **grblHAL:** Same protocol but more settings (extended $ numbers)
- **FluidNC:** Send `$CD` to dump YAML config, parse YAML tree, send `$path/to/setting=value`

For GRBL $$, the parser is trivial (regex or sscanf on `$(\d+)=(.+)`). For FluidNC YAML, use a minimal YAML parser -- the config is flat enough that a simple key-path extractor works without a full YAML library. Alternatively, FluidNC's `$cmd` interface uses simple `$path=value` syntax that does not require YAML parsing at all.

**Confidence:** MEDIUM -- GRBL $$ is trivial and HIGH confidence. FluidNC YAML parsing is more nuanced; the `$cmd` interface may be sufficient to avoid full YAML.

## What NOT to Add

| Technology | Why Skip |
|------------|----------|
| Boost (any part) | Massive dependency for marginal gains; C++17 stdlib covers needs |
| libserialport | LGPL licensing, existing custom serial works well |
| Full YAML parser (yaml-cpp) | FluidNC `$cmd` interface avoids need; keep deps minimal |
| Lua/Python scripting | Over-engineered for macro use case; stored G-code suffices |
| gRPC/Protobuf | No CNC firmware uses these protocols |
| USB HID library | CNC controllers use serial-over-USB (CDC ACM), not HID |
| Qt Serial Port | Wrong framework (app uses SDL2 + ImGui) |

## Communication Protocol Summary

### How CNC Senders Handle GRBL Communication

Based on analysis of UGS (Java), CNCjs (Node.js), bCNC (Python), and Candle (C++/Qt):

| Aspect | Industry Standard | DW Status |
|--------|-------------------|-----------|
| Serial protocol | 115200 baud, 8N1, no flow control | Done |
| Streaming | Character-counting (preferred) or send-response | Done (char counting) |
| Status polling | 5Hz recommended, 10Hz max | Done (5Hz) |
| Real-time commands | Single-byte injection bypassing buffer | Done |
| Overrides | Extended ASCII bytes per GRBL spec | Done |
| Connection detection | Wait for banner OR probe with `?` | Done (both) |
| Firmware variants | GRBL, grblHAL, FluidNC, Smoothieware | Partial (GRBL + FluidNC detection) |
| WiFi/WebSocket | FluidNC port 81, same protocol as serial | Not yet |
| M6 tool change | Sender-side macro injection, not firmware | Not yet |
| Job resume | Sender tracks line number + modal state | Not yet |
| Macro system | Stored G-code with variable substitution | Not yet |
| Settings modification | `$$` read, `$N=V` write | Not yet |

### Status Report Refresh Rates

The GRBL v1.1 interface documentation recommends:
- **5Hz** for normal operation (200ms interval) -- what DW already uses
- **10Hz** maximum before diminishing returns and CPU taxation on the controller
- Status response latency is 5-20ms after `?` is sent

UGS uses ~5Hz. CNCjs had issues with automatic polling (some versions required manual `?`). Candle uses ~10Hz. bCNC uses ~5Hz.

**Recommendation:** Keep 5Hz as default. Add a user-configurable option (1-10Hz) for users who want faster updates during precision work. The IO thread already handles this cleanly.

### Safe Resume Implementation

Based on analysis of UGS, LightBurn, Duet3D, and community implementations:

**The sender is responsible for resume, not the firmware.** GRBL has no concept of "resume from line N." The sender must:

1. **Track state continuously:** Every acknowledged line updates the sender's modal state tracker
2. **On pause (feed hold):** Record current line number, position, and full modal state
3. **On resume from pause:** Send `~` cycle start -- GRBL handles this natively if no position change occurred
4. **On resume after abort/power-loss:**
   a. Re-home the machine (critical for position reference)
   b. Scan G-code from line 0 to resume point, building modal state without executing motion
   c. Generate a preamble: set coordinate system, units, distance mode, spindle, coolant, feed rate
   d. Rapid to the XY position of the resume point at safe Z
   e. Plunge to Z position
   f. Resume streaming from the interrupted line

**Critical safety note:** Resuming after power loss is inherently dangerous. The machine has lost its position reference. The safe approach requires re-homing and user confirmation before any motion. This is NOT a "just works" feature -- it is an assisted manual recovery.

**Confidence:** HIGH -- this is the universal approach across all senders. The modal state tracker is the key component.

## Installation / Build Impact

```bash
# No new external dependencies required for this milestone.
# All additions use existing stack:
# - C++17 stdlib (threading, filesystem, optional, variant)
# - SQLite3 (job state persistence, macros)
# - nlohmann/json (settings serialization, macro variables)
# - Dear ImGui (UI panels)
# - SDL2 + OpenGL 3.3 (rendering)

# Windows serial port implementation uses Win32 API (already available):
# - CreateFile, SetCommState, WaitCommEvent, SetupDiGetClassDevs

# Optional future: WebSocket for FluidNC
# Candidate: IXWebSocket (header-only, MIT license, CMake FetchContent)
# Or: hand-rolled RFC 6455 framing (~200 lines for text-only frames)
```

## Architecture Pattern: Transport Abstraction

The single most impactful architectural change for this milestone:

```cpp
// Abstract transport layer
class Transport {
public:
    virtual ~Transport() = default;
    virtual bool open(const std::string& address, int param) = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;
    virtual bool write(const std::string& data) = 0;
    virtual bool writeByte(u8 byte) = 0;
    virtual std::optional<std::string> readLine(int timeoutMs) = 0;
    virtual void drain() = 0;
};

// Implementations
class SerialTransport : public Transport { /* wraps existing SerialPort */ };
class WebSocketTransport : public Transport { /* FluidNC WiFi */ };
// Future: class TCPTransport : public Transport { /* raw TCP */ };
```

CncController changes from holding `SerialPort m_port` to `std::unique_ptr<Transport> m_transport`. The rest of the controller logic remains unchanged because it already only uses the 4 methods that map to the Transport interface.

## Sources

- [GRBL v1.1 Interface Documentation](https://github.com/gnea/grbl/wiki/Grbl-v1.1-Interface) -- HIGH confidence, authoritative
- [GRBL v1.1 Commands](https://github.com/gnea/grbl/wiki/Grbl-v1.1-Commands) -- HIGH confidence
- [GRBL v1.1 Configuration](https://github.com/gnea/grbl/wiki/Grbl-v1.1-Configuration) -- HIGH confidence
- [Universal G-Code Sender (UGS)](https://github.com/winder/Universal-G-Code-Sender) -- HIGH confidence, reference implementation
- [CNCjs](https://github.com/cncjs/cncjs) -- MEDIUM confidence (status polling had known issues)
- [Candle](https://github.com/Denvi/Candle) -- MEDIUM confidence (C++/Qt reference)
- [FluidNC WebSocket](http://wiki.fluidnc.com/en/support/interface/websockets) -- MEDIUM confidence (wiki was unreachable, used GitHub issues)
- [FluidNC GCode Senders compatibility](http://wiki.fluidnc.com/en/support/gcode_senders) -- MEDIUM confidence
- [bCNC Tool Change](https://github.com/vlachoudis/bCNC/wiki/Tool-Change) -- HIGH confidence for macro patterns
- [grblHAL Core](https://github.com/grblHAL/core) -- MEDIUM confidence (growing ecosystem)
- [GRBL Status Polling Frequency](https://github.com/fra589/grbl-Mega-5X/issues/45) -- MEDIUM confidence
