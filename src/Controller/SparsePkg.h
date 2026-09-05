// Folcolor(tm) (c) 2020 Kevin Weatherman
// MIT license https://opensource.org/licenses/MIT
#pragma once

// Windows 11 only: register/unregister the sparse (identity) package that
// surfaces Folcolor's modern context menu via IExplorerCommand.
// InstallSparsePkg returns TRUE on success; on failure the legacy menu still
// works, so callers should warn but not abort installation.
BOOL InstallSparsePkg();
BOOL UninstallSparsePkg();