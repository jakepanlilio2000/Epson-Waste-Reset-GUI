# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

> **Note on Fork:** This repository is an enhanced fork of the original [RxNaison/Epson-Waste-Reset](https://github.com/RxNaison/Epson-Waste-Reset). It introduces a fully interactive C++ desktop graphical interface (GUI), expanded multi-platform/multi-architecture releases, and enhanced documentation, while maintaining complete backward compatibility with upstream CLI and C ABI contracts.

---

## [1.6.0] - 2026-11-05

### Added
- **Interactive C++ Desktop GUI (`ewr_gui`)**:
  - Native Windows desktop application built with C++17, DirectX 11, and Dear ImGui.
  - **Live Hardware Dashboard**:
    - Real-time visual progress gauges for waste ink pad counters (Main Pad, Platen Pad, etc.) with used/max counts and percentage indicators.
    - Dynamic color transitions: Emerald Green (`< 80%`), Warning Amber (`80–99%`), and Critical Red (`100%`) with automatic service-limit alerts.
    - Cartridge ink levels visualizer with authentic colors for Black, Cyan, Magenta, Yellow, Light Cyan, Light Magenta, etc.
  - **Guided Empty-State Onboarding**:
    - Intuitive 3-step setup guide for first-time users (`USB Auto-Detect`, `LAN Network Scan`, `Manual Model Selector`).
    - Preparation checklist covering printer power, Windows Print Spooler queues, and hardware safety.
  - **Searchable Model Database Picker**:
    - Instant autocomplete search popup filtering across all 1,450+ models and aliases.
    - Quick-select button that directly configures target printer profile.
  - **One-Click Maintenance Actions**:
    - `Reset Waste Ink Pad Counters`: Full lifecycle execution (preflight read, safety gates, EEPROM writes, read-back verification).
    - `Reset Cartridge Ink Levels`: Resets ink level flags for models supporting per-color ink maps.
    - `Dry-Run / Test Simulation`: Simulates planned writes without modifying EEPROM hardware.
    - `Dump EEPROM`: Dumps all EEPROM addresses for inspection and backup.
  - **Network Printer Scanner**:
    - Broadcasts SNMP queries (UDP 161) to discover Epson printers on Wi-Fi and Ethernet in ~2 seconds.
    - Displays IP address, reported model, database match, and device ID with a one-click `Connect` button.
  - **EEPROM Hex Inspector**:
    - Monospace 16-byte memory table with hex offsets, byte values, and ASCII decode.
    - Export functionality to raw binary (`.bin`) and structured JSON (`.json`).
  - **Live Activity & Trace Log**:
    - Real-time event stream from EWR engine callbacks.
    - Severity filtering (`All`, `Info+`, `Warnings+`, `Errors`), text search, auto-scroll toggle, and copy to clipboard.
  - **Responsive Design & Windows 10/11 Polish**:
    - Immersive dark title bar (`DWMWA_USE_IMMERSIVE_DARK_MODE`).
    - Dynamic DPI scaling with Segoe UI system font rendering.
    - Minimum window size constraints (`680x480`) and auto-stacking layouts for compact screens.
    - Informative hover tooltips across all buttons, inputs, and toggles.
- **Comprehensive Multi-Architecture Release Matrix**:
  - **Windows**: `x64` (64-bit), `x86` (32-bit Win32), `arm64` (Windows on ARM) with both CLI (`ewr.exe`) and GUI (`ewr_gui.exe`).
  - **Linux**: `x64` (`x86_64`), `x86` (`i686`), `arm64` (`aarch64`), `arm` (`armv7hf` / Raspberry Pi).
  - **macOS**: Apple Silicon (`arm64`), Intel (`x86_64`), and Universal (`arm64 + x86_64`).
  - **C API SDK**: Dedicated SDK archives for every target architecture with C ABI headers (`ewr_c.h`) and shared libraries (`ewrc.dll`, `libewrc.so`, `libewrc.dylib`).

---

## [1.5.1] - 2026-10-18 (Upstream Baseline)

### Added
- SNMP directed broadcast discovery across local network subnets (`ewr_discover_network`).
- Handover of the machine-wide run lock from USB session to network session.
- Schema 4 database format introducing per-counter masks, weights, and service limits.

### Fixed
- Addressed 1-byte address framing for legacy models.
- Resolved Windows Ready Print / Microsoft IPP Class Driver port conflicts.

---

## [1.5.0] - 2026-09-12 (Upstream)

### Added
- Network printer communication over SNMP (UDP 161) via `--ip <address>` and `--ip auto`.
- C ABI version 2 introducing `ewr_session_open_network` and network discovery.
- Safe EEPROM refusal detection on recent models that lock EEPROM over network connections.

---

## [1.4.1] - 2026-08-01 (Upstream)

### Added
- Signed build-provenance attestations via GitHub SLSA Sigstore (`gh attestation verify`).
- Diagnostic USB soft-reset flag (`--usb-soft-reset`) on Windows.
- Asynchronous `OVERLAPPED` I/O in Win32 `SetupAPI` transport to drain print spooler buffers.

---

## [1.4.0] - 2026-06-15 (Upstream)

### Added
- Over-The-Air (OTA) printer database synchronization from GitHub repository at startup.
- Offline database fallback shipping with the distribution (`database.json`).
- Extended support to over 1,450 printer models across EcoTank (ET), L-series, WorkForce (WF), Expression (XP), and Stylus Photo lines.

