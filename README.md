<p align="center">
  <img src="resources/logo/app_icon.png" alt="Filo" width="128">
</p>

<h1 align="center">Filo</h1>

<p align="center">
  <strong>One screen. One chain. No DAW.</strong><br>
  A standalone AU/VST3 plugin host for guitarists, on macOS, Windows and Linux.
</p>

<p align="center">
  <img src="https://img.shields.io/badge/macOS-12%2B-black?style=flat-square&logo=apple" alt="macOS 12+">
  <img src="https://img.shields.io/badge/Windows-10%2B-blue?style=flat-square&logo=windows" alt="Windows 10+">
  <img src="https://img.shields.io/badge/Linux-x86__64-yellow?style=flat-square&logo=linux" alt="Linux x86_64">
  <img src="https://img.shields.io/badge/hosts-AU%20%C2%B7%20VST3-blue?style=flat-square" alt="AU · VST3">
  <img src="https://img.shields.io/badge/C%2B%2B-17-orange?style=flat-square&logo=cplusplus" alt="C++17">
  <img src="https://img.shields.io/badge/license-MIT-green?style=flat-square" alt="MIT License">
</p>

---

## The idea

Guitarists who want to plug in and play through a couple of plugins shouldn't have to launch a DAW. DAWs are overkill for the basic case: connect the guitar, drop a chain of effects, jam over a YouTube video.

The open-source alternatives (Carla, Pedalboard2, Element) are powerful but too dense for someone who just wants a guitar tone in two clicks. **Filo** is the opposite: one window, one chain, your plugins — nothing else.

## What's inside

- **Single-screen UI** — input device, output device, buffer size, plugin chain, and stereo level meters. Nothing else to learn.
- **AU + VST3 hosting** — loads any Audio Unit or VST3 installed in the standard system folders.
- **Drag & drop chain** — reorder plugins by dragging rows; signal flows top to bottom.
- **One-click bypass** — every slot has a bypass toggle that swaps in JUCE's native bypass without rebuilding the graph.
- **Native plugin UIs** — double-click any row to open the plugin's own editor in a floating window.
- **Out-of-process VST3 scanning** — a broken or hostile VST3 cannot take the host down. AU is scanned in-process (already sandboxed by `audiocomponentd`), VST3 in a child worker that JUCE auto-blacklists on crash.
- **Glitch-free chain edits** — `AudioProcessorGraph` connections are rebuilt on the message thread, with bypass and remove dispatched async to avoid dangling-pointer races.
- **Persistent setup** — selected devices, buffer size, sample rate, and plugin list are saved across launches.
- **dB level meters** — RMS bar with peak marker on input and output, 30 Hz refresh.

## Signal chain

```
Guitar In  →  [Plugin 1]  →  [Plugin 2]  →  …  →  [Plugin N]  →  Output
                  │             │                      │
              bypass / UI   bypass / UI           bypass / UI
```

Plugins run in series, top to bottom. Each row in the UI corresponds to one node in the graph.

## Download

Grab the latest pre-built app from the [Releases page](https://github.com/tondo-audio/filo/releases/latest).

### macOS

1. Download `Filo-vX.Y.Z-macOS.dmg` and open it.
2. Drag `Filo.app` into `/Applications`.
3. First launch: right-click `Filo.app` → **Open** to bypass Gatekeeper (Filo ships unsigned).
4. Grant microphone access when prompted — required to receive audio from your interface.

> Filo loads plugins from the standard macOS locations:
> `/Library/Audio/Plug-Ins/Components` and `~/Library/Audio/Plug-Ins/Components` (AU)
> `/Library/Audio/Plug-Ins/VST3` and `~/Library/Audio/Plug-Ins/VST3` (VST3)

### Windows

1. Download `Filo-vX.Y.Z-Windows.zip` and unzip it.
2. Run `Filo.exe` directly, or move it anywhere on disk and create a shortcut.
3. SmartScreen may ask for manual confirmation on first launch (Filo ships unsigned).

> Filo loads VST3 plugins from `C:\Program Files\Common Files\VST3`. Audio Unit hosting is macOS-only.

### Linux

1. Download `Filo-vX.Y.Z-Linux.zip` and unzip it.
2. Run `./Filo` from a terminal (requires ALSA or JACK at runtime).

> Filo loads VST3 plugins from `~/.vst3`, `/usr/lib/vst3` and `/usr/local/lib/vst3`. Audio Unit hosting is macOS-only.

## Build from source

> Requires CMake 3.22+, a C++17 compiler, and one of: macOS 12+, Windows 10+ (x64) with MSVC, or Linux x86_64 (with `libasound2-dev`, `libjack-jackd2-dev`, `libcurl4-openssl-dev`, `libfreetype-dev`, `libx11-dev`, `libxcomposite-dev`, `libxcursor-dev`, `libxext-dev`, `libxinerama-dev`, `libxrandr-dev`, `libxrender-dev`, `libglu1-mesa-dev`, `mesa-common-dev`). JUCE 8.0.6 is downloaded automatically on first configure via CPM.cmake.

```bash
# Configure & build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

After build, the standalone executable lands here:

| Platform | Path |
|---|---|
| macOS | `build/Filo_artefacts/Release/Filo.app` (with multi-resolution `Icon.icns` generated from `resources/logo/app_icon.png`) |
| Windows | `build\Filo_artefacts\Release\Filo.exe` |
| Linux | `build/Filo_artefacts/Release/Filo` |

## Architecture at a glance

| Class | Role |
|---|---|
| `FiloApplication` (`Main.cpp`) | Entry point. Owns `AudioDeviceManager`, `AudioEngine`, `PluginManager`, and the persistent `PropertiesFile`. Idempotent shutdown so a worker-mode launch doesn't crash on teardown. |
| `AudioEngine` (`src/audio/`) | Wraps a `juce::AudioProcessorGraph` plus `AudioProcessorPlayer`. Holds dedicated `LevelMeasurer` nodes on input and output, and rebuilds connections when the chain changes. Exposes atomic peak/RMS for the UI. |
| `PluginManager` (`src/plugins/`) | Async plugin scan and load. Owns `AudioPluginFormatManager` (AU + VST3) and a `KnownPluginList` persisted to settings. |
| `OutOfProcessScanner` (`src/plugins/PluginScannerSubprocess.*`) | `KnownPluginList::CustomScanner` that runs VST3 scans inside a `ChildProcessCoordinator`/`Worker` pair. The worker installs a Mach exception handler so a crashing VST3 dies cleanly and JUCE auto-blacklists the path. AU is scanned in-process. |
| `MainComponent` (`src/`) | Single-screen layout. Drives a 30 Hz `Timer` that pulls meter values from the engine. `DragAndDropContainer` for chain reordering. |
| `PluginChainView` (`src/ui/`) | Renders one `PluginRowComponent` per chain entry. Handles bypass, remove, double-click-to-open-UI, and drag-and-drop reorder. |
| `DeviceSelectorBar` (`src/ui/`) | Compact combo boxes for input device, output device, sample rate, and buffer size. |

## Tech stack

| | |
|---|---|
| **Framework** | [JUCE 8](https://juce.com/) |
| **Audio graph** | `juce::AudioProcessorGraph` + `AudioProcessorPlayer` |
| **Plugin hosting** | `AudioPluginFormatManager` (AU + VST3) + `KnownPluginList` |
| **VST3 sandbox** | `juce::ChildProcessCoordinator` / `ChildProcessWorker` |
| **Build** | CMake + [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake) |

## License

MIT — see [LICENSE](LICENSE).

---

<p align="center">
  Built by <a href="https://github.com/tondo-audio">Tondo Audio</a> · Source at <a href="https://github.com/tondo-audio/filo">tondo-audio/filo</a>
</p>
