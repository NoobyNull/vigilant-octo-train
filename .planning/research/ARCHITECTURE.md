# Architecture Patterns: CNC Controller Application

**Domain:** CNC controller/sender desktop application (GRBL/FluidNC/grblHAL)
**Researched:** 2026-02-24
**Overall confidence:** HIGH (based on existing codebase analysis + established open-source patterns from UGS, Candle, CNCjs, LinuxCNC)

## Recommended Architecture

Digital Workshop already has a solid foundation. The CNC control features should be built as **separate, focused panels** that communicate through the existing `CncController` + `MainThreadQueue` + callback wiring pattern established in `application_wiring.cpp`. This matches the approach used by Universal G-Code Sender (UGS), which structures each concern as an independent panel/module, and avoids the monolithic single-form anti-pattern seen in Candle's `frmmain.cpp` (6000+ lines).

### Current Architecture (What Exists)

```
Application
  |-- CncController (dedicated IO thread, character-counting streaming)
  |     |-- SerialPort (POSIX /dev/ttyUSB*, /dev/ttyACM*)
  |     |-- MachineStatus (parsed from GRBL `?` responses at 5Hz)
  |     |-- StreamProgress (line-level ack tracking)
  |     `-- CncCallbacks -> MainThreadQueue -> UI callbacks
  |
  |-- ToolDatabase (Vectric .vtdb SQLite, full CRUD)
  |-- ToolCalculator (pure function: CalcInput -> CalcResult)
  |-- MaterialManager (Janka hardness classification)
  |
  |-- GCodePanel (dual-mode: View + Send, already has)
  |     |-- 3D path visualization (OpenGL framebuffer)
  |     |-- Simulation playback (trapezoidal planner)
  |     |-- Connection bar (port selection, connect/disconnect)
  |     |-- Machine status display (state, position, feed, spindle)
  |     |-- Playback controls (start/stop/hold/resume)
  |     |-- Feed override slider
  |     |-- Console (raw GRBL line log)
  |     `-- MachineProfileDialog (kinematic presets)
  |
  `-- ToolBrowserPanel (tree view, detail editor, calculator, machine editor)
```

### Recommended Target Architecture

```
Application
  |
  |-- Core Layer (no UI dependencies)
  |     |-- CncController (existing - IO thread, streaming, status polling)
  |     |-- SerialPort (existing - POSIX wrapper)
  |     |-- GrblSettingsManager (NEW - read/write/cache $$ settings)
  |     |-- MacroStorage (NEW - named G-code snippets, SQLite-backed)
  |     |-- JobStateModel (NEW - formal state machine with transitions)
  |     |-- WorkCoordinateManager (NEW - WCS offset tracking, G54-G59)
  |     |-- ToolDatabase (existing)
  |     |-- ToolCalculator (existing)
  |     `-- MaterialManager (existing)
  |
  |-- UI Layer (ImGui panels, all render on main thread)
  |     |-- GCodePanel (existing - KEEP as-is, already well-structured)
  |     |-- ToolBrowserPanel (existing - KEEP as-is)
  |     |-- JogPanel (NEW - directional jog, step sizes, keyboard shortcuts)
  |     |-- GrblSettingsPanel (NEW - firmware $$ settings editor)
  |     |-- MacroPanel (NEW - quick-run macro buttons)
  |     |-- ProbePanel (NEW - probe routines for Z zero, corner finding)
  |     `-- StatusBar integration (existing - extend with CNC state indicator)
  |
  `-- Wiring Layer (application_wiring.cpp)
        |-- CncCallbacks -> all CNC panels
        |-- ToolBrowserPanel selection -> GCodePanel tool info
        `-- MacroPanel execute -> CncController command injection
```

### Component Boundaries

| Component | Responsibility | Communicates With |
|-----------|---------------|-------------------|
| `CncController` | Serial I/O, GRBL protocol, streaming, status polling | `SerialPort`, `MainThreadQueue` (outbound callbacks) |
| `GCodePanel` | G-code viewing, simulation, job streaming UI, console | `CncController` (via callbacks), `GCodeRepository`, `ToolDatabase` |
| `ToolBrowserPanel` | Tool selection, editing, feeds/speeds calculation | `ToolDatabase`, `MaterialManager`, `ToolCalculator` |
| `JogPanel` (new) | Manual machine movement, step/continuous jog | `CncController` (jog commands), `JobStateModel` (guards) |
| `GrblSettingsPanel` (new) | Read/display/edit GRBL $$ settings | `GrblSettingsManager`, `CncController` (send $$ and $x=val) |
| `MacroPanel` (new) | Store/edit/execute named G-code snippets | `MacroStorage`, `CncController` (command injection) |
| `ProbePanel` (new) | Guided probe routines (Z zero, edge finding) | `CncController` (G38.x commands), `WorkCoordinateManager` |
| `JobStateModel` (new) | Formal state machine: Idle/Run/Hold/Alarm/Check | `CncController` (status updates drive transitions) |
| `GrblSettingsManager` (new) | Cache firmware settings, validate before write | `CncController` (send $$ to read, $x=val to write) |
| `MacroStorage` (new) | Persist named macros to SQLite | App database (or separate file) |
| `WorkCoordinateManager` (new) | Track G54-G59 offsets, WCS selection | `CncController` ($# command to read offsets) |

### Data Flow

#### 1. Tool Selection -> Calculator -> Controller (existing, works well)

```
ToolBrowserPanel
  |-- User selects tool geometry from tree
  |-- User selects material (from MaterialManager)
  |-- User selects machine (from ToolDatabase)
  |
  v
ToolCalculator::calculate(CalcInput) -> CalcResult
  |-- RPM, feed rate, plunge rate, stepdown, stepover
  |-- Power-limited flag if spindle can't handle it
  |-- Rigidity derating applied for drive type
  |
  v
CalcResult displayed in ToolBrowserPanel::renderCalculator()
  |
  v (future: "Apply to G-code" button)
  GCodePanel receives suggested feed/speed for override
```

This pipeline is already clean. The `CalcInput` struct cleanly separates tool geometry, material hardness band, and machine parameters. No changes needed.

#### 2. G-code Load -> Analyze -> Stream -> Controller (existing, works well)

```
User loads .nc file
  |
  v
gcode::Parser::parse() -> gcode::Program (commands + path segments)
  |
  v
gcode::Analyzer::analyze(Program, MachineProfile) -> Statistics
  |-- Total/cutting/rapid path lengths
  |-- Estimated time (trapezoidal motion planner)
  |-- Per-segment times for simulation
  |
  v
GCodePanel renders stats + 3D path view
  |
  v (user clicks "Send" in Send mode)
GCodePanel::buildSendProgram() -> vector<string>
  |
  v
CncController::startStream(lines)
  |-- IO thread: character-counting send loop
  |-- 5Hz status polling
  |-- Callbacks via MainThreadQueue:
  |     onStatusUpdate -> GCodePanel::onGrblStatus()
  |     onLineAcked -> GCodePanel::onGrblLineAcked()
  |     onProgressUpdate -> GCodePanel::onGrblProgress()
  |     onAlarm -> GCodePanel::onGrblAlarm()
  |     onRawLine -> GCodePanel::onGrblRawLine()
  v
GCodePanel updates progress bar, console, DRO, simulation overlay
```

This is already well-implemented with proper thread safety.

#### 3. IO Thread <-> UI Thread Communication (existing pattern, extend)

The current pattern is correct and should be the template for all new CNC panels:

```
IO Thread (CncController::ioThreadFunc)
  |-- Reads serial responses
  |-- Parses status reports, ok/error acks, alarm messages
  |-- Dispatches via m_mtq->enqueue([callback, data]() { callback(data); })
  |
  v
MainThreadQueue::processAll() (called every frame in Application::update)
  |-- Executes all queued lambdas on the main thread
  |-- Safe to touch ImGui state, OpenGL, UI state
  |
  v
Panel callback methods (e.g., onGrblStatus, onGrblLineAcked)
  |-- Update panel member variables
  |-- Next render() call picks up new state
```

**Key design constraint:** All UI state mutations happen on the main thread. The IO thread never directly modifies panel state. The `MainThreadQueue` bounded FIFO (1000 slots) with blocking enqueue prevents unbounded growth while the condition variable prevents busy-waiting.

For new panels, the wiring follows the same pattern in `application_wiring.cpp`:
1. Create the panel in UIManager
2. Inject dependencies via setters (setCncController, etc.)
3. Wire CncCallbacks to panel methods
4. Panel methods store state; render() reads it

#### 4. Firmware Settings Read/Write (new)

```
User opens GrblSettingsPanel
  |
  v
GrblSettingsManager::requestSettings()
  |-- Sends "$$\n" via CncController (or raw serial write)
  |-- IO thread receives lines like "$0=10", "$1=25", ...
  |-- Parses into map<int, GrblSetting> with value + description
  |-- Dispatches to main thread via callback
  |
  v
GrblSettingsPanel renders table:
  | Setting | Value | Description | [Edit] |
  |---------|-------|-------------|--------|
  | $0      | 10    | Step pulse (us) | [...] |
  | $100    | 800   | X steps/mm  | [...] |
  |
  v (user edits a value and confirms)
GrblSettingsManager::writeSetting(settingNum, newValue)
  |-- Validates range/type
  |-- Sends "$100=800\n" via CncController
  |-- Waits for "ok" ack
  |-- Re-reads $$ to confirm
```

**Implementation note:** GRBL's $$ output is a series of `$N=V` lines terminated by "ok". The existing `processResponse()` method handles "ok" and raw `[MSG:]` lines, but does not currently collect `$N=V` lines. The `GrblSettingsManager` needs a response collector callback wired into the raw line handler, or a dedicated command/response mode added to `CncController`.

The cleanest approach: add an `onSettingLine` callback to `CncCallbacks`, or use `onRawLine` and filter for `$N=` prefix in the settings manager.

#### 5. Macro Execution (new)

```
MacroStorage (SQLite table: id, name, category, gcode_text, sort_order)
  |
  v
MacroPanel renders grid of macro buttons:
  | [Home All]  [Probe Z]  [Set WCS Zero] |
  | [Park]      [Warm Up]  [Tool Change]  |
  |
  v (user clicks a macro button)
MacroPanel checks JobStateModel:
  |-- If Idle or Hold: safe to send
  |-- If Run: warn (macro would interrupt stream)
  |-- If Alarm: only allow $X, $H, reset
  |
  v
CncController sends macro lines
  |-- Option A: inject into stream queue (if streaming)
  |-- Option B: send directly as one-shot lines (if idle)
```

**For GRBL specifically:** Macros are just G-code snippets sent line by line. There is no Fanuc-style G65 macro call or persistent variable system. Macros live entirely in the sender software (Digital Workshop), not on the controller.

Common CNC sender macros:
- **Home All:** `$H`
- **Probe Z:** `G38.2 Z-50 F100` (probe down), then `G92 Z0` (set zero)
- **Park:** `G53 G0 Z0` (raise to max Z), `G53 G0 X0 Y0` (go to origin)
- **Set WCS Zero:** `G10 L20 P1 X0 Y0 Z0` (set current position as G54 zero)
- **Tool Change Position:** `G53 G0 Z0` then `G53 G0 X-10 Y-10`

## Job State Machine (new component)

The current code has `MachineState` enum and tracks it via status polling, but there is no formal state machine with guarded transitions. This is needed to prevent dangerous operations (e.g., jogging during a stream, sending macros during alarm without unlock).

### State Diagram

```
                    connect()
  [Disconnected] ----------------> [Connected:Idle]
       ^                                |
       |  disconnect()                  |
       +--------------------------------+
                                        |
                        startStream()   |   $H (home)
                    +-------------------+-------------------+
                    |                                       |
                    v                                       v
              [Connected:Run]                      [Connected:Home]
                    |                                       |
         feedHold() |  stream complete                      | home complete
                    v         |                             |
              [Connected:Hold]|                             |
                    |         |                             |
        cycleStart()|         v                             v
                    +-->[Connected:Idle] <------------------+
                              |
                  alarm       |
                    +-------- | --------+
                    v                   |
              [Connected:Alarm]         |
                    |                   |
            $X unlock                   |
                    +-------------------+
```

### Guard Rules

| Current State | Allowed Actions | Blocked Actions |
|---------------|----------------|-----------------|
| Disconnected | connect | everything else |
| Idle | stream, jog, macro, probe, settings edit, home | feed hold, cycle start |
| Run | feed hold, override adjust, status query | jog, stream start, settings edit |
| Hold | cycle start, soft reset, override adjust | jog, stream start, settings edit |
| Alarm | soft reset, unlock ($X), home ($H) | jog, stream, macro, settings edit |
| Home | status query | everything else (wait for completion) |
| Jog | jog cancel, feed hold | stream start |

### Implementation

```cpp
class JobStateModel {
public:
    // Derived from MachineStatus.state + connection state + streaming state
    enum class JobState {
        Disconnected,
        Idle,
        Running,
        Held,
        Alarm,
        Homing,
        Jogging,
        Checking,  // GRBL $C check mode
        Door,
        Sleep
    };

    // Update from CncController status
    void update(bool connected, bool streaming, MachineState grblState);

    // Guard queries (panels call these before enabling buttons)
    bool canJog() const;
    bool canStartStream() const;
    bool canEditSettings() const;
    bool canRunMacro() const;
    bool canProbe() const;
    bool canHome() const;

    JobState state() const { return m_state; }

private:
    JobState m_state = JobState::Disconnected;
};
```

This is a **read-only derived model**, not an event-driven state machine. It computes its state from the CncController's status + connection + streaming flags each time a status update arrives. This is simpler and less error-prone than trying to maintain separate state that could drift from the controller's actual state.

## Patterns to Follow

### Pattern 1: Separate Panels with Dependency Injection

**What:** Each CNC concern gets its own ImGui Panel subclass. Dependencies injected via setters in `application_wiring.cpp`.

**Why:** This is the established pattern in the codebase (GCodePanel, ToolBrowserPanel, CostPanel, etc.). It avoids a god-object panel.

**Example:**
```cpp
class JogPanel : public Panel {
public:
    JogPanel();
    void render() override;

    void setCncController(CncController* ctrl) { m_cnc = ctrl; }
    void setJobStateModel(JobStateModel* model) { m_jobState = model; }

    // Callback from CncController status updates
    void onStatusUpdate(const MachineStatus& status);

private:
    void renderJogButtons();
    void renderStepSizeSelector();
    void renderKeyboardHints();
    void sendJog(float x, float y, float z);

    CncController* m_cnc = nullptr;
    JobStateModel* m_jobState = nullptr;
    MachineStatus m_status;
    float m_stepSize = 1.0f;  // mm
    float m_feedRate = 1000.0f; // mm/min
};
```

### Pattern 2: Callback-Based IO Thread Dispatch

**What:** IO thread never touches UI state. All state changes dispatched via `MainThreadQueue::enqueue()` with captured lambdas.

**Why:** This is already how `CncController` works. It is correct and thread-safe.

**When:** Any time a background thread needs to update UI state.

**Example (from existing code):**
```cpp
// In IO thread:
m_mtq->enqueue([cb = m_callbacks.onStatusUpdate, st = m_lastStatus]() {
    cb(st);  // Executes on main thread
});
```

### Pattern 3: Guard Before Act

**What:** Before sending any command to the controller, check `JobStateModel` for permission.

**Why:** Prevents dangerous operations like jogging during a stream or sending G-code during alarm lock.

**Example:**
```cpp
void JogPanel::sendJog(float x, float y, float z) {
    if (!m_jobState || !m_jobState->canJog()) return;
    if (!m_cnc || !m_cnc->isConnected()) return;

    char cmd[128];
    snprintf(cmd, sizeof(cmd), "$J=G91 G21 X%.3f Y%.3f Z%.3f F%.0f",
             x, y, z, m_feedRate);
    // CncController needs a sendCommand() method for one-shot lines
    m_cnc->sendCommand(cmd);
}
```

### Pattern 4: Settings as Cached Snapshot

**What:** Read firmware settings once on connect (or on demand), cache in memory, present in UI. Only write back individual changed values.

**Why:** Reading $$ is cheap but blocking. Caching avoids repeated reads. Writing individual `$N=V` lines with confirmation prevents partial updates.

**When:** GrblSettingsPanel, MachineProfile auto-detection.

## Anti-Patterns to Avoid

### Anti-Pattern 1: Monolithic CNC Panel

**What:** Putting jog controls, settings editor, macro buttons, probe routines, console, DRO, and path view all in one panel.

**Why bad:** Candle does this with `frmmain.cpp` at 6000+ lines. It becomes unmaintainable. Different CNC tasks have different screen real estate needs.

**Instead:** Separate panels that can be docked/undocked independently via ImGui docking. The GCodePanel is already the right size for its responsibilities.

### Anti-Pattern 2: Polling UI State from IO Thread

**What:** Having the IO thread read UI variables (like "is the user pressing jog?") directly.

**Why bad:** Data races, undefined behavior, hard-to-debug timing bugs.

**Instead:** UI posts commands to CncController methods (which are thread-safe because they write to atomic flags or mutex-protected queues). IO thread reads from those safe interfaces.

### Anti-Pattern 3: Blocking Serial Reads on Main Thread

**What:** Calling `SerialPort::readLine()` from the render loop.

**Why bad:** Serial reads can block up to the timeout (20ms in current code). At 60fps that is the entire frame budget. UI freezes.

**Instead:** All serial I/O stays on the dedicated IO thread (already correctly implemented).

### Anti-Pattern 4: Stateless Command Dispatch

**What:** Sending any command to the controller without checking machine state first.

**Why bad:** Sending jog commands during alarm state causes `error:9` (G-code locked). Sending `$$` during streaming corrupts the program. Users could damage workpieces.

**Instead:** Use `JobStateModel` guards (Pattern 3).

### Anti-Pattern 5: Separate IO Thread Per Panel

**What:** Each panel opening its own serial connection or spawning its own thread.

**Why bad:** Serial ports are exclusive resources. Multiple threads reading the same port causes interleaved/corrupt data.

**Instead:** Single `CncController` with single IO thread, shared by all panels via dependency injection. Already correctly implemented.

## Panel Structure Recommendation

**Verdict: Separate panels, not unified.** Here's why:

1. **ImGui docking** allows users to arrange panels however they like. A woodworker setting up a job wants DRO + jog prominently. During a cut, they want progress + console + override. These are different layouts for different tasks.

2. **The existing pattern works.** GCodePanel is already a well-bounded ~190-line header with clear subsections. Adding jog, settings, macros, and probing to it would triple its size.

3. **UGS (the most mature open-source sender) uses separate modules.** Each feature (DRO, jog, overrides, probe, macro) is an independent panel/plugin.

### Panel Inventory (Build Order)

| Priority | Panel | Justification |
|----------|-------|---------------|
| 1 (exists) | GCodePanel | Already has View + Send modes, connection, streaming, console |
| 2 (exists) | ToolBrowserPanel | Already has tool tree, calculator, machine editor |
| 3 (new) | JogPanel | Essential for machine setup before any job. Small, focused |
| 4 (new) | GrblSettingsPanel | Needed for machine configuration. Read $$ -> table -> edit |
| 5 (new) | MacroPanel | Quality-of-life. Grid of user-defined quick-run buttons |
| 6 (new) | ProbePanel | Advanced feature. Guided Z-probe, edge-finding routines |

### CncController Extensions Needed

The existing `CncController` needs minor additions to support the new panels:

```cpp
// New method: send a single command line (for jog, macros, settings)
void sendCommand(const std::string& line);

// New method: send a real-time command byte (already exists for overrides)
// void sendRealtime(u8 byte);  // Already covered by feedHold(), etc.

// New callback: for $$ settings lines
// Option A: Add to CncCallbacks
std::function<void(int settingNum, const std::string& value)> onSettingReceived;
// Option B: Use onRawLine and filter in GrblSettingsManager (simpler)
```

The `sendCommand()` method should:
- Be callable from any thread (or require main thread + post to IO thread)
- Respect the character-counting buffer if streaming is active
- Log via onRawLine callback

## GrblSettingsManager Design

```cpp
struct GrblSetting {
    int number;
    std::string value;
    std::string description;  // From built-in lookup table
    std::string units;        // "us", "mm", "mm/min", etc.
    enum Type { Integer, Float, Boolean, Bitmask } type;
    float min, max;           // Valid range
};

class GrblSettingsManager {
public:
    // Request settings read (sends $$, collects responses)
    void requestRead();

    // Write a single setting (validates, sends $N=V, waits for ok)
    bool writeSetting(int number, const std::string& value);

    // Get cached settings (populated after requestRead completes)
    const std::map<int, GrblSetting>& settings() const;

    // Feed raw lines from CncController to collect $N=V responses
    void onRawLine(const std::string& line);

    // Callback when settings read is complete
    std::function<void()> onSettingsLoaded;

private:
    std::map<int, GrblSetting> m_settings;
    bool m_reading = false;

    // Built-in descriptions for GRBL 1.1 settings
    static const std::map<int, GrblSettingMeta> s_settingMeta;
};
```

## MacroStorage Design

```cpp
struct Macro {
    int64_t id = -1;
    std::string name;          // "Probe Z", "Park", "Home All"
    std::string category;      // "Setup", "Probing", "Custom"
    std::string gcode;         // Multi-line G-code text
    std::string icon;          // FontAwesome icon code (optional)
    int sortOrder = 0;
    bool builtIn = false;      // Prevent deletion of defaults
};

class MacroStorage {
public:
    bool open(Database& db);  // Creates macro table if needed
    std::vector<Macro> getAll();
    std::vector<Macro> getByCategory(const std::string& category);
    bool insert(Macro& m);
    bool update(const Macro& m);
    bool remove(int64_t id);

    // Seed default macros on first run
    void seedDefaults();
};
```

Default macros to seed:
- **Home All** (`$H`) - Setup category
- **Unlock** (`$X`) - Setup category
- **Probe Z** (`G38.2 Z-25 F100\nG92 Z0\nG0 Z5`) - Probing category
- **Set XY Zero** (`G10 L20 P1 X0 Y0`) - Setup category
- **Park** (`G53 G0 Z0\nG53 G0 X0 Y0`) - Movement category
- **Return to XY Zero** (`G0 X0 Y0`) - Movement category

## Scalability Considerations

| Concern | Current (1 machine) | Future (multi-machine) |
|---------|---------------------|----------------------|
| Serial connections | Single CncController | Map of CncController per port. Each with own IO thread. |
| Settings cache | Single GrblSettingsManager | One per controller instance |
| Job queue | Single stream at a time | One active job per controller |
| UI panels | All panels share one controller | Panel targets selectable controller |
| Database | Single .vtdb, single app.db | No change needed (tools are machine-agnostic) |

Multi-machine is not needed now but the architecture naturally supports it because `CncController` is already a self-contained instance with its own thread. The panels receive a pointer, so swapping which controller they point at is trivial.

## Sources

- [GRBL v1.1 Interface Protocol](https://github.com/gnea/grbl/wiki/Grbl-v1.1-Interface) - HIGH confidence (official GRBL wiki)
- [GRBL v1.1 Configuration / Settings](https://github.com/gnea/grbl/wiki/Grbl-v1.1-Configuration) - HIGH confidence (official GRBL wiki)
- [Universal G-Code Sender (UGS)](https://github.com/winder/Universal-G-Code-Sender) - HIGH confidence (most mature open-source sender, modular architecture)
- [Candle GRBL Controller](https://github.com/Denvi/Candle) - MEDIUM confidence (Qt/C++ reference, but monolithic anti-pattern)
- [CNCjs](https://github.com/cncjs/cncjs) - MEDIUM confidence (Node.js/browser-based, different tech but good UX patterns)
- [LinuxCNC QtDragon GUI](https://linuxcnc.org/docs/html/gui/qtdragon.html) - MEDIUM confidence (industrial-grade reference for panel layout)
- [GRBL Machine Status Panel](https://zeevy.github.io/grblcontroller/machine-status-panel.html) - MEDIUM confidence (documents state transitions)
- [CNC Control Panel Design Best Practices](https://radonix.com/machine-control-panel-design-operation/) - LOW confidence (commercial marketing, but useful for HMI patterns)
- Existing Digital Workshop codebase analysis - HIGH confidence (direct source code review)
