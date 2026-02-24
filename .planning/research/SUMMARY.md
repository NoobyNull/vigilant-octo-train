# Research Summary: CNC Control Suite Milestone

**Domain:** Desktop CNC controller GUI (GRBL/grblHAL/FluidNC compatible)
**Researched:** 2026-02-24
**Overall confidence:** HIGH

## Executive Summary

Digital Workshop has a solid low-level CNC communication foundation: character-counting streaming, 5Hz status polling, real-time command injection, override support, feed hold/cycle start, and alarm handling. The existing code correctly implements the GRBL v1.1 protocol for serial communication. What is missing is entirely at the application and integration layer: UI panels to surface the existing data, tool database integration with the controller, job management/safety features, and firmware settings modification.

The technology stack requires zero new external dependencies for the core milestone. Everything needed (serial communication, SQLite persistence, JSON serialization, ImGui UI, OpenGL visualization) is already in the project. The only significant platform gap is Windows serial port support (currently stubbed). A WebSocket transport for FluidNC WiFi connectivity is desirable but can be deferred to a later phase without blocking the core workflow.

The competitive landscape (UGS, CNCjs, bCNC, Candle, gSender) shows that all senders implement essentially the same GRBL protocol layer. Digital Workshop's differentiation comes from its integrated Vectric-compatible tool database and Janka-based feeds/speeds calculator -- no other GRBL sender has these. The milestone should prioritize connecting these existing subsystems to the controller rather than building features that other senders already do better.

Seven specific bugs/risks were identified in the existing CncController code (documented in PITFALLS.md), most notably a thread safety violation in real-time command dispatch and missing soft-reset-on-error behavior. These should be fixed before building new features on top of the controller.

## Key Findings

**Stack:** No new dependencies needed. Win32 serial port is the only required platform addition. Transport abstraction (SerialPort -> Transport interface) enables future WebSocket/TCP support with zero CncController changes.

**Architecture:** Separate ImGui panels per concern (JogPanel, GrblSettingsPanel, MacroPanel, ProbePanel), wired through existing MainThreadQueue callback pattern. JobStateModel as a read-only derived state machine guards dangerous operations. GrblSettingsManager caches firmware settings with validation and backup.

**Critical pitfall:** Thread safety violation in existing real-time command methods (feedHold, cycleStart, overrides) -- they call SerialPort directly from the UI thread while the IO thread also uses SerialPort. Must fix before adding features.

## Implications for Roadmap

Based on research, suggested phase structure:

1. **Fix Foundation** - Address existing bugs in CncController (thread safety, error handling, disconnect detection)
   - Addresses: PITFALLS CRITICAL-2, MODERATE-2, MODERATE-6
   - Avoids: Building new features on a buggy base

2. **Core Control UI** - Build the table-stakes UI panels (DRO, jog, homing, zero-setting, console input)
   - Addresses: Every "PARTIAL" or "MISSING" table-stakes feature
   - Avoids: Scope creep into differentiator features before basics work

3. **Tool-Aware Controller** - Connect tool DB + calculator + material system to the controller
   - Addresses: Primary differentiator (M6 interception, feed validation, recommended vs actual overlay)
   - Avoids: Building a "me too" sender; this is where DW becomes unique

4. **Safety and Recovery** - Modal state tracker, job resume assistant, pre-flight checks
   - Addresses: CRITICAL-4 (unsafe resume), job management
   - Avoids: Shipping resume features without the modal state tracker foundation

5. **Firmware and Macros** - GRBL $$ settings panel, macro storage and execution, machine profile sync
   - Addresses: Settings modification, automation, tool-change workflows
   - Avoids: Dangerous settings changes without validation/backup infrastructure

6. **Platform and Transport** - Windows serial implementation, optional WebSocket for FluidNC
   - Addresses: Cross-platform gap, WiFi machine support
   - Avoids: Platform work blocking core feature development

**Phase ordering rationale:**
- Phase 1 before everything else because the thread safety bug affects all future features
- Phase 2 before 3 because tool integration requires a working UI to test against
- Phase 3 before 4 because the tool-aware features are DW's unique value and should ship early
- Phase 4 before 5 because safe resume is more critical than convenience macros
- Phase 6 last because it is pure platform work that does not add user-visible features for Linux users

**Research flags for phases:**
- Phase 1: Standard bug fixes, no additional research needed
- Phase 2: Standard ImGui patterns, no additional research needed
- Phase 3: M6 interception pattern well-documented by bCNC; may need phase-specific research on how to present recommended vs actual feed in the UI
- Phase 4: Modal state reconstruction is complex; needs phase-specific research on edge cases (arc mid-point resume, coordinate system changes mid-program)
- Phase 5: FluidNC YAML parsing needs investigation if $cmd interface proves insufficient
- Phase 6: Windows serial API is well-documented; WebSocket RFC 6455 framing may need library evaluation

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | No new deps needed; existing stack covers everything |
| Features | HIGH | Well-understood domain; competitive analysis of 6+ senders |
| Architecture | HIGH | Extends existing patterns; code review confirms feasibility |
| Pitfalls | HIGH | Verified against GRBL wiki, gnea/grbl issues, community reports |
| GRBL Protocol | HIGH | Official wiki is authoritative and well-maintained |
| FluidNC WebSocket | MEDIUM | Wiki was unreachable during research; used GitHub issues as source |
| grblHAL Extensions | MEDIUM | Growing ecosystem, less documentation than classic GRBL |
| Resume/Recovery | MEDIUM | Universal approach is clear but edge cases are poorly documented |

## Gaps to Address

- **Arc resume edge case:** If a job is interrupted mid-arc (G2/G3), the resume preamble must handle partial arc state. This needs deeper investigation during Phase 4.
- **FluidNC YAML parsing:** The $cmd interface may not expose all configuration paths. Need to verify with real FluidNC hardware.
- **grblHAL tool table integration:** grblHAL has a persistent tool table on the controller side. DW's tool DB could potentially sync with it. Low priority but worth investigating.
- **Windows serial port testing:** The Win32 implementation cannot be verified without a Windows CI environment with serial hardware (or virtual COM port).
- **Input pin parsing:** The MachineStatus.inputPins field is parsed but the bit definitions are not mapped to human-readable sensor names. Need the GRBL `Pn:` field documentation for bit mapping.

## Sources

All sources are documented in the individual research files:
- [GRBL v1.1 Interface](https://github.com/gnea/grbl/wiki/Grbl-v1.1-Interface) -- primary protocol reference
- [GRBL v1.1 Configuration](https://github.com/gnea/grbl/wiki/Grbl-v1.1-Configuration) -- settings reference
- [UGS](https://github.com/winder/Universal-G-Code-Sender) -- architecture reference
- [CNCjs](https://github.com/cncjs/cncjs) -- feature/UX reference
- [bCNC](https://github.com/vlachoudis/bCNC/wiki/Tool-Change) -- macro/tool-change reference
- [Candle](https://github.com/Denvi/Candle) -- C++ implementation reference
- [FluidNC](https://github.com/bdring/FluidNC) -- ESP32/WiFi firmware reference
- [grblHAL](https://github.com/grblHAL/core) -- extended GRBL features reference
