---
phase: 13-tcp-ip-byte-stream-transport-for-cnc-connections
plan: 01
subsystem: cnc
tags: [byte-stream, interface, polymorphism, serial, transport]

requires:
  - phase: 12-cnc-always-connected-and-unified-settings
    provides: SerialPort and CncController implementation
provides:
  - IByteStream abstract interface for transport polymorphism
  - SerialPort inheriting IByteStream
  - CncController using unique_ptr<IByteStream>
affects: [13-02, 13-03, future-transport-implementations]

tech-stack:
  added: []
  patterns: [transport-agnostic byte stream interface, unique_ptr polymorphism]

key-files:
  created:
    - src/core/cnc/byte_stream.h
  modified:
    - src/core/cnc/serial_port.h
    - src/core/cnc/cnc_controller.h
    - src/core/cnc/cnc_controller.cpp

key-decisions:
  - "ConnectionState enum moved to byte_stream.h (shared by all transports)"
  - "open() excluded from IByteStream (transport-specific signatures)"
  - "CncController owns unique_ptr<IByteStream> for polymorphic dispatch"

patterns-established:
  - "Transport interface: all transports implement IByteStream"
  - "Transport construction: create concrete type, open, then move into unique_ptr"

requirements-completed: [TCP-01, TCP-02]

duration: 5min
completed: 2026-02-28
---

# Plan 13-01: IByteStream Interface Extraction Summary

**Abstract IByteStream interface extracted from SerialPort; CncController refactored to unique_ptr<IByteStream> for polymorphic transport dispatch**

## Performance

- **Duration:** 5 min
- **Tasks:** 2
- **Files modified:** 4 (1 created, 3 modified)

## Accomplishments
- Created `IByteStream` abstract base class with all transport-agnostic methods
- Refactored `SerialPort` to inherit from `IByteStream` with `override` on all methods
- Changed `CncController::m_port` from `SerialPort` value to `unique_ptr<IByteStream>`
- All 715 existing tests pass without modification

## Task Commits

1. **Task 1+2: Create IByteStream and refactor CncController** - `e456dd5` (feat)

## Files Created/Modified
- `src/core/cnc/byte_stream.h` - IByteStream abstract base class with ConnectionState enum
- `src/core/cnc/serial_port.h` - SerialPort now inherits from IByteStream
- `src/core/cnc/cnc_controller.h` - m_port is now unique_ptr<IByteStream>
- `src/core/cnc/cnc_controller.cpp` - connect() creates SerialPort via make_unique, all m_port. -> m_port->

## Decisions Made
- ConnectionState enum moved from serial_port.h to byte_stream.h (needed by all transports)
- open() excluded from IByteStream -- each transport has different open signature
- device() kept in interface -- returns transport-specific identifier string

## Deviations from Plan
None - plan executed exactly as written.

## Issues Encountered
None

## Next Phase Readiness
- IByteStream interface ready for TcpSocket implementation in Plan 13-02
- CncController ready to accept any IByteStream transport via unique_ptr

---
*Phase: 13-tcp-ip-byte-stream-transport-for-cnc-connections*
*Completed: 2026-02-28*
