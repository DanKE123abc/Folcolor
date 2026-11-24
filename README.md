# Folcolor™: The Windows folder color utility
## Open Source, 100% free, minimalist, and secure.

![GitHub](https://img.shields.io/github/license/kweatherman/Folcolor)
![GitHub Release Date](https://img.shields.io/github/release-date/kweatherman/Folcolor)

</br>

![Folcolor Logo](readme_assets/before_after_header_trans_img_s.png)

### http://www.folcolor.com

Coloring your Windows folders can help you organize and increase your productivity. Your folders are instantly recognizable by color vs having to pause to read their text label.

Especially useful for programmers/developers, artists, power users, etc., that might have dozens of folders and multilevel nested folder trees to manage.

### Latest release [download](https://github.com/kweatherman/Folcolor/releases/)

## Installing

1. Download the release bundle (`Folcolor-win11.zip`) and extract **all** files to one folder.
2. Run `Folcolor.exe` and click "Install".

The bundle contains the application plus the optional Windows 11 modern-menu payload:

| File | Purpose |
|---|---|
| `Folcolor.exe` | The application (x86, Windows 7–11) |
| `FolcolorMenu.x64.dll` | Context-menu handler for x64 Windows 11 |
| `FolcolorMenu.arm64.dll` | Optional ARM64 handler (only when built) |
| `FolcolorMenu.x86.dll` | Legacy x86 handler (not used by Windows 11) |
| `Folcolor-sparse.appx` | Identity package that enables the modern menu |
| `Folcolor.cer` | Public certificate used to trust the package |
| `menu.ps1` | Registration/unregistration helper script |

If only `Folcolor.exe` is present, Folcolor still installs and works through the classic context menu.

### The dialog window
![Folcolor Logo](readme_assets/ui_screenshot_install.png)

</br>
Now installed, right click on a folder, select "Color Folder", and select one of the 14 color selections.

![Folcolor Logo](readme_assets/set_game_folder_white_anim.gif)

## Windows 11 context menu

Windows 11's first-level context menu only accepts commands that come from a
package with an identity. Folcolor ships a small **sparse identity package**
plus an out-of-process `IExplorerCommand` DLL, which gives you:

* "Color Folder" directly in the Windows 11 first right-click menu.
* The classic cascade (14 colors + Restore Default + Launch Folcolor) as its submenu.
* The classic registry menu stays installed as a fallback and appears under
  **Show more options** (or Shift+F10).

The identity package is registered **per user**, while the signing certificate
is trusted **machine-wide**:

* `Folcolor.cer` is imported into `Cert:\LocalMachine\TrustedPeople`. AppX
  signature validation runs in the machine-scoped AppX deployment service and
  cannot see a per-user `TrustedPeople` store, so a user-scoped certificate is
  not enough.
* The package is registered with `Add-AppxPackage -ExternalLocation`.
* If Folcolor is elevated as a different account than the signed-in user, the
  installer delegates registration to a helper process running with the shell
  user's token.

Use the **Win11 menu: On/Off** button in the dialog to enable or disable the
modern menu at any time. Turning it off removes the package and certificate.

Registration writes diagnostics to `%TEMP%\Folcolor-menu.log`. If anything
fails, the classic context menu is still installed and functional.

## Features
* Selection of 14 colors based on the pleasant Google Material Design palette.
* 100% Free Open Source. You can know what it's made of and freely modify, remove, or add features to it.
* Minimalist, systems-level design. The application is a single small executable
  with an embedded icon resource; the Windows 11 menu handler is a separate
  out-of-process DLL loaded only by the COM surrogate (`dllhost.exe`), never
  inside Explorer itself.
* Secure design: Minimal API usage, zero network calls. No adware, no nag
  screens, no data collection, and no in-process Explorer shell extension.

## Requirements
Windows 7 to 11, 32-bit or 64-bit. The Windows 11 first-level context menu
requires Windows 11 22H2 (build 22000) or later; the classic menu works on all
supported versions.

## Motivation
After trying several commercial and free implementations of similar coloring tools, they were all lacking in one way or another.  Many had security red flags, advertising pop-ups, odd color schemes, using bloated Windows explorer shell Extensions, and/or with disagreeable license terms.  
I could see there really wasn't that much too them functionally so it was time to roll my own, make the better simpler one that I desired and then share it.

See http://www.folcolor.com for more details.

## Bugs, Issues, Feature requests
Please report any potential bugs or other issues, suggestions, feature requests, using the github issue feautre.
https://github.com/kweatherman/Folcolor/issues

## Building

Developed in C/C++ using [Microsoft Visual Studio 2022](https://visualstudio.microsoft.com):

```
msbuild src/Controller/Controller.sln /p:Configuration=Release /p:Platform=x86 /p:PlatformToolset=v143

msbuild src/Controller/MenuDLL/MenuDLL.vcxproj /p:Configuration=Release /p:Platform=x64 /p:PlatformToolset=v143
rem Optional ARM64 handler:
msbuild src/Controller/MenuDLL/MenuDLL.vcxproj /p:Configuration=Release /p:Platform=ARM64 /p:PlatformToolset=v143

powershell -ExecutionPolicy Bypass -File src/Controller/SparsePackage/build-sparse-pkg.ps1
```

The packaging script patches the manifest version and publisher, builds
`Win32\Release\Folcolor-sparse.appx`, signs it, exports `Folcolor.cer`, and
copies the per-architecture DLLs next to the executable.

Signing uses a self-signed certificate if none is supplied. For CI, provide a
stable signing identity through `FOLCOLOR_PFX_BASE64` and
`FOLCOLOR_PFX_PASSWORD` (a Base64-encoded `.pfx` and its password); the script
then derives the manifest publisher from that certificate, so a CA-issued
certificate works without editing the manifest.

Python 3.10 using the [PyCharm IDE](https://www.jetbrains.com/pycharm ) for procedurally generating the color icon sets.

See http://www.folcolor.com for more development details.

## Contributing
If you'd like to contribute, please fork the repository and use a feature
branch. Pull requests are welcomed.

## Licensing
The code and assets in this project is licensed under MIT license.  
Copyright (c) 2020 Kevin Weatherman  
https://github.com/kweatherman/Folcolor/blob/main/LICENSE
