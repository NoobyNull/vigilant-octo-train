# Phase 7: Firmware Settings - Context

**Gathered:** 2026-02-24
**Status:** Ready for planning

<domain>
## Phase Boundary

View, edit, backup, and restore GRBL firmware settings. Machine tuning UI for per-axis parameters. Bidirectional sync between DW machine profiles and GRBL $$ settings. Requirements: FWC-01 through FWC-07.

</domain>

<decisions>
## Implementation Decisions

### Settings Panel (FWC-01-02)
- Read GRBL $$ settings on connect, display with human-readable descriptions
- Each setting editable with validation (min/max/type constraints based on GRBL docs)
- Changes written to GRBL one setting at a time ($X=value)
- EEPROM write consideration: add small delay between setting writes to avoid GRBL data loss

### Backup/Restore (FWC-03-04)
- Export: read all $$ → save to JSON file with machine name and date
- Import: read JSON → confirm with user → write settings one by one
- Confirmation dialog shows diff between current and backup values

### Machine Tuning (FWC-05)
- Focused UI for the most common tuning parameters: steps/mm, max feed rate, acceleration per axis
- This is a subset of the full settings panel, presented in a more intuitive way

### Profile Sync (FWC-06-07)
- On connect: read GRBL $$ → update DW machine profile
- Push: write DW machine profile values → GRBL $$ settings
- Existing MachineProfile class (src/core/gcode/machine_profile.h) maps to GRBL $ settings

### Claude's Discretion
- Settings panel layout (table, grouped sections, searchable list)
- Whether tuning is a separate panel or a tab within the settings panel
- Backup file format details (JSON structure)
- How to handle unknown settings (grblHAL/FluidNC extend beyond standard GRBL)
- Delay timing between EEPROM writes

</decisions>

<specifics>
## Specific Ideas

- Existing MachineProfile already has per-axis settings and GRBL $ mapping
- Research identified EEPROM write data loss as a critical pitfall — must add delays
- GRBL $$ response format is well-documented: "$0=10" per line
- FluidNC and grblHAL may have additional settings — handle unknown settings gracefully

</specifics>

<deferred>
## Deferred Ideas

- FluidNC YAML config parsing — if $cmd interface proves insufficient, future work
- grblHAL tool table sync — low priority, future investigation

</deferred>

---

*Phase: 07-firmware-settings*
*Context gathered: 2026-02-24*
