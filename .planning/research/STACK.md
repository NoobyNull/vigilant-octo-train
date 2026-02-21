# Stack Research: Digital Workshop Milestone

**Project:** Digital Workshop - CNC Woodworking Desktop App
**Domain:** Additional capabilities for existing C++17/SDL2/ImGui desktop app
**Researched:** 2026-02-08
**Confidence:** HIGH

## Context

This research covers NEW libraries needed to add CNC control, business workflow, and infrastructure capabilities to an existing C++17 desktop app. The existing stack (SDL2, Dear ImGui, OpenGL 3.3, SQLite3, CMake, zlib, GLM, GoogleTest) is locked and not re-evaluated here.

**Project constraints:**
- C++17 (no C++20/23 features)
- Permissive licenses only (MIT, BSD, zlib, Public Domain)
- CMake FetchContent integration (no external package managers)
- Under 25MB deployed size
- Cross-platform (Linux, Windows, macOS)

## Recommended Stack

### HTTP/REST Communication

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| cpp-httplib | v0.31.0 | HTTP/HTTPS REST client for ncSender communication | Header-only, MIT license, zero dependencies, proven stability. Latest release Feb 9, 2025. Supports SSL via OpenSSL/mbedTLS. Perfect for REST API calls to ncSender service. |

**Confidence:** HIGH (verified from [official GitHub releases](https://github.com/yhirose/cpp-httplib))

**Integration:**
```cmake
FetchContent_Declare(httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.31.0
)
FetchContent_MakeAvailable(httplib)
target_link_libraries(your_target PRIVATE httplib::httplib)
```

**Notes:**
- Header-only by default (include `httplib.h`)
- Optional compiled mode for faster builds
- Built-in JSON parsing not included; pair with nlohmann/json
- SSL support requires OpenSSL or mbedTLS

### WebSocket Communication

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| IXWebSocket | v11.4.6 | WebSocket client for real-time ncSender events | BSD-3-Clause license, minimal dependencies (no Boost), excellent cross-platform support, actively maintained (May 2025 release). Simpler than WebSocket++. |

**Confidence:** HIGH (verified from [official GitHub](https://github.com/machinezone/IXWebSocket))

**Alternative:** WebSocket++ v0.8.2 (last updated April 2020, less active)

**Integration:**
```cmake
FetchContent_Declare(ixwebsocket
  GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
  GIT_TAG v11.4.6
)
FetchContent_MakeAvailable(ixwebsocket)
target_link_libraries(your_target PRIVATE ixwebsocket::ixwebsocket)
```

**Notes:**
- C++11 compatible (works with C++17)
- Supports deflate compression
- Multiple SSL backends (OpenSSL, Secure Transport, mbedTLS)
- Autobahn-compliant WebSocket implementation

### JSON Processing

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| nlohmann/json | latest (develop) | JSON parsing for REST APIs and config files | Industry standard for C++ JSON. MIT license, header-only, intuitive API, extensive documentation. Used by millions of projects. Essential for ncSender REST communication. |

**Confidence:** HIGH (verified from [official GitHub](https://github.com/nlohmann/json))

**Integration:**
```cmake
FetchContent_Declare(json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3  # Use specific tag in production
)
FetchContent_MakeAvailable(json)
target_link_libraries(your_target PRIVATE nlohmann_json::nlohmann_json)
```

**Notes:**
- Single header file (`json.hpp`)
- Intuitive syntax: `json j = R"({"key": "value"})"_json;`
- Native CMake support with FetchContent
- Excellent error messages

### PDF Generation

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| libHaru | v2.4.5 | PDF generation for quotes and reports | Zlib/LIBPNG license (permissive), cross-platform, actively maintained (March 2025 release). Mature library with comprehensive PDF features. No heavy dependencies. |

**Confidence:** MEDIUM (verified from [GitHub releases](https://github.com/libharu/libharu), but maintenance history shows gaps)

**Integration:**
```cmake
FetchContent_Declare(libharu
  GIT_REPOSITORY https://github.com/libharu/libharu.git
  GIT_TAG v2.4.5
)
FetchContent_MakeAvailable(libharu)
target_link_libraries(your_target PRIVATE hpdf)
```

**Features:**
- Text, images, outlines, annotations
- PNG/JPEG embedding
- TrueType/Type1 font embedding
- PDF encryption support
- Compression (deflate)

**Caveats:**
- C library (not C++), requires manual resource management
- API is procedural, not object-oriented
- Documentation can be sparse; expect code exploration

### Filesystem Monitoring

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| efsw | latest (master) | Watch folder monitoring for import workflows | MIT license, cross-platform (inotify/FSEvents/IOCP/kqueue), asynchronous notifications, recursive directory watching. Battle-tested in multiple projects. |

**Confidence:** MEDIUM (verified from [GitHub](https://github.com/SpartanJ/efsw), active but no recent versioned releases)

**Alternative:** watcher (MIT, newer design, minimal dependencies)

**Integration:**
```cmake
FetchContent_Declare(efsw
  GIT_REPOSITORY https://github.com/SpartanJ/efsw.git
  GIT_TAG master  # Pin to specific commit hash in production
)
FetchContent_MakeAvailable(efsw)
target_link_libraries(your_target PRIVATE efsw)
```

**Notes:**
- Platform-specific backends automatically selected
- UTF-8 string support
- Minimal overhead; asynchronous event callbacks
- Fallback to polling if native API unavailable

### Mathematical Expression Evaluation

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| ExprTk | latest | Feeds/speeds calculator formula engine | MIT license, header-only, extremely flexible runtime expression parser. Supports variables, functions, vectors. Ideal for user-defined machining formulas. |

**Confidence:** HIGH (verified from [official site](http://www.partow.net/programming/exprtk/) and [GitHub](https://github.com/ArashPartow/exprtk))

**Integration:**
```cmake
# Header-only: download exprtk.hpp
FetchContent_Declare(exprtk
  GIT_REPOSITORY https://github.com/ArashPartow/exprtk.git
  GIT_TAG master
)
FetchContent_Populate(exprtk)
target_include_directories(your_target PRIVATE ${exprtk_SOURCE_DIR})
```

**Usage Example:**
```cpp
#include "exprtk.hpp"
// Feed rate = RPM * chip_load * num_flutes
expression_t expr;
expr.register_symbol_table(symbol_table);
parser_t parser;
parser.compile("rpm * chip_load * num_flutes", expr);
double feed_rate = expr.value();
```

**Notes:**
- No build step required (header-only)
- Very fast compilation of expressions
- Extensible with custom functions
- Full operator precedence support

### String Formatting

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| {fmt} | latest (v11.x) | Modern string formatting for logging and UI | MIT license, optional header-only mode, safe and fast. Basis for C++20 std::format. Type-safe printf replacement. |

**Confidence:** HIGH (verified from [GitHub](https://github.com/fmtlib/fmt))

**Integration:**
```cmake
FetchContent_Declare(fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 11.0.2  # Or latest stable
)
FetchContent_MakeAvailable(fmt)
target_link_libraries(your_target PRIVATE fmt::fmt)
# OR for header-only mode:
# target_compile_definitions(your_target PRIVATE FMT_HEADER_ONLY)
```

**Why not std::format?**
- std::format is C++20 (project is C++17)
- {fmt} is the reference implementation
- Consistent API across all compilers

### Supporting Libraries

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| cppcodec | latest | Base64 encoding for API authentication | If ncSender uses Basic Auth or token encoding. MIT license, header-only, RFC 4648 compliant. |
| ada-url | v2.x | URL parsing and manipulation | If constructing REST endpoints programmatically. Apache 2.0/MIT dual license, WHATWG-compliant. |

**Confidence:** MEDIUM (cppcodec and ada-url verified from GitHub searches)

**Integration (cppcodec):**
```cmake
FetchContent_Declare(cppcodec
  GIT_REPOSITORY https://github.com/tplgy/cppcodec.git
  GIT_TAG master
)
FetchContent_Populate(cppcodec)
target_include_directories(your_target PRIVATE ${cppcodec_SOURCE_DIR})
```

**Integration (ada-url):**
```cmake
FetchContent_Declare(ada
  GIT_REPOSITORY https://github.com/ada-url/ada.git
  GIT_TAG v2.9.2  # Check for latest
)
FetchContent_MakeAvailable(ada)
target_link_libraries(your_target PRIVATE ada::ada)
```

## Architectural Pattern: Command Pattern for Undo/Redo

**Recommendation:** Implement in-house using standard C++17 patterns.

**Why not a library?**
- Command pattern is conceptual, not code-heavy
- Qt's undo framework is GPL/LGPL (license conflict)
- Most implementations are framework-specific
- Custom implementation integrates better with existing architecture

**Confidence:** HIGH (verified from [design pattern articles](https://gernotklingler.com/blog/implementing-undoredo-with-the-command-pattern/))

**Core Pattern:**
```cpp
class Command {
public:
    virtual ~Command() = default;
    virtual void execute() = 0;
    virtual void undo() = 0;
    virtual std::string describe() const = 0;
};

class CommandStack {
    std::vector<std::unique_ptr<Command>> undo_stack;
    std::vector<std::unique_ptr<Command>> redo_stack;
public:
    void execute(std::unique_ptr<Command> cmd);
    void undo();
    void redo();
};
```

**Resources:**
- [Implementing Undo/Redo with Command Pattern](https://gernotklingler.com/blog/implementing-undoredo-with-the-command-pattern/)
- [Command Pattern for Undo Functionality](https://matt.berther.io/2004/09/16/using-the-command-pattern-for-undo-functionality/)

## Alternatives Considered

| Category | Recommended | Alternative | Why Not |
|----------|-------------|-------------|---------|
| HTTP Client | cpp-httplib | cpprestsdk (MS C++ REST SDK) | cpprestsdk is archived/deprecated. Boost dependency. Overcomplicated for simple REST calls. |
| HTTP Client | cpp-httplib | libcurl | Not header-only, larger binary footprint, complex API. Good choice if HTTPS with many TLS backends needed. |
| WebSocket | IXWebSocket | WebSocket++ v0.8.2 | WebSocket++ last updated April 2020. IXWebSocket more actively maintained and simpler API. |
| WebSocket | IXWebSocket | Boost.Beast | Requires Boost (25MB+ size impact), steeper learning curve. |
| JSON | nlohmann/json | RapidJSON | RapidJSON is faster but less ergonomic. nlohmann/json is standard, readable, debug-friendly. |
| PDF | libHaru | PoDoFo | PoDoFo is LGPL (license conflict). More complex API. |
| PDF | libHaru | QPrinter (Qt) | Requires entire Qt framework (massive size increase). GPL/LGPL licensing. |
| Filesystem Watch | efsw | std::filesystem polling | C++17 std::filesystem has no native watch API. Manual polling is inefficient. |
| Filesystem Watch | efsw | watcher (e-dant) | Newer library, less battle-tested. efsw has longer track record. Both viable. |
| Expression Parser | ExprTk | muParser | ExprTk has more flexible API, better documentation, more active. Both MIT. |
| Formatting | {fmt} | std::ostringstream | Type-unsafe, verbose, error-prone. {fmt} is modern replacement. |
| Formatting | {fmt} | std::format (C++20) | Project is C++17. {fmt} is the reference implementation anyway. |

## What NOT to Use

| Avoid | Why | Use Instead |
|-------|-----|-------------|
| cpprestsdk (Microsoft C++ REST SDK) | Archived/deprecated by Microsoft. Last update 2021. Requires Boost. | cpp-httplib |
| Boost.Beast | Massive dependency footprint (Boost is 25MB+ alone). Overkill for simple REST/WebSocket. | cpp-httplib + IXWebSocket |
| Qt Networking | Requires entire Qt framework (100MB+). GPL/LGPL licensing conflicts with permissive-only requirement. | cpp-httplib + IXWebSocket |
| libcurl (for simple REST) | Not header-only, complex API, larger binary size. Good for complex HTTP needs, overkill here. | cpp-httplib |
| PoDoFo (PDF) | LGPL license (copyleft, conflicts with project rules). | libHaru |
| iText (PDF) | Java library. AGPL license. | libHaru |
| Custom HTTP client | Security vulnerabilities, TLS complexity, substantial development time. | cpp-httplib |
| Custom WebSocket client | RFC 6455 is complex. Error-prone. Not worth the effort. | IXWebSocket |
| Custom JSON parser | Slow, bug-prone, reinventing the wheel. | nlohmann/json |

## ncSender Protocol Integration Notes

Based on research, ncSender is not a widely documented standard. Assuming it follows common CNC control patterns:

**Typical CNC Control APIs:**
- REST endpoints: GET /status, POST /command, GET /state
- WebSocket events: machine state changes, job progress, alarms
- JSON payloads for commands and responses

**Recommended Approach:**
1. Use cpp-httplib for REST API calls (status queries, MDI commands)
2. Use IXWebSocket for real-time state monitoring
3. Use nlohmann/json for all payload serialization/deserialization

**Research Gap:** Specific ncSender protocol documentation not found. If ncSender uses a proprietary protocol, may need vendor docs.

**Confidence:** LOW (ncSender-specific), HIGH (general CNC control patterns)

**References:**
- [CNCjs Controller API](https://github.com/cncjs/cncjs) (WebSocket + REST example)
- [Buildbotics CNC API](https://buildbotics.com/introduction-to-the-buildbotics-cnc-controller-api/) (WebSocket + HTTP patterns)
- [NC.js REST API](https://steptools.github.io/NC.js/api/index.html) (WebSocket state management)

## Query Language for Advanced Search

**Recommendation:** Implement simple recursive descent parser in-house OR use ExprTk for boolean expressions.

**Why not a library?**
- Most query DSL libraries are for SQL (overkill)
- Lexy (C++ parsing DSL) is header-only but complex for simple search syntax
- Simple search syntax (e.g., `material:oak AND thickness:>0.75`) can be hand-parsed

**Confidence:** MEDIUM

**Suggested Syntax:**
```
field:value
field:>number
field:<number
AND / OR / NOT
Parentheses for grouping
```

**Implementation Path:**
1. Tokenize input string
2. Recursive descent parser for boolean logic
3. Build SQLite WHERE clause from parse tree

**If complexity grows:** Consider Lexy or PEG parsers. For MVP, hand-coded parser is sufficient.

## Version Compatibility Matrix

| Package | Compatible With | Notes |
|---------|-----------------|-------|
| cpp-httplib v0.31.0 | OpenSSL 1.1+, mbedTLS 2.x+ | SSL backend optional; choose one |
| IXWebSocket v11.4.6 | OpenSSL, Secure Transport, mbedTLS | Auto-detects platform SSL |
| nlohmann/json latest | C++11/14/17/20/23 | No external dependencies |
| libHaru v2.4.5 | zlib, libpng (optional) | zlib already in project |
| ExprTk | C++11+ | Header-only, no dependencies |
| {fmt} v11.x | C++11+ | Header-only mode available |
| efsw | C++11+ | Platform APIs (inotify, FSEvents, IOCP, kqueue) |

**SSL Backend Choice:**
- **OpenSSL:** Most common, best performance, larger binary (~2MB)
- **mbedTLS:** Smaller footprint (~500KB), embedded-friendly
- **Recommendation:** OpenSSL for desktop app (broader compatibility)

## Build Size Impact Estimate

| Library | Size Impact | Notes |
|---------|-------------|-------|
| cpp-httplib | ~200KB | Header-only, minimal compiled code |
| IXWebSocket | ~300KB | Small library, efficient |
| nlohmann/json | ~100KB | Header-only, optimizes well |
| libHaru | ~500KB | PDF rendering adds complexity |
| ExprTk | ~50KB | Header-only, templates optimize out unused code |
| {fmt} | ~150KB | Optional compiled mode smaller |
| efsw | ~100KB | Thin wrapper over OS APIs |
| **Total NEW** | **~1.4MB** | Well under 25MB budget |

Existing stack + new libraries: ~5MB total (SQLite ~1MB, SDL2 ~1MB, ImGui ~500KB, others ~1MB, new ~1.4MB). Ample headroom.

## Installation Pattern (CMakeLists.txt)

```cmake
include(FetchContent)

# HTTP Client
FetchContent_Declare(httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.31.0
)

# WebSocket Client
FetchContent_Declare(ixwebsocket
  GIT_REPOSITORY https://github.com/machinezone/IXWebSocket.git
  GIT_TAG v11.4.6
)

# JSON
FetchContent_Declare(json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)

# PDF Generation
FetchContent_Declare(libharu
  GIT_REPOSITORY https://github.com/libharu/libharu.git
  GIT_TAG v2.4.5
)

# Filesystem Watcher
FetchContent_Declare(efsw
  GIT_REPOSITORY https://github.com/SpartanJ/efsw.git
  GIT_TAG master
)

# Expression Parser (header-only)
FetchContent_Declare(exprtk
  GIT_REPOSITORY https://github.com/ArashPartow/exprtk.git
  GIT_TAG master
)

# String Formatting
FetchContent_Declare(fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 11.0.2
)

FetchContent_MakeAvailable(httplib ixwebsocket json libharu efsw fmt)
FetchContent_Populate(exprtk)  # Header-only, just populate

target_link_libraries(DigitalWorkshop PRIVATE
  httplib::httplib
  ixwebsocket::ixwebsocket
  nlohmann_json::nlohmann_json
  hpdf
  efsw
  fmt::fmt
)
target_include_directories(DigitalWorkshop PRIVATE ${exprtk_SOURCE_DIR})
```

## License Compliance Summary

All recommended libraries use permissive licenses compatible with project requirements:

| Library | License | Compliant |
|---------|---------|-----------|
| cpp-httplib | MIT | ✓ |
| IXWebSocket | BSD-3-Clause | ✓ |
| nlohmann/json | MIT | ✓ |
| libHaru | zlib/LIBPNG | ✓ |
| efsw | MIT | ✓ |
| ExprTk | MIT | ✓ |
| {fmt} | MIT | ✓ |
| cppcodec | MIT | ✓ |
| ada-url | Apache 2.0 / MIT | ✓ |

**No GPL/LGPL dependencies.** All licenses allow commercial use, modification, and distribution.

## Sources

**HTTP/REST Libraries:**
- [cpp-httplib GitHub](https://github.com/yhirose/cpp-httplib) — Latest release verification (HIGH confidence)
- [cpp-httplib releases](https://github.com/yhirose/cpp-httplib/releases) — Version v0.31.0 confirmed (HIGH confidence)
- [Microsoft cpprestsdk](https://github.com/microsoft/cpprestsdk) — Deprecated status (HIGH confidence)

**WebSocket Libraries:**
- [IXWebSocket GitHub](https://github.com/machinezone/IXWebSocket) — Latest release v11.4.6 (HIGH confidence)
- [WebSocket++ GitHub](https://github.com/zaphoyd/websocketpp) — Version 0.8.2, April 2020 (MEDIUM confidence)

**JSON Libraries:**
- [nlohmann/json GitHub](https://github.com/nlohmann/json) — Industry standard status (HIGH confidence)
- [RapidJSON](https://rapidjson.org/) — Alternative evaluation (MEDIUM confidence)

**PDF Libraries:**
- [libHaru GitHub](https://github.com/libharu/libharu) — Version v2.4.5, March 2025 (MEDIUM confidence)
- [libHaru SourceForge](https://libharu.sourceforge.net/) — Feature documentation (MEDIUM confidence)

**Filesystem Monitoring:**
- [efsw GitHub](https://github.com/SpartanJ/efsw) — Cross-platform support (MEDIUM confidence)
- [watcher GitHub](https://github.com/e-dant/watcher) — Alternative option (MEDIUM confidence)

**Expression Parsing:**
- [ExprTk Official](http://www.partow.net/programming/exprtk/) — Documentation and examples (HIGH confidence)
- [ExprTk GitHub](https://github.com/ArashPartow/exprtk) — MIT license verification (HIGH confidence)

**String Formatting:**
- [{fmt} GitHub](https://github.com/fmtlib/fmt) — MIT license, C++20 basis (HIGH confidence)

**Supporting Libraries:**
- [cppcodec GitHub](https://github.com/tplgy/cppcodec) — Base64 encoding (MEDIUM confidence)
- [ada-url GitHub](https://github.com/ada-url/ada) — URL parsing (MEDIUM confidence)

**Design Patterns:**
- [Implementing Undo/Redo with Command Pattern](https://gernotklingler.com/blog/implementing-undoredo-with-the-command-pattern/) — Implementation guide (HIGH confidence)
- [Command Pattern for Undo](https://matt.berther.io/2004/09/16/using-the-command-pattern-for-undo-functionality/) — Additional reference (MEDIUM confidence)

**CNC Control Patterns:**
- [CNCjs GitHub](https://github.com/cncjs/cncjs) — WebSocket + REST example (MEDIUM confidence)
- [Buildbotics CNC API](https://buildbotics.com/introduction-to-the-buildbotics-cnc-controller-api/) — Industry patterns (MEDIUM confidence)
- [NC.js REST API](https://steptools.github.io/NC.js/api/index.html) — WebSocket state management (MEDIUM confidence)

**Query Language Parsing:**
- [Lexy Parsing DSL](https://lexy.foonathan.net/) — C++ parsing library (MEDIUM confidence)

---

**Overall Stack Confidence:** HIGH for core libraries (HTTP, WebSocket, JSON, expression parsing), MEDIUM for PDF and filesystem watching (fewer alternatives, some maintenance concerns).

**Key Recommendation:** Start with cpp-httplib, IXWebSocket, nlohmann/json, and ExprTk. These are battle-tested, well-documented, and integrate seamlessly with CMake FetchContent.
