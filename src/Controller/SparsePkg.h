// Folcolor(tm) (c) 2020 Kevin Weatherman
// MIT license https://opensource.org/licenses/MIT
#pragma once

#include <windows.h>

// Windows 11 only: register/unregister the sparse (identity) package that
// surfaces Folcolor's modern context menu via IExplorerCommand.
//
// The identity package is registered per user. The signing certificate is
// trusted machine-wide (LocalMachine\TrustedPeople) because AppX signature
// validation runs in the machine-scoped AppX deployment service and cannot
// see a user's TrustedPeople store. When Folcolor runs elevated as a
// different account than the signed-in user, registration is delegated to a
// helper process running with the shell user's token.

BOOL IsWin11MenuSupported();
BOOL IsWin11MenuEnabled();
void SetWin11MenuEnabled(BOOL enabled);

// Copy the menu payload next to the installed exe and register it.
BOOL InstallSparsePkg();

// Unregister the package/certificate. Safe to call when nothing is installed.
BOOL UninstallSparsePkg();

// Command-line helper modes (--install-menu / --uninstall-menu), executed in
// the signed-in user's context by InstallSparsePkg/UninstallSparsePkg.
BOOL RegisterMenuForCurrentUser();
BOOL UnregisterMenuForCurrentUser();

// Asks the user and restarts Explorer so the menu registration is picked up.
void PromptRestartExplorer(HWND owner);

// Path of the diagnostic log written by the registration script.
const wchar_t* GetMenuLogPath();
