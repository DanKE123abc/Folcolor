# Folcolor - Agent Guide

## What This Is
Windows folder color customization utility. Single Win32 C/C++ executable (~1MB), no shell extension, requires admin (UAC).

## Build
```
msbuild src/Controller/Controller.sln /p:Configuration=Release /p:Platform=x86 /p:PlatformToolset=v143
```
Output: `src/Controller/Win32/Release/Folcolor.exe`

- Toolset: **v143** (VS 2022). Only **Win32/x86** platform — no x64 build.
- Release uses `/MT` (static CRT), `/O1` (minimize size), whole-program optimization.
- Debug uses dynamic CRT, no optimization.
- No precompiled headers (`NotUsing`).

## Directory Boundaries
| Path | Purpose |
|---|---|
| `src/Controller/` | C++ app source — all .cpp/.h, resources, VS project/solution |
| `src/Generator/` | Python scripts + base icons for generating color icon sets |
| `readme_assets/` | Images/GIFs for README |

## Entry Points
- `src/Controller/main.cpp` — `wWinMain`, dialog UI, install/uninstall logic
- `src/Controller/Installer.cpp` — registry and file install/uninstall
- `src/Controller/FolderColorize.cpp` — context menu action: applies color to a folder
- `src/Controller/Utility.cpp` — helpers (icon cache reset, window focus, etc.)
- `src/Controller/Language.cpp` — i18n (detects system language)

## Runtime Behavior
- Installs to `%ProgramFiles%\Folcolor\`
- Registers context menu at `HKCR\Directory\shell\Folcolor`
- Invoked from context menu as: `Folcolor.exe --index=N --folder="path"`
- Detects OS version via `RtlGetVersion` to pick icon set: Win11 (build >= 22000), Win10, or Win7/8
- Single-instance app (brings existing window to focus if already running)
- Version constant in `StdAfx.h`

## Icon Regeneration (Python)
```
cd src/Generator
python BatchProcess.py
```
Requires: Python 3.x + `pillow`. Reads base `.ico` files in `src/Generator/`, writes colored variants to `src/Controller/Resources/Win10set/`, `Win11set/`, `Win7_8set/`.

## CI
`.github/workflows/build.yml` — runs on push/PR to `main`/`master`, builds Release x86, uploads artifact.

## Style Conventions
- Win32 API, raw C++ — no frameworks, no STL containers in visible code
- `StdAfx.h` is a manual include hub (not PCH) — add shared includes there
- `resource.h` and `.rc` files define UI layout; resource IDs prefixed with `IDC_` / `IDI_` / `IDD_`
