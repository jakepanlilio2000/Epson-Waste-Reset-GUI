# EWR (Epson Waste Reset) — GUI & Multi-Platform Edition

[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-blue)](https://github.com/RxNaison/Epson-Waste-Reset)
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
> 1. **Interactive Native C++ Desktop GUI (`ewr_gui`)**: A responsive, lightweight, compact desktop interface built on Dear ImGui and DirectX 11 / Win32. Features real-time visual progress gauges, dynamic threshold coloring, instant searchable model picker (1,450+ models), LAN SNMP network printer discovery, EEPROM hex inspector, and safety-verified one-click reset flows.
> 2. **Multi-Architecture & Multi-OS CI/CD Pipeline**: Automated GitHub Actions release pipeline building standalone, self-contained packages for **Windows, Linux, and macOS** across **x86, x64, ARM (armv7hf), and ARM64 (aarch64)** architectures, as well as Universal macOS binaries and C ABI SDKs.
> 3. **Non-Blocking Architecture**: Fully asynchronous background engine execution with live status and trace callbacks.
> 4. **100% Core Compatibility**: Retains the exact same safety preflight, read-back verification lifecycle, and C ABI as upstream.

---

## 📦 Downloads & Supported Platforms

Every release is packaged into self-contained archives bundled with `database.json`. Windows packages include both the command-line interface (`ewr.exe`) and the graphical desktop application (`ewr_gui.exe`).

| Platform / Operating System | Architecture | Target Package | Contents | Developer C ABI SDK |
| :--- | :--- | :--- | :--- | :--- |
| **Windows** | **x64** (64-bit Intel/AMD) | `EWR-v1.6.0-win-x64.zip` | `ewr.exe`, `ewr_gui.exe`, `database.json` | `EWR-SDK-v1.6.0-win-x64.zip` |
| **Windows** | **x86** (32-bit Win32) | `EWR-v1.6.0-win-x86.zip` | `ewr.exe`, `ewr_gui.exe`, `database.json` | `EWR-SDK-v1.6.0-win-x86.zip` |
| **Windows** | **ARM64** (Surface Pro X, Snapdragon) | `EWR-v1.6.0-win-arm64.zip` | `ewr.exe`, `ewr_gui.exe`, `database.json` | `EWR-SDK-v1.6.0-win-arm64.zip` |
| **Linux** | **x64** (`x86_64`) | `EWR-v1.6.0-linux-x64.tar.gz` | `ewr`, `database.json` | `EWR-SDK-v1.6.0-linux-x64.tar.gz` |
| **Linux** | **x86** (`i686` 32-bit) | `EWR-v1.6.0-linux-x86.tar.gz` | `ewr`, `database.json` | `EWR-SDK-v1.6.0-linux-x86.tar.gz` |
| **Linux** | **ARM64** (`aarch64` / Raspberry Pi 3, 4, 5) | `EWR-v1.6.0-linux-arm64.tar.gz` | `ewr`, `database.json` | `EWR-SDK-v1.6.0-linux-arm64.tar.gz` |
| **Linux** | **ARM** (`armv7hf` 32-bit / Raspberry Pi) | `EWR-v1.6.0-linux-arm.tar.gz` | `ewr`, `database.json` | `EWR-SDK-v1.6.0-linux-arm.tar.gz` |
| **macOS** | **Apple Silicon** (`arm64` M1/M2/M3/M4) | `EWR-v1.6.0-macos-arm64.tar.gz` | `ewr`, `database.json` | `EWR-SDK-v1.6.0-macos-arm64.tar.gz` |
| **macOS** | **Intel** (`x86_64`) | `EWR-v1.6.0-macos-x64.tar.gz` | `ewr`, `database.json` | `EWR-SDK-v1.6.0-macos-x64.tar.gz` |
| **macOS** | **Universal** (`arm64` + `x86_64` Fat Binary) | `EWR-v1.6.0-macos-universal.tar.gz` | `ewr`, `database.json` | `EWR-SDK-v1.6.0-macos-universal.tar.gz` |

### Verifying your download
Release archives carry signed build-provenance attestations via GitHub SLSA Sigstore. You can verify the cryptographic integrity of any downloaded package using the [GitHub CLI](https://cli.github.com/):

```bash
gh attestation verify EWR-v1.6.0-win-x64.zip -R <your-repo-or-RxNaison/Epson-Waste-Reset>
```

A `✓ Verification succeeded!` confirmation guarantees that the binary was built directly on GitHub Actions without tampering.

---

## 🖥️ Interactive Desktop GUI (`ewr_gui`)

The C++ graphical interface (`ewr_gui.exe`) brings all diagnostic and maintenance capabilities into a modern, compact, dark-mode desktop window.

### Key GUI Features
* **Real-time Waste Ink Gauges**: Visual progress bars display current counter values, service limits, and exact percentages for all configured waste pad groups (Main Waste Ink Pad, Platen Pad).
* **Smart Dynamic Color Coding**:
  - 🟢 **Normal (`< 80%`)**: Healthy operation.
  - 🟡 **Warning (`80% – 99%`)**: Maintenance due soon.
  - 🔴 **Critical (`100%`)**: Service limit reached (printer locked).
* **Cartridge Ink Levels**: Dedicated visualizer for models exposing per-color ink maps (Black, Cyan, Magenta, Yellow, Light Cyan, Light Magenta, etc.).
* **Searchable Model Database**: Instant filter and selector popup searching across 1,450+ models and aliases with fuzzy search.
* **Network Printer Discovery**: Broadcasts SNMP UDP queries across local subnets to discover Wi-Fi and Ethernet Epson printers in seconds.
* **EEPROM Memory Inspector**: Live 16-byte monospace hex viewer with ASCII decode, supporting exports to raw binary (`.bin`) and JSON (`.json`).
* **One-Click Maintenance Actions**:
  - `Reset Waste Ink Pad Counters`: Complete guided safety lifecycle (preflight check -> safety confirmation dialog -> EEPROM write -> read-back verification).
  - `Reset Ink Levels`: Rewrites ink cartridge accounting flags on compatible models.
  - `Dry Run / Simulation`: Simulates planned write sequences without modifying EEPROM hardware.
  - `Dump EEPROM`: Creates a full timestamped memory snapshot.
* **Live Event & Trace Log**: Color-coded log streaming real-time engine packets, with filter levels, auto-scroll, and one-click copy to clipboard.
* **Native Windows Polish**: Windows 11 / 10 Immersive Dark title bar (`DWMWA_USE_IMMERSIVE_DARK_MODE`), high-DPI scaling, and responsive auto-stacking layouts.

### Running the GUI
On Windows, simply double-click `ewr_gui.exe` (keep it in the same directory as `database.json`).

---

## ⚡ Core Features

* **Reads before it writes, and verifies after.** Every reset runs the strict safety lifecycle: read current counters, refuse to continue if the printer reports a blocking error, prompt once, execute EEPROM writes, then read the counters back to verify they match target reset values.
* **Smart Protocol Engine:** Constructs exact EEPROM write packets (`|B`) on the fly for your specific model while properly honoring IEEE 1284.4 (D4) flow-control credits.
* **~1,450 Printer Models Supported:** From classic Stylus Photo R-series to current EcoTank (ET), L-series, XP, and WorkForce (WF) lines.
* **Automatic USB & Network Discovery:** Discovers USB printers via IEEE 1284 device IDs and networked printers over SNMP (UDP 161).
* **OTA Database Updates:** Automatically checks GitHub at startup for database updates while retaining full offline capabilities via the bundled `database.json`.
* **Zero Custom Drivers Required:**
  - **Windows:** Native Win32 `SetupAPI` with asynchronous `OVERLAPPED` I/O to drain the Windows Print Spooler, plus statically linked `libusb` for vendor-specific USB endpoints.
  - **Linux & macOS:** Automatically detaches kernel drivers (CUPS) for raw hardware access.
* **Trace Logging:** Produces `ewr_trace.log` recording every raw byte exchanged, making diagnostic troubleshooting simple.

---

## 🚀 Command-Line Usage (CLI)

1. Download the archive for your operating system from the Releases page and extract it. Keep `ewr` (or `ewr.exe`) and `database.json` together.
2. Turn your printer on and connect it via **USB** (or have it connected to the same local Wi-Fi / Ethernet network).
3. Run the executable:
   - **Windows:** `ewr.exe`
   - **Linux / macOS:** `sudo ./ewr` *(raw USB access requires root permissions)*
4. EWR detects the printer and proposes the matching model profile. Press **Enter** to accept or type a model name to search.
5. Review the reported waste ink pad percentage.
6. When prompted, type `y` to execute the reset.
7. **Turn the printer OFF and back ON using its physical power button** to commit changes to hardware.

### CLI Options

| Option | What it does |
| :--- | :--- |
| `--status`, `-s` | Read-only check: printer status, ink levels, and waste counter values. Sends no writes. |
| `--list`, `-l` | List all detected Epson USB interfaces and network printers with IEEE 1284 IDs and database matches. |
| `--model <name>` | Specify target model directly (e.g. `--model "L3150"` or `--model "ET-2800 Series"`). |
| `--ip <address>` | Connect to printer over network via SNMP (UDP 161): `ewr --ip 192.168.1.100`. |
| `--ip auto` | Scan network and prompt to choose from discovered printers. |
| `--dry-run` | Perform preflight detection and show planned writes without touching EEPROM. |
| `--dump` | Dump printer EEPROM memory to a timestamped file for analysis. |
| `--find-addresses`| Diagnostic helper: reads EEPROM three times across cleaning cycles to discover unknown waste counter bytes. |
| `--find-key` | Test all ~130 read keys in the database against an unmapped model to identify compatible protocol keys. |
| `--yes`, `-y` | Answer confirmation automatically for unattended script automation. |
| `--force-yes` | Force execution past model mismatch or non-fatal warning gates (last resort). |
| `--cartridge`, `-c` | Reset cartridge ink accounting flags instead of waste ink pads. |
| `--no-update` | Disable OTA GitHub database update check (offline mode). |
| `--usb-soft-reset` | Windows diagnostic: issue a USB bus soft reset before beginning session. |
| `--json` | Output machine-readable JSON line stream for automated integration. |
| `--help`, `-h` | Display help and argument list. |

### Driving EWR Programmatically: `--json`

The `--json` flag formats stdout as newline-delimited JSON objects (`hello`, `event`, `result`). See [docs/json-output.md](docs/json-output.md) for the specification.

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

---

## 🛠️ Building from Source

### Prerequisites
* **Windows:** Visual Studio 2019/2022/2026 with C++ Desktop Development tools. CMake 3.15+.
* **Linux (Debian/Ubuntu):** `sudo apt-get install build-essential cmake pkg-config libusb-1.0-0-dev libcurl4-openssl-dev`
* **Linux (Arch):** `sudo pacman -S base-devel cmake libusb curl pkgconf`
* **macOS:** `brew install cmake libusb curl pkgconf`

### Build Steps

```bash
# 1. Configure CMake
cmake -B build -DCMAKE_BUILD_TYPE=Release

# 2. Build executables (Release mode)
cmake --build build --config Release
```

On Windows, this produces both `ewr.exe` (CLI) and `ewr_gui.exe` (GUI) statically linked in the repository root.

### Running the Test Suite
EWR includes comprehensive unit tests verifying packet generators, protocol parsers, and safety checks:

```bash
cmake -B build -DEWR_BUILD_TESTS=ON
cmake --build build --config Release
ctest --test-dir build --output-on-failure -C Release
```

---

## 🔍 Troubleshooting & Diagnostic Guidelines

If your printer fails to connect or the reset does not take effect:

1. **Check `ewr_trace.log`**: Every run writes a full communication trace to `ewr_trace.log` beside the binary. Include this log in issue reports.
2. **Clear Print Queues**: An active print job waiting in the Windows Spooler or CUPS will lock printer communications. Open your system print queue and cancel all pending documents.
3. **Close Companion Software**: Close Epson Status Monitor, Epson Scan, and third-party printer utilities.
4. **Use Direct USB Connections**: Connect directly to a motherboard USB port (avoid unpowered USB hubs or extension cables).
5. **Physical Power Cycle**: Always turn the printer OFF and back ON using its physical power button after resetting.

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
