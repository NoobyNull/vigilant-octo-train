# External Integrations

**Analysis Date:** 2026-02-19

## APIs & External Services

**None Detected**

This is a self-contained desktop application with no external API dependencies. The application does not integrate with:
- Cloud APIs (AWS, Azure, GCP, etc.)
- SaaS platforms (Stripe, Auth0, SendGrid, etc.)
- Real-time services (Firebase, Supabase, etc.)
- Third-party authentication providers

All functionality is built on local computation and local data storage.

## Data Storage

**Databases:**
- **SQLite3 (local file-based)**
  - Connection: Direct file-based (`*.db` in user's app data directory)
  - ORM/Client: Custom RAII wrapper in `src/core/database/database.h`
  - Location: Determined by `src/core/paths/app_paths.cpp` (platform-specific app data directories)
  - Features:
    - WAL mode for concurrent access
    - Foreign keys enabled
    - Transaction support with RAII `Transaction` class
    - Connection pooling via `ConnectionPool` class

**File Storage:**
- **Local filesystem only**
  - User's local library directory (configurable in settings)
  - Default location: `~/.local/share/DigitalWorkshop/library/` (Linux) or equivalent on Windows/macOS
  - Supported file formats:
    - Models: `.stl`, `.obj`, `.3mf` (loaded and cached)
    - Projects: `.dwp` (custom JSON-based format)
    - G-code: `.gcode`, `.nc`, `.tap` (parsed and analyzed)
    - Archives: Material archive files (ZIP-based with `.mat` extension)
  - Usage: Import pipeline in `src/core/import/` reads from user-selected directories

**Caching:**
- None - Application uses direct filesystem and database access
- Thumbnails: Generated on-demand and stored as TGA files in local cache directory

## Configuration Storage

**Configuration Format:**
- INI-style text file persisted to app config directory
- Location: `~/.config/DigitalWorkshop/config.ini` (or platform equivalent)
- Implementation: `src/core/config/config.cpp` with atomic write safety
- Hot-reload: File change detection via polling in `ConfigWatcher` class

**Configuration Content (not secrets):**
- Window dimensions and maximized state
- Theme selection (Dark/Light/HighContrast)
- UI scale and layout preferences
- Navigation style (Default/CAD/Maya)
- Recent projects list (paths only, max 10)
- Input key bindings and modifiers
- Render settings (lighting, ambient, material colors)
- Workspace panel visibility state
- Log level
- Import pipeline settings (parallelism, file handling mode)
- Library directory path

**Settings Application:**
- Separate executable in `settings/` directory (built from `src/CMakeLists.txt` as additional target)
- Provides GUI for modifying configuration without touching config.ini directly
- Communicates via config file changes (monitored by hot-reload system)

## Authentication & Identity

**Auth Provider:**
- None - Local application only
- No user accounts, login, or authentication required
- All data is local to user's machine

**User Identification:**
- Anonymous/implicit (first user on machine)
- No user session management

## Monitoring & Observability

**Error Tracking:**
- None - No external error reporting service
- Errors logged locally to console and log file
- Log level configurable in config file

**Logging:**
- Framework: Custom logger in `src/core/utils/log.h`
- Output: Stderr/stdout + optional file logging
- Levels: Debug, Info, Warning, Error (configurable)
- Usage: Throughout codebase via `log::debug()`, `log::info()`, `log::warning()`, `log::error()`

## CI/CD & Deployment

**Hosting:**
- None - This is a self-contained desktop application
- Distribution: Direct executable distribution or installer packages

**Packaging:**
- Windows: NSIS installer (`.exe`) with automatic updates via file associations
- Linux: TGZ archive (gzip-compressed tarball)
- Installation: CPack-based (CMake native packaging)

**Version Identification:**
- Git commit hash embedded at build time
- Build date included in version string
- No external version server or update checker

## Environment Configuration

**Required Environment Variables:**
- None

**Optional Environment Variables:**
- None

**Configuration File Locations (detected at runtime):**
- Linux:
  - Config: `~/.config/DigitalWorkshop/config.ini`
  - App data: `~/.local/share/DigitalWorkshop/`
  - Cache: `~/.cache/DigitalWorkshop/`

- Windows:
  - Config: `%APPDATA%/DigitalWorkshop/config.ini`
  - App data: `%APPDATA%/DigitalWorkshop/`
  - Cache: `%LOCALAPPDATA%/DigitalWorkshop/cache/`

- macOS:
  - Config: `~/Library/Application Support/DigitalWorkshop/config.ini`
  - App data: `~/Library/Application Support/DigitalWorkshop/`
  - Cache: `~/Library/Caches/DigitalWorkshop/`

**Path Detection Implementation:**
- `src/core/paths/app_paths.h` - Platform-specific path resolution
- Called at startup: `paths::ensureDirectoriesExist()` in `src/app/application.cpp`
- Automatic directory creation if missing

## Webhooks & Callbacks

**Incoming:**
- None

**Outgoing:**
- None

## Event System (Internal)

**Event Bus:**
- Custom local event system in `src/core/events/event_bus.h`
- Not external - used for inter-module communication within the application
- Implementation: Observer pattern with thread-safe event dispatch
- Thread context: Dispatches on main thread via `MainThreadQueue`

**Key Event Types:**
- Model selection changes
- Import queue updates
- Project modifications
- Workspace configuration changes
- Rendering state updates

## Data Exchange Formats

**Import/Export:**
- **3D Models:**
  - STL binary/ASCII format (external files, no protocol)
  - OBJ format (external files)
  - 3MF format (ZIP container with XML manifest, RFC 3161 compliance)

- **Projects:**
  - DWP format (JSON-serialized in database or exported to file)
  - Contains model associations, cost estimates, metadata

- **G-code:**
  - Standard RS-274X G-code format (ASCII text)
  - No special protocol or API

- **Material Libraries:**
  - ZIP archives with metadata and thumbnails
  - Material data stored as JSON in database

## License & Terms

**Runtime Dependencies Licenses:**
- SDL2: zlib/libpng license (permissive)
- Dear ImGui: MIT license (permissive)
- OpenGL GLAD: D3D9/MIT (permissive)
- GLM: Happy Bunny / MIT (permissive)
- SQLite: Public domain (CC0)
- zlib: zlib license (permissive)
- miniz: MIT license (permissive)
- stb: CC0 / MIT (permissive)
- nlohmann/json: MIT license (permissive)
- GoogleTest: BSD 3-Clause (dev/test only)

No GPL dependencies; fully compatible with commercial use.

---

*Integration audit: 2026-02-19*
