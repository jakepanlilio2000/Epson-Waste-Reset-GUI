# EWR (Epson Waste Reset) — GUI & Multi-Platform Edition

[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS%20%7C%20Android-blue)](https://github.com/RxNaison/Epson-Waste-Reset)
[![Architecture](https://img.shields.io/badge/arch-x86%20%7C%20x64%20%7C%20arm%20%7C%20arm64-blueviolet)](https://github.com/RxNaison/Epson-Waste-Reset)
[![C++](https://img.shields.io/badge/language-C%2B%2B17-orange)](https://en.cppreference.com/w/cpp/17)
[![License](https://img.shields.io/badge/license-Apache_License_2.0-green)](LICENSE)

A free, cross-platform, and completely open-source C++ utility and graphical desktop tool to reset the **"Waste Ink Pad"** counter on Epson printers.

EWR bypasses the need to pay for commercial reset keys (e.g. WIC Reset) or run closed-source, malware-flagged `AdjProg.exe` executables. By dynamically generating IEEE 1284.4 hardware packets and utilizing a continuously updated database, EWR communicates directly with the printer's motherboard over USB or SNMP network protocol to safely read, inspect, and zero out the EEPROM waste counters.

---

> [!NOTE]
> ### 🌟 Upstream Attribution & Fork Notice
> This project is an enhanced fork of the original [**RxNaison/Epson-Waste-Reset**](https://github.com/RxNaison/Epson-Waste-Reset) created by [**RxNaison**](https://github.com/RxNaison).
>
> **Major Enhancements in this Fork:**
> 1. **Interactive Native C++ Desktop GUI (`ewr_gui`)**: A responsive, lightweight, compact desktop interface built on Dear ImGui and DirectX 11 / Win32. Features real-time visual progress gauges, dynamic threshold coloring, instant searchable model picker (~1,450+ models), LAN SNMP network printer discovery, EEPROM hex inspector, and safety-verified one-click reset flows.
> 2. **Multi-Architecture & Multi-OS CI/CD Pipeline**: Automated GitHub Actions release pipeline building standalone, self-contained packages for **Windows, Linux, macOS, and Android** across **x86_64, ARM (armhf), and ARM64 (aarch64)** architectures, as well as Universal macOS binaries, Linux `.deb` / `.rpm` packages, and unified C ABI SDKs.
> 3. **Non-Blocking Architecture**: Fully asynchronous background engine execution with live status, stage, and packet trace callbacks.
> 4. **100% Core Compatibility**: Retains the exact same safety preflight, read-back verification lifecycle, backups/restore system, and C ABI as upstream.

---

## Features

* **Interactive Desktop GUI (`ewr_gui`):** High-performance native Windows GUI with live visual gauges, searchable model list, hex editor, and guided one-click maintenance flows.
* **Reads before it writes, and verifies after.** Every reset runs the same lifecycle: read the current counters, refuse to continue if the printer reports a state that makes the reset pointless or unsafe, ask you once, write, then read the counters back and compare. You see the numbers move instead of trusting an acknowledgement.
* **Every write can be undone.** Before the first byte is written, EWR saves the bytes it is about to overwrite to `backups/`. `--restore` puts them back — from that backup, or from an `--dump` file — through the same gates and read-back as a reset.
* **Printer maintenance, too.** Nozzle check, head cleaning, power cleaning and the printer's own counters (pages, cleanings, firmware, serial number), over USB or the network — behind flags and the API, so a double-click still does exactly one thing: the reset.
* **Smart Protocol Engine:** Constructs exact EEPROM write packets (`|B`) on the fly for your specific model. It manages the IEEE 1284.4 (D4) credit system properly, so nothing overflows a buffer or locks the printer up.
* **~1,450 printers supported**, from Stylus Photo R-series to the current ET / L / XP / WF lines. Cartridge ink levels can be reset too on models that expose a per-colour map.
* **OTA Database Sync:** The printer database refreshes itself at startup, before anything is read from or written to the printer, so a database fix applies to the run that fetched it and the database in use never changes underneath a reset. The full database also ships in the download — EWR works offline out of the box.
* **Cross-Platform Core:**
  * **Windows (x64 and ARM64):** Native Win32 `SetupAPI` with asynchronous `OVERLAPPED` I/O to safely drain the Windows Print Spooler buffers, plus a statically linked `libusb` for the vendor-specific interfaces `usbprint.sys` cannot reach — on ET-2xxx units the maintenance engine lives on one of those. Both transports run in one pass, `usbprint.sys` first. Zero custom drivers required. Includes both `ewr.exe` (CLI) and `ewr_gui.exe` (GUI).
  * **Linux (x86_64, aarch64, armhf - a Raspberry Pi too) & macOS (one universal binary for Intel and Apple Silicon):** Uses `libusb` to automatically detach the kernel driver (CUPS) for exclusive, raw hardware access. `.deb` and `.rpm` packages install a udev rule, so no `sudo` is needed.
  * **Android:** An app that reaches the printer over an OTG cable or Wi-Fi, built on the same C library.
* **Zero Hardcoded PIDs:** Scans your OS USB tree to find connected Epson printers and identifies them from the IEEE 1284 device ID they report.
* **A trace log for every run:** `ewr_trace.log` records every byte in both directions next to the binary. It is what makes a bug report solvable.
* **Replay Fallback:** If your printer is too new for the database, EWR can still parse and execute a raw Wireshark dump you supply (stripping USBPcap headers automatically).

---

## 🖥️ Interactive Desktop GUI (`ewr_gui`)

The C++ graphical interface (`ewr_gui.exe`) brings all diagnostic and maintenance capabilities into a modern, compact desktop window.

### Key GUI Features
* **Real-time Waste Ink Gauges**: Visual progress bars display current counter values, service limits, and exact percentages for all configured waste pad groups (Main Waste Ink Pad, Platen Pad).
* **Smart Dynamic Color Coding**:
  - 🟢 **Normal (`< 80%`)**: Healthy operation.
  - 🟡 **Warning (`80% – 99%`)**: Maintenance due soon.
  - 🔴 **Critical (`100%`)**: Service limit reached (printer locked).
* **Cartridge Ink Levels**: Dedicated visualizer for models exposing per-color ink maps (Black, Cyan, Magenta, Yellow, Light Cyan, Light Magenta, etc.).
* **Searchable Model Database**: Instant filter and selector popup searching across ~1,450+ models and aliases with fuzzy search.
* **Network Printer Discovery**: Broadcasts SNMP UDP queries across local subnets to discover Wi-Fi and Ethernet Epson printers in seconds.
* **EEPROM Memory Inspector**: Live 16-byte monospace hex viewer with ASCII decode, supporting exports to raw binary (`.bin`) and JSON (`.json`).
* **One-Click Maintenance Actions**:
  - `Reset Waste Ink Pad Counters`: Complete guided safety lifecycle (preflight check -> safety confirmation dialog -> EEPROM write -> read-back verification).
  - `Reset Ink Levels`: Rewrites ink cartridge accounting flags on compatible models.
  - `Dry Run / Simulation`: Simulates planned write sequences without modifying EEPROM hardware.
  - `Dump EEPROM`: Creates a full timestamped memory snapshot.
* **Live Event & Trace Log**: Color-coded log streaming real-time engine packets, with filter levels, auto-scroll, and one-click copy to clipboard.
* **Native Windows Polish**: Windows 11 / 10 Immersive Dark title bar (`DWMWA_USE_IMMERSIVE_DARK_MODE`), high-DPI scaling, and responsive auto-stacking layouts.

---

## 📦 Downloads & Supported Platforms

Every release is packaged into self-contained archives bundled with `database.json`. Windows packages include both the command-line interface (`ewr.exe`) and the graphical desktop application (`ewr_gui.exe`).

| Platform / Operating System | Architecture | Target Package | Contents |
| :--- | :--- | :--- | :--- |
| **Windows** | **x64** (64-bit Intel/AMD) | `EWR-<tag>-windows-x64.zip` | `ewr.exe`, `ewr_gui.exe`, `database.json` |
| **Windows** | **ARM64** (Snapdragon / Surface) | `EWR-<tag>-windows-arm64.zip` | `ewr.exe`, `ewr_gui.exe`, `database.json` |
| **Linux** | **x86_64** (`amd64`) | `EWR-<tag>-linux-x86_64.tar.gz` | `ewr`, `database.json` |
| **Linux** | **aarch64** (`arm64` / Raspberry Pi) | `EWR-<tag>-linux-aarch64.tar.gz` | `ewr`, `database.json` |
| **Linux** | **armhf** (32-bit ARM / Raspberry Pi) | `EWR-<tag>-linux-armhf.tar.gz` | `ewr`, `database.json` |
| **macOS** | **Universal** (`arm64` + `x86_64`) | `EWR-<tag>-macos-universal.tar.gz` | `ewr`, `database.json` |
| **Android** | **Android 8.0+** | `EWR-<tag>-android.apk` | Full Android App (USB OTG + Wi-Fi) |
| **Linux Packages** | `deb` / `rpm` | `ewr_<ver>_<arch>.deb` / `.rpm` | System package with udev rules |
| **Unified SDK** | All Platforms | `EWR-SDK-<tag>.zip` | Headers, docs, and prebuilt `lib/` for all OSes |

### Verifying your download
Release archives carry signed build-provenance attestations via GitHub SLSA Sigstore. You can verify the cryptographic integrity of any downloaded package using the [GitHub CLI](https://cli.github.com/):

```bash
gh attestation verify EWR-<tag>-windows-x64.zip -R RxNaison/Epson-Waste-Reset
```

A `✓ Verification succeeded!` confirmation guarantees that the binary was built directly on GitHub Actions without tampering.

---

## 🚀 Usage

### Option A: Graphical Desktop App (`ewr_gui`)
1. Download the Windows package and unzip it anywhere. Keep `ewr_gui.exe` and `database.json` in the same folder.
2. Turn your Epson printer on and connect it via USB (or ensure it is on the same local network).
3. Double-click `ewr_gui.exe`. The printer is auto-detected. Click **Reset Waste Ink Counters** and confirm the safety prompt.

### Option B: Command Line Interface (`ewr`)
1. Download the latest version from the Releases page and unzip it. Keep `ewr` and `database.json` together.
2. Turn your Epson printer on and connect it **via USB**, or have it on the same network as your computer: when nothing answers on USB, EWR searches the network and asks which printer to use. `--ip <address>` goes straight to one.
3. Run it:
   * **Windows:** double-click `ewr.exe`
   * **Linux / macOS:** `sudo ./ewr` *(raw USB access requires root permissions without udev rules)*
4. EWR detects the printer and offers the matching database entry. Press Enter to accept it, or type part of a model name to search for another.
5. It reads the printer's status and current counter values and prints them, then asks for confirmation once. Answer `y`.
6. It saves a backup of the bytes it is about to overwrite to `backups/`, writes, then reads the counters back and tells you whether every write verified.
7. Turn the printer off and on with its physical power button.

---

## ⚙️ CLI Options & Capabilities

| Command Option | Description |
| :--- | :--- |
| `ewr` | Interactive: detect the printer, pick the model from a menu, confirm, reset. |
| `--status` | Read-only: print status, current waste pad and ink counters, then exit. Safe to run at any time. |
| `--model <Name>` | Name the model explicitly (e.g. `--model L3150`), skipping detection and the menu. |
| `--ip <address>` | Connect to a network printer over SNMP (UDP 161) instead of searching USB first. |
| `--dry-run` | Show every EEPROM packet that would be sent without writing anything. |
| `--dump` | Read the entire accessible EEPROM to a timestamped file (`ewr_dump_<Model>_<time>.txt`). |
| `--restore <file>` | Writes back what a backup from `backups/`, or an `--dump` file, holds of this model's counter bytes — only those, only the ones that differ. Same gates, confirmation and read-back as a reset; `--dry-run` shows the plan. |
| `--info` | Read-only: firmware version, serial number, the printer's own counters (pages printed, cleanings, ink changes, scans) and the codes of its last fatal errors, where the database knows them. |
| `--nozzle-check` | Print the nozzle check pattern. Load plain paper. `--nozzle-check alt` asks for the alternate pattern some models print. |
| `--head-clean [level]` | A head cleaning: `gentle`, `normal` (the default), `medium` or `strong`, as far as the model lists them. `--clean-group black` or `color` cleans only those nozzles. Uses ink, and the ink goes into the waste pads. |
| `--power-clean` | The ink charge: a much stronger cleaning that uses a lot of ink. Only when normal cleanings did not clear the nozzles. Asks you to type `charge`. |
| `--print-data` | Send a cleaning as print data (ESC/P2 remote mode) even where the model has its own maintenance command — for a printer that refuses that one. |
| `--find-addresses` | For a model with no reset addresses yet: read the EEPROM three times with a head cleaning between reads — EWR runs them, or you do from the panel — and report the bytes that rise every time. No EEPROM byte is written. |
| `--find-key` | For a printer the database has no entry for: try every read key in the database and report which one, if any, the printer answers. Takes no `--model`. Read-only. |
| `--all-keys` | With `--find-key`, and only if no database key fits and the printer answered them at all: sweep every 2-byte key, families closest to the known ones first, and stop at the first that answers. Read-only. |
| `--yes`, `-y` | Answer the reset confirmation with yes, for callers driving EWR non-interactively. |
| `--force-yes` | `--yes`, and overrule the gates that would stop it: a model mismatch, a printer error, a database conflict. Last resort. |
| `--cartridge`, `-c` | Reset the cartridge ink levels instead of the waste ink pads, on models that carry a per-color ink map. |
| `--no-update` | Fully offline: no update check, no download, `database.json` left as it is. Use it while editing `database.json`. |
| `--usb-soft-reset` | Diagnostic only, Windows. Resets the USB channel once at the start of the run, then waits (up to 90 s) for the printer to finish re-initialization. |
| `--json` | Machine-readable output: one JSON object per line on stdout, for callers driving EWR from another language. |
| `--help`, `-h` | The same list, from the binary. |

On a terminal, EWR colors its output: message tags, ink and waste pad gauges, the printer's state. Set `NO_COLOR=1` to turn that off. Piped output, log files and `--json` never carry color codes.

### Unattended runs: `--yes`

```bash
ewr --yes                      # reset the printer that answers, unattended
ewr --model L3150 --yes        # ... or name the model yourself
```

`--yes` answers one question: the final *"Reset the waste ink pad counters now?"*. Everything else about the run is unchanged — the preflight read, the read-back verification, and every gate that can stop it.

Without `--model` it takes the model the printer reports over IEEE 1284, the same entry the menu offers on Enter, and stops if the printer names nothing the database recognises. What it deliberately does **not** do:
* **It does not overrule the detected model.** If `--model` names something other than the printer that answered, the run stops instead of writing.
* **It does not push past a printer error or a database conflict.** Those gates report what the printer said, not what you intended, so `--yes` declines them and exits non-zero.
* **It never arms the cartridge ink reset by itself.** On a model that offers both, `--yes` takes the waste ink pads. Reaching the ink reset takes `--cartridge`, named explicitly.
* **It does not search the menu for you.** It takes the detected model or it stops; it never falls back to picking an entry from a list.

Exit status is `0` when the reset completed and verified, `1` when it was stopped or failed, `2` for a bad command line. Writes the printer acknowledged but did not keep are a failure: the read-back shows the old values, and the run says so instead of claiming success.

Two EWR runs must not drive one printer simultaneously: they share the USB handle and take each other's replies. A second run stops with a message instead of starting.

### Driving EWR from another language: `--json`

`--json` turns stdout into one JSON object per line: a `hello`, an `event` per step, and one `result` carrying the outcome. Any language that can start a process can use it:

```python
import json, subprocess

run = subprocess.run(
    ["./ewr", "--model", "L3150", "--yes", "--json", "--no-update"],
    stdout=subprocess.PIPE, stdin=subprocess.DEVNULL, text=True,
)

for line in run.stdout.splitlines():
    msg = json.loads(line)
    if msg.get("type") == "result":
        if msg["ok"]:
            print("Reset succeeded! Verified writes:", msg["data"]["writes"]["verified"])
        else:
            print("Reset failed:", msg.get("error"))
```

To put the reset *inside* your program instead of spawning it — a GUI, a service, a language binding — there is a C ABI in `include/ewr/ewr_c.h`, built as `ewrc.dll` / `libewrc.so` alongside the CLI. Every release carries it prebuilt in `EWR-SDK-<tag>.zip`, with `lib/<platform>/` folders beside the header.

### Undoing a reset: backups and `--restore`

Every reset — and every restore — first saves the bytes it is about to overwrite, as the preflight read them, to `backups/` next to `ewr` (or to your user folder when EWR cannot write there: `%LOCALAPPDATA%\EWR`, `~/.local/share/ewr`, `~/Library/Application Support/EWR`). The path is printed before the first write. If the backup cannot be saved, or the printer did not answer for every byte, EWR stops and asks rather than writing something it could not undo.

```bash
ewr --restore backups/L3150_X5NA012345_20261024T183651Z.json --dry-run   # what it would write
ewr --restore backups/L3150_X5NA012345_20261024T183651Z.json             # put the bytes back
ewr --restore ewr_dump_L3150_1736900000.txt                              # from a dump, any version
```

A restore writes only the bytes a reset of that model writes — its waste and ink counters — and only those that differ from what the printer holds now. Restoring an old backup puts the counters back where they were.

### Maintenance: nozzle check and head cleaning

```bash
ewr --nozzle-check                 # print the pattern (load plain paper)
ewr --head-clean                   # a normal cleaning
ewr --head-clean strong --clean-group black
ewr --power-clean                  # the ink charge - a lot of ink
ewr --info                         # firmware, serial, page and cleaning counters
```

These write no EEPROM byte, but they act on the printer: a cleaning uses ink and fills the waste pads. So they run the way a reset does — the printer's state is read first, a printer that reports an error is not sent anything, and EWR watches the printer's status until it is idle again, then shows what the cleaning did to the pad counters.

---

## 🛠️ Building from Source

### Prerequisites
* **Windows (x64 or ARM64):** Visual Studio 2019/2022/2026 with MSVC C++ build tools. `libusb` is fetched and built automatically by CMake; it is linked statically and `ewr.exe` / `ewr_gui.exe` ship standalone.
* **Linux (Ubuntu, Debian, Fedora, openSUSE, Arch, Raspberry Pi OS):**
  - **Ubuntu / Debian / Raspberry Pi OS:** `sudo apt-get install build-essential cmake git pkg-config libcurl4-openssl-dev` (or `libcurl4-gnutls-dev`)
  - **Fedora / RHEL / CentOS:** `sudo dnf install gcc-c++ cmake git pkgconf libcurl-devel`
  - **Arch Linux:** `sudo pacman -S base-devel cmake git curl`
  *(Note: If system `libusb-1.0` is not installed, CMake automatically falls back to fetching and statically linking the bundled libusb).*
* **macOS:** `brew install cmake pkgconf` — libusb is built from the pinned source and libcurl comes with the system. Universal binary with `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"`.
* **Android:** Android Studio, or SDK with NDK `27.0.12077973` and JDK 21: `cd android && ./gradlew assembleDebug`.

### Build Steps

```bash
# 1. Configure CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 2. Build executables (Release mode)
cmake --build build --config Release

# 3. Optional: Install system-wide (Linux / Raspberry Pi)
sudo cmake --install build
```

On Windows, this builds both `ewr.exe` (CLI) and `ewr_gui.exe` (GUI) statically linked in the repository root.

---

## 🔍 Troubleshooting & Diagnostic Guidelines

If your printer fails to connect or the reset does not take effect:

1. **Check `ewr_trace.log`**: Every run writes a full communication trace to `ewr_trace.log` beside the binary. Include this log in issue reports.
2. **Clear Print Queues**: An active print job waiting in the Windows Spooler or CUPS will lock printer communications. Open your system print queue and cancel all pending documents.
3. **Close Companion Software**: Close Epson Status Monitor, Epson Scan, Epson Event Manager, and third-party printer utilities.
4. **Use Direct USB Connections**: Connect directly to a motherboard USB port (avoid unpowered USB hubs or extension cables).
5. **Install Manufacturer Drivers**: If Windows 11 installed the generic `Microsoft IPP Class Driver`, remove it in Printers & Scanners and install the genuine Epson driver from [epson.com/support](https://epson.com/support).
6. **Physical Power Cycle**: Always turn the printer OFF and back ON using its physical power button after resetting.

---

## 🤝 Contributing & Adding New Models

Adding or updating printer models is done by editing `database.json` and submitting a Pull Request — no C++ code modification is required.

* Run `ewr --find-key` to test read keys against unmapped printers.
* Run `ewr --model <Name> --find-addresses` to map unknown EEPROM counter locations.
* Review **[CONTRIBUTING.md](CONTRIBUTING.md)** for database schema documentation and contribution guidelines.

---

## 👏 Credits & Acknowledgements

* **[RxNaison](https://github.com/RxNaison)** — Author of the original [Epson-Waste-Reset](https://github.com/RxNaison/Epson-Waste-Reset) project, architecture, protocol engine, and curated database.
* **[reinkpy](https://codeberg.org/atufi/reinkpy)** — Primary reference for printer models and protocol coverage.
* **[ez-reset](https://github.com/CiRIP/ez-reset)** — Counter byte mappings, service limits, and commit procedures.
* **[reink](https://github.com/lion-simba/reink)** — Original reverse engineering of legacy Stylus protocols.
* **[Gutenprint](https://gutenprint.sourceforge.net/)** — Model naming taxonomy and identification aliases.
* **[Dear ImGui](https://github.com/ocornut/imgui)** — Bloat-free immediate mode graphical user interface library for C++.

---

## ⚠️ Disclaimer
Manipulating printer hardware via direct EEPROM writes carries inherent risks. EWR is provided "as is" without warranty of any kind. Always ensure the physical waste ink pads are cleaned, inspected, or replaced to avoid physical ink leakage.
