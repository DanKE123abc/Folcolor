# Folcolor - Agent Guide

## What This Is
Windows folder color customization utility. A small Win32 C/C++ executable
(~1MB) plus an optional out-of-process Windows 11 context-menu DLL. The app
requires admin (UAC) because it writes `HKCR` and installs under Program Files.

## Build
```
msbuild src/Controller/Controller.sln /p:Configuration=Release /p:Platform=x86 /p:PlatformToolset=v143
```
Output: `src/Controller/Win32/Release/Folcolor.exe` (and the x86 menu DLL).

The Windows 11 menu DLL must match the machine architecture, not the app:

```
msbuild src/Controller/MenuDLL/MenuDLL.vcxproj /p:Configuration=Release /p:Platform=x64   /p:PlatformToolset=v143
msbuild src/Controller/MenuDLL/MenuDLL.vcxproj /p:Configuration=Release /p:Platform=ARM64 /p:PlatformToolset=v143   (optional)
```
Outputs: `src/Controller/x64/Release/FolcolorMenu.dll`,
`src/Controller/ARM64/Release/FolcolorMenu.dll`.

Then build the signed sparse package:
```
powershell -ExecutionPolicy Bypass -File src/Controller/SparsePackage/build-sparse-pkg.ps1
```
The script:
* requires `Win32\Release\Folcolor.exe` and `x64\Release\FolcolorMenu.dll`
* patches manifest version/publisher from the signing certificate
* packs `Win32\Release\sparse-pkg\` with MakeAppx (`/nv`)
* signs with `Folcolor.pfx` and exports `Win32\Release\Folcolor.cer`
* copies `FolcolorMenu.x64.dll` / `.x86.dll` / `.arm64.dll` next to the exe

Signing uses `FOLCOLOR_PFX_BASE64` + `FOLCOLOR_PFX_PASSWORD` when set
(CI secret); otherwise a local self-signed certificate is generated. CI does
this automatically and uploads the release bundle (exe + DLLs + appx + cer +
zip).

- Toolset: **v143** (VS 2022). The app is **Win32/x86 only**; MenuDLL builds
  Win32/x64/ARM64.
- Release uses `/MT` (static CRT), `/O1` (minimize size), whole-program optimization.
- Debug uses dynamic CRT, no optimization.
- No precompiled headers (`NotUsing`).

## Directory Boundaries
| Path | Purpose |
|---|---|
| `src/Controller/` | C++ app source — all .cpp/.h, resources, VS project/solution |
| `src/Controller/MenuDLL/` | Win11 modern menu COM DLL (`IExplorerCommand` + subcommands). Builds into `FolcolorMenu.dll` |
| `src/Controller/SparsePackage/` | AppxManifest, cert + appx build scripts |
| `src/Controller/SparsePkg.cpp` | Runtime copy/register/unregister of the sparse identity package, Win11 toggle, Explorer restart |
| `src/Generator/` | Python scripts + base icons for generating color icon sets |
| `readme_assets/` | Images/GIFs for README |

## Entry Points
- `src/Controller/main.cpp` — `wWinMain`, dialog UI, command-line parsing,
  install/uninstall logic, `g_isWin11`
- `src/Controller/Installer.cpp` — registry and file install/uninstall
- `src/Controller/SparsePkg.cpp` — Win11 sparse-package install/uninstall,
  per-user registration, `--install-menu` / `--uninstall-menu` helper modes,
  Explorer restart
- `src/Controller/FolderColorize.cpp` — context menu action: applies color to a folder
- `src/Controller/Utility.cpp` — helpers (icon cache reset, window focus, etc.)
- `src/Controller/Language.cpp` — i18n (detects system language, owns `g_isWin11`)

## Runtime Behavior
- Installs to `%ProgramFiles%\Folcolor\`
- Registers classic context menus at `HKCR\Directory\shell\Folcolor` (all
  Windows) and `HKCU\Software\Classes\Directory\Background\shell\Folcolor`
  (Win11 background)
- On Win11 also copies `FolcolorMenu.dll` (matching machine architecture),
  `Folcolor-sparse.appx`, `Folcolor.cer` and `menu.ps1` next to the exe, then
  registers the sparse identity package **per user**:
  - `Folcolor.cer` → `Cert:\LocalMachine\TrustedPeople` (machine-wide; AppX
    signature validation runs in the machine-scoped service)
  - `Add-AppxPackage -ExternalLocation <install dir>`
  - when the process is elevated as a different account than the shell user,
    registration is delegated to `Folcolor.exe --install-menu` running with the
    shell user's token
- The modern menu registers only `desktop5:ItemType Type="Directory"`;
  right-clicking folder background is intentionally not registered (would
  require an `IFolderView` lookup).
- The "Win11 menu: On/Off" button stores `HKCU\Software\Folcolor\EnableWin11Menu`
  (default 1) and registers/unregisters immediately.
- After successful registration the user is asked to restart Explorer (the
  restart uses the shell user's token so Explorer does not become elevated).
- Invoked from context menu as: `Folcolor.exe --index=N --folder="path"`
  (repeated `--folder` arguments handle multi-selection in one process).
- Detects OS version via `RtlGetVersion` to pick icon set: Win11 (build >= 22000), Win10, or Win7/8
- Single-instance app for the UI only; context-menu actions bypass the
  single-instance check so they work while the dialog is open.
- Version constant in `StdAfx.h`

## Icon Regeneration (Python)
```
cd src/Generator
python BatchProcess.py
```
Requires: Python 3.x + `pillow`. Reads base `.ico` files in `src/Generator/`, writes colored variants to `src/Controller/Resources/Win10set/`, `Win11set/`, `Win7_8set/`.

## CI
`.github/workflows/build.yml` — runs on push/PR to `main`/`master`:
builds Release x86 (Controller + MenuDLL), builds the x64 (and optional ARM64)
menu DLL, runs `build-sparse-pkg.ps1`, creates `Folcolor-win11.zip`, and
uploads the full release bundle.

## Style Conventions
- Win32 API, raw C++ — no frameworks, no STL containers in visible code
  (the Win11 menu DLL and sparse-package code use `std::wstring`/`std::vector`)
- `StdAfx.h` is a manual include hub (not PCH) — add shared includes there
- `resource.h` and `.rc` files define UI layout; resource IDs prefixed with `IDC_` / `IDI_` / `IDD_`
- C++ sources with non-ASCII text are UTF-8. Both projects compile with `/utf-8`;
  `MenuStrings.h` also carries a UTF-8 BOM so CI runners with an English code page
  cannot turn menu labels into mojibake.
