// Folcolor(rm) (c) 2020 Kevin Weatherman
// MIT license https://opensource.org/ licenses/ MIT
#include "StdAfx.h"
#include "resource.h"
#include "FolderColorize.h"
#include "Language.h"
extern WCHAR myPathGlobal[ MAX_PATH];
extern int iconOffsetGlobal;
extern LANG_ID g_ currentLang;
extern BOOL g_ isWin11;
// Icon index to color label: now fetched using GetColorName for i18n
// Convert wide string to ANSI CP buffer (thread- local temporary)
static const char* WideToAnsiTemp( const wchar_ t* wstr)
{
	 static char buf[ 10];
	 if (! wstr)
	{
		 buf[ 0] = '\0';
		 return buf;
	}
	 int ret = WideCharToMultiByte( CP_ACP, 0, wstr, - 1, buf, ( int) sizeof(buf), NULL, NULL);
	 if (ret == 0)
		 buf[ 0] = '\0';
	 return buf;
}
static const char* GetMenuString( int stringId)
{
	 const wchar_ t* ws = GetLangString( g_ currentLang, stringId);
	 return WideToAnsiTemp( ws);
}
// Return TRUE if system has our installed registry entry (checks both legacy HKCR and Win11 HKCU paths)
BOOL HasInstallRegistry()
{
	 HKEY rootKey = NULL;
	 LSTATUS lStatus = RegOpenKeyExA( HKEY_CLASSES_ ROOT, REGISTRY_ PATH, 0, KEY_ READ, & rootKey);
	 if (lStatus == ERROR_SUCCESS)
	{
		 RegCloseKey( rootKey);
		 return TRUE;
	}
	 if (g_ isWin11)
	{
		 lStatus = RegOpenKeyExA( HKEY_CURRENT_ USER, REGISTRY_ PATH_ WIN11, 0, KEY_ READ, & rootKey);
		 if (lStatus == ERROR_SUCCESS)
	{
			 RegCloseKey( rootKey);
			 return TRUE;
		}
	}
	 return FALSE;
}
// Write shell menu entries for a given registry root/ parent path pair.
// _root: HKEY to use (HKEY_CLASSES_ ROOT or HKEY_CURRENT_ USER)
// _rootRegPath: relative path below _root, e. g. "Directory\shell\ Folcolor"
// _iconSrcPath: ANSI path to Folcolor. exe for icon and command values.
// On error, CRITICAL_ API_ FAIL will abort immediately.
static void InstallShellMenu( HKEY _root, LPCSTR _rootRegPath, const char* _iconSrcPath)
{
	 LSTATUS lStatus;
	 HKEY rootRegKey = NULL;
	 lStatus = RegOpenKeyExA( _root, _rootRegPath, 0, KEY_ WRITE, & rootRegKey);
	 if (lStatus == ERROR_FILE_ NOT_ FOUND)
	{
		 lStatus = RegCreateKeyExA( _root, _rootRegPath, 0, NULL, REG_OPTION_ NON_ VOLATILE, KEY_ WRITE, NULL, & rootRegKey, NULL);
	}
	 if (lStatus != ERROR_SUCCESS)
		 CRITICAL_ API_ FAIL( RegCreateKeyExA, lStatus);
	 int len;
	 char buffer[ 10];
	 DWORD dwValue;
# define WRITE_STRING(_k, _n, _s, _sz) lStatus = RegSetValueEx( _k, _n, 0, REG_SZ, ( PBYTE) (_ s), ( DWORD) (_ sz)); if (lStatus != ERROR_SUCCESS) CRITICAL_ API_ FAIL( RegSetValueEx, lStatus);
# define WRITE_LITERAL(_k, _n, _lit) WRITE_STRING(_k, _n, _lit, sizeof(_ lit))
# define WRITE_DWORD(_k, _n, _val) dwValue = DWORD(_ val); lStatus = RegSetValueEx( _k, _n, 0, REG_DWORD, ( PBYTE) & dwValue, sizeof( DWORD)); if (lStatus != ERROR_SUCCESS) CRITICAL_ API_ FAIL( RegSetValueEx, lStatus);
# define WRITE_ICON_ENTRY(_k, _idx) len = sprintf_ s(buffer, sizeof( buffer), "%s" TARGET_ NAME ",%u", _iconSrcPath, ( UINT) (_ idx)); WRITE_STRING(_k, "Icon", buffer, ( len + 1))
# define WRITE_CMD_ENTRY(_k, _idx) len = sprintf_ s(buffer, sizeof( buffer), "%s" TARGET_ NAME " " COMMAND_ICON "%d " COMMAND_FOLDER "%%1", _iconSrcPath, ( int) (_ idx)); WRITE_STRING(_k, "", buffer, ( len + 1))
# define CREATE_ SUBKEY(_p, _s, _out) lStatus = RegCreateKeyExA( _p, _s, 0, NULL, REG_OPTION_ NON_ VOLATILE, KEY_ WRITE, NULL, &_ out, NULL); if (lStatus != ERROR_SUCCESS) CRITICAL_ API_ FAIL( RegCreateKeyExA, lStatus);
	 const wchar_ t* colorFolderW = GetLangString( g_ currentLang, STR_COLOR_FOLDER);
	 RegSetValueExW( rootRegKey, L" MUIVerb", 0, REG_SZ, ( PBYTE) colorFolderW, ( DWORD) (( wcslen( colorFolderW) + 1) * sizeof( wchar_ t)));
	 WRITE_LITERAL( rootRegKey, " SubCommands", " ");
	 len = sprintf_ s(buffer, sizeof( buffer), "%s" TARGET_ NAME, _iconSrcPath);
	 WRITE_STRING( rootRegKey, " Icon", buffer, ( len + 1));
	 HKEY shellKey = NULL;
	 CREATE_ SUBKEY( rootRegKey, " shell", shellKey);
		 HKEY colorKey = NULL, cmdKey = NULL;
		 for ( UINT i = 0; i < COLOR_ICON_COUNT; i++)
		{
			 char num[ 8];
			 sprintf( num, sizeof( num), "%02u", i);
			 CREATE_ SUBKEY( shellKey, num, colorKey);
			 WRITE_ICON_ENTRY( colorKey, ( i + ( UINT) iconOffsetGlobal));
			 const wchar_ t* colorNameW = GetColorName( g_ currentLang, i);
			 RegSetValueExW( colorKey, L" MUIVerb", 0, REG_SZ, ( PBYTE) colorNameW, ( DWORD) (( wcslen( colorNameW) + 1) * sizeof( wchar_ t)));
			 CREATE_ SUBKEY( colorKey, " command", cmdKey);
			 WRITE_CMD_ENTRY( cmdKey, i);
			 RegCloseKey( cmdKey); cmdKey = NULL;
			 RegCloseKey( colorKey); colorKey = NULL;
		}
		 CREATE_ SUBKEY( shellKey, " 14", colorKey);
		 WRITE_LITERAL( colorKey, " Icon", "%SystemRoot%\\ system32\\ shell32. dll,4");
		 const wchar_ t* restoreW = GetLangString( g_ currentLang, STR_RESTORE_ DEFAULT);
		 RegSetValueExW( colorKey, L" MUIVerb", 0, REG_SZ, ( PBYTE) restoreW, ( DWORD) (( wcslen( restoreW) + 1) * sizeof( wchar_ t)));
		 WRITE_DWORD( colorKey, " CommandFlags", 0x20);
		 CREATE_ SUBKEY( colorKey, " command", cmdKey);
		 WRITE_CMD_ENTRY( cmdKey, COLOR_ICON_COUNT);
		 RegCloseKey( cmdKey); cmdKey = NULL;
		 RegCloseKey( colorKey); colorKey = NULL;
		 CREATE_ SUBKEY( shellKey, " 15", colorKey);
		 len = sprintf_ s(buffer, sizeof( buffer), "%s" TARGET_ NAME, _iconSrcPath);
		 WRITE_STRING( colorKey, " Icon", buffer, ( len + 1));
		 const wchar_ t* launchW = GetLangString( g_ currentLang, STR_LAUNCH_ FOLCOLOR);
		 RegSetValueExW( colorKey, L" MUIVerb", 0, REG_SZ, ( PBYTE) launchW, ( DWORD) (( wcslen( launchW) + 1) * sizeof( wchar_ t)));
		 WRITE_LITERAL( colorKey, " HasLUAShield", " ");
		 WRITE_DWORD( colorKey, " CommandFlags", 0x20);
		 CREATE_ SUBKEY( colorKey, " command", cmdKey);
		 len = sprintf_ s(buffer, sizeof( buffer), "%s" TARGET_ NAME, _iconSrcPath);
		 WRITE_STRING( cmdKey, "", buffer, ( len + 1));
		 RegCloseKey( cmdKey); cmdKey = NULL;
		 RegCloseKey( colorKey); colorKey = NULL;
	 RegCloseKey( shellKey); shellKey = NULL;
	 RegCloseKey( rootRegKey); rootRegKey = NULL;
# undef WRITE_STRING
# undef WRITE_LITERAL
# undef WRITE_DWORD
# undef WRITE_ICON_ENTRY
# undef WRITE_CMD_ENTRY
# undef CREATE_ SUBKEY
}
// Write our shell registry. Handles both Win11 new context menu (HKCU, no admin) and legacy (HKCR, admin required).
static void InstallRegistry()
{
	 // Delete our existing keys if they are already there
	 DeleteRegistryPath( HKEY_CLASSES_ ROOT, REGISTRY_ PATH);
	 if (g_ isWin11)
		 DeleteRegistryPath( HKEY_CURRENT_ USER, REGISTRY_ PATH_ WIN11);
	 // ASCII copy installation path as icon source path
	 char srcPath[ MAX_PATH];
	 size_ t dontCare;
	 errno_ t en = wcstombs_ s(& dontCare, srcPath, sizeof( srcPath), myPathGlobal, sizeof( srcPath) - 1);
	 if (en != 0)
		 CRITICAL_ API_ ERRNO( wcstombs_ s, en);
	 // Legacy menu: HKEY_CLASSES_ ROOT\Directory\shell\ Folcolor (all Windows, requires admin UAC)
	 InstallShellMenu( HKEY_CLASSES_ ROOT, REGISTRY_ PATH, srcPath);
	 // Win11 new menu: HKEY_CURRENT_ USER\Software\Classes\Directory\Background\shell\ Folcolor (no admin UAC needed)
	 if (g_ isWin11)
		 InstallShellMenu( HKEY_CURRENT_ USER, REGISTRY_ PATH_ WIN11, srcPath);
}
// Install ourself
void Install()
{
	 // Create installation folder
	 if (! CreateDirectoryW( myPathGlobal, NULL))
		 CRITICAL_ API_ FAIL( CreateDirectoryW, GetLastError());
	 // Copy ourself there
	 WCHAR myPath[ MAX_PATH];
	 if (! GetModuleFileNameW( NULL, myPath, _ countof( myPath)))
		 CRITICAL_ API_ FAIL( GetModuleFileNameW, GetLastError());
	 WCHAR myName[ _MAX_FNAME];
	 if (! GetModuleBaseNameW( GetCurrentProcess(), NULL, myName, _ countof( myName)))
		 CRITICAL_ API_ FAIL( GetModuleBaseNameW, GetLastError());
	 WCHAR targetPath[ MAX_PATH];
	 if (_ snwprintf_ s( targetPath, _ countof( targetPath), _ countof( targetPath) - 1, L"%s%s", myPathGlobal, myName) < 1)
		 CRITICAL( " Path size limit error!");
	 if (! CopyFileW( myPath, targetPath, FALSE))
		 CRITICAL_ API_ FAIL( CopyFileW, GetLastError());
	 // And " README. md" file if it exists
	 if (PathRemoveFileSpecW( myPath))
	{
		 if (wcscat_ s( myPath, _ countof( myPath), L"\\ README. md") != 0)
			 CRITICAL( " Path size limit error!");
		 if (_ snwprintf_ s( targetPath, _ countof( targetPath), _ countof( targetPath) - 1, L"%s README. md", myPathGlobal) < 1)
			 CRITICAL( " Path size limit error!");
		 CopyFileW( myPath, targetPath, FALSE);
	}
	 InstallRegistry();
	 // Without this the app icon might not show up in explorer right away
	 ResetWindowsIconCache();
}
// Uninstall ourself
// Returns 0 = Needs manual uninstall step, 1 = complete
int Uninstall()
{
	 // Remove our registry keys (both legacy HKCR and Win11 HKCU)
	 DeleteRegistryPath( HKEY_CLASSES_ ROOT, REGISTRY_ PATH);
	 if (g_ isWin11)
		 DeleteRegistryPath( HKEY_CURRENT_ USER, REGISTRY_ PATH_ WIN11);
	 ResetWindowsIconCache();
	 // Double check the path to avoid a disaster
	 if (wcsstr( myPathGlobal, INSTALL_ FOLDER))
	{
		 WCHAR myPathCopy[ MAX_PATH + 1];
		 ZeroMemory( myPathCopy, sizeof( myPathCopy));
		 if (wcsncpy_ s( myPathCopy, MAX_PATH, myPathGlobal, wcslen( myPathGlobal) - 1) == 0)
		{
			 SHFILEOPSTRUCTW nfo =
			 {
				 NULL,
				 FO_DELETE,
				 myPathCopy,
				 NULL,
				 ( FOF_NO_ UI | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT | FOF_ALLOWUNDO),
				 FALSE,
				 NULL,
				 NULL
			 };
			 SHFileOperationW( & nfo);
		}
	}
	 // Return 0 if installation folder is still there
	 DWORD attr = GetFileAttributesW( myPathGlobal);
	 return ! (( attr != INVALID_ FILE_ ATTRIBUTES) && ( attr & FILE_ ATTRIBUTE_ DIRECTORY));
}