// Folcolor(tm) (c) 2020 Kevin Weatherman
// MIT license https://opensource.org/licenses/MIT
//
// Windows 11 sparse-package registration for the modern context menu.
//
// The identity package is registered per user with Add-AppxPackage
// -ExternalLocation. The signing certificate is trusted machine-wide
// (LocalMachine\TrustedPeople): AppX signature validation runs in the
// machine-scoped AppX deployment service and cannot see a user's store.
// Folcolor.exe requires administrator, so when it is elevated as a different
// account than the shell user, registration is delegated to a helper process
// running with the shell user's token.
//
// Registration is best-effort: if anything fails the classic context menu is
// still installed and fully functional.

#include "StdAfx.h"
#include "Language.h"
#include "SparsePkg.h"

#include <string.h>
#include <wchar.h>
#include <string>

#ifndef PROCESSOR_ARCHITECTURE_ARM64
#define PROCESSOR_ARCHITECTURE_ARM64 12
#endif

extern WCHAR myPathGlobal[MAX_PATH];
extern BOOL g_isWin11;

#define PKG_NAME      L"Folcolor"
#define PKG_DLL       L"FolcolorMenu.dll"
#define PKG_APPX      L"Folcolor-sparse.appx"
#define PKG_CER       L"Folcolor.cer"
#define PKG_SUBJECT   L"CN=Folcolor"

#define MENU_LOG_FILE L"Folcolor-menu.log"

// ---------------------------------------------------------------------------
// Small path helpers
// ---------------------------------------------------------------------------
static std::wstring GetSourceDir()
{
    WCHAR path[MAX_PATH] = {};
    ::GetModuleFileNameW(NULL, path, _countof(path));
    ::PathRemoveFileSpecW(path);
    return path;
}

static std::wstring TrimTrailingSlash(const std::wstring& path)
{
    std::wstring result = path;
    while (!result.empty() && (result.back() == L'\\' || result.back() == L'/'))
        result.pop_back();
    return result;
}

static std::wstring LogPath()
{
    WCHAR temp[MAX_PATH] = {};
    if (::GetTempPathW(_countof(temp), temp) == 0)
        return L"Folcolor-menu.log";

    std::wstring path = temp;
    path += MENU_LOG_FILE;
    return path;
}

static BOOL FileExists(LPCWSTR path)
{
    DWORD attributes = ::GetFileAttributesW(path);
    return (attributes != INVALID_FILE_ATTRIBUTES) && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
}

// The surrogate host that runs the context-menu DLL matches the machine
// architecture, so pick the DLL that was built for it.
static LPCWSTR MenuDllSourceName()
{
    SYSTEM_INFO info = {};
    ::GetNativeSystemInfo(&info);
    switch (info.wProcessorArchitecture)
    {
        case PROCESSOR_ARCHITECTURE_AMD64:
            return L"FolcolorMenu.x64.dll";
        case PROCESSOR_ARCHITECTURE_ARM64:
            return L"FolcolorMenu.arm64.dll";
        default:
            return L"FolcolorMenu.x86.dll";
    }
}

// ---------------------------------------------------------------------------
// Payload copy
// ---------------------------------------------------------------------------
static BOOL CopySourceToInstall(LPCWSTR sourceName, LPCWSTR targetName)
{
    std::wstring source = GetSourceDir();
    source += L"\\";
    source += sourceName;

    std::wstring target = myPathGlobal;
    target += targetName;

    if (_wcsicmp(source.c_str(), target.c_str()) == 0)
        return FileExists(target.c_str());

    if (!FileExists(source.c_str()))
        return FALSE;

    // dllhost may still hold the previous DLL for a few seconds after a click.
    for (int attempt = 0; attempt < 3; ++attempt)
    {
        if (::CopyFileW(source.c_str(), target.c_str(), FALSE))
            return TRUE;
        ::Sleep(200);
    }

    // An older copy that is still installed is better than no menu at all.
    return FileExists(target.c_str());
}

static BOOL RunningFromInstallFolder()
{
    std::wstring install = TrimTrailingSlash(myPathGlobal);
    return _wcsicmp(GetSourceDir().c_str(), install.c_str()) == 0;
}

static BOOL CopyMenuPayload()
{
    // Arch-specific DLL first. The historical plain name is only accepted when
    // we are already running from the install folder (it is the DLL that was
    // installed earlier), not when a loose x86 DLL sits next to the download.
    BOOL dllCopied = CopySourceToInstall(MenuDllSourceName(), PKG_DLL);
    if (!dllCopied && RunningFromInstallFolder())
        dllCopied = CopySourceToInstall(L"FolcolorMenu.dll", PKG_DLL);
    if (!dllCopied)
        return FALSE;

    if (!CopySourceToInstall(PKG_APPX, PKG_APPX))
        return FALSE;
    if (!CopySourceToInstall(PKG_CER, PKG_CER))
        return FALSE;
    if (!CopySourceToInstall(L"menu.ps1", L"menu.ps1"))
        return FALSE;

    return TRUE;
}

// ---------------------------------------------------------------------------
// PowerShell bridge
// ---------------------------------------------------------------------------
static BOOL RunPowerShellFile(const std::wstring& scriptPath, const std::wstring& arguments,
                              DWORD timeoutMs, DWORD* exitCode)
{
    std::wstring commandLine = L"powershell.exe -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -File \"";
    commandLine += scriptPath;
    commandLine += L"\" ";
    commandLine += arguments;

    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi = {};
    if (!::CreateProcessW(NULL, &commandLine[0], NULL, NULL, FALSE, CREATE_NO_WINDOW,
                          NULL, NULL, &si, &pi))
        return FALSE;

    DWORD waitResult = ::WaitForSingleObject(pi.hProcess, timeoutMs);
    if (exitCode)
    {
        DWORD code = 1;
        ::GetExitCodeProcess(pi.hProcess, &code);
        *exitCode = code;
    }

    ::CloseHandle(pi.hProcess);
    ::CloseHandle(pi.hThread);

    return waitResult == WAIT_OBJECT_0;
}

static BOOL RunMenuScript(LPCWSTR mode, BOOL machineWide)
{
    // The script ships next to the exe (admin-only Program Files) so it
    // cannot be swapped by a medium-integrity process between write and
    // execution.
    std::wstring script = myPathGlobal;
    script += L"menu.ps1";
    if (!FileExists(script.c_str()))
        return FALSE;

    std::wstring appx = myPathGlobal;
    appx += PKG_APPX;
    std::wstring cer = myPathGlobal;
    cer += PKG_CER;
    std::wstring external = TrimTrailingSlash(myPathGlobal);

    std::wstring arguments = L"-Mode ";
    arguments += mode;
    arguments += L" -Appx \"" + appx + L"\"";
    arguments += L" -Cer \"" + cer + L"\"";
    arguments += L" -External \"" + external + L"\"";
    arguments += L" -Name " PKG_NAME;
    arguments += L" -Subject \"" PKG_SUBJECT L"\"";
    if (machineWide)
        arguments += L" -Machine";

    DWORD exitCode = 1;
    return RunPowerShellFile(script, arguments, 120000, &exitCode) && (exitCode == 0);
}

// ---------------------------------------------------------------------------
// Per-user registration (executed in the signed-in user's context)
// ---------------------------------------------------------------------------
BOOL RegisterMenuForCurrentUser()
{
    return RunMenuScript(L"install", FALSE);
}

BOOL UnregisterMenuForCurrentUser()
{
    return RunMenuScript(L"uninstall", FALSE);
}

// ---------------------------------------------------------------------------
// Shell-user delegation
// ---------------------------------------------------------------------------
static HANDLE OpenShellUserToken(DWORD access)
{
    HWND shell = ::GetShellWindow();
    if (!shell)
        return NULL;

    DWORD pid = 0;
    ::GetWindowThreadProcessId(shell, &pid);
    if (!pid)
        return NULL;

    HANDLE process = ::OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!process)
        return NULL;

    HANDLE token = NULL;
    if (!::OpenProcessToken(process, access, &token))
        token = NULL;

    ::CloseHandle(process);
    return token;
}

static BOOL IsShellUserSameAsCurrent()
{
    HANDLE shellToken = OpenShellUserToken(TOKEN_QUERY);
    if (!shellToken)
        return TRUE;

    HANDLE currentToken = NULL;
    if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &currentToken))
    {
        ::CloseHandle(shellToken);
        return TRUE;
    }

    struct TokenUserBuffer
    {
        TOKEN_USER user;
        BYTE sid[SECURITY_MAX_SID_SIZE];
    };

    TokenUserBuffer shellUser = {};
    TokenUserBuffer currentUser = {};
    DWORD shellSize = 0;
    DWORD currentSize = 0;
    BOOL same = TRUE;

    if (::GetTokenInformation(shellToken, TokenUser, &shellUser, sizeof(shellUser), &shellSize) &&
        ::GetTokenInformation(currentToken, TokenUser, &currentUser, sizeof(currentUser), &currentSize))
    {
        same = ::EqualSid(shellUser.user.User.Sid, currentUser.user.User.Sid) != FALSE;
    }

    ::CloseHandle(currentToken);
    ::CloseHandle(shellToken);
    return same;
}

static BOOL CreateProcessWithShellToken(LPCWSTR application, std::wstring commandLine,
                                        DWORD flags, DWORD timeoutMs, DWORD* exitCode)
{
    HANDLE shellToken = OpenShellUserToken(TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY | TOKEN_QUERY);
    if (!shellToken)
        return FALSE;

    HANDLE primaryToken = NULL;
    BOOL ok = ::DuplicateTokenEx(shellToken, MAXIMUM_ALLOWED, NULL,
                                 SecurityImpersonation, TokenPrimary, &primaryToken) != FALSE;
    ::CloseHandle(shellToken);
    if (!ok)
        return FALSE;

    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi = {};
    ok = ::CreateProcessWithTokenW(primaryToken, LOGON_WITH_PROFILE, application,
                                   &commandLine[0],
                                   flags | CREATE_NO_WINDOW, NULL, NULL, &si, &pi) != FALSE;
    ::CloseHandle(primaryToken);

    if (!ok)
        return FALSE;

    DWORD waitResult = ::WaitForSingleObject(pi.hProcess, timeoutMs);
    if (exitCode)
    {
        DWORD code = 1;
        ::GetExitCodeProcess(pi.hProcess, &code);
        *exitCode = code;
    }

    ::CloseHandle(pi.hProcess);
    ::CloseHandle(pi.hThread);

    return waitResult == WAIT_OBJECT_0;
}

// Runs "Folcolor.exe <argument>" as the shell user. Prefer the installed
// copy so the helper always sees the payload in Program Files.
static BOOL RunHelperAsShellUser(LPCWSTR argument, DWORD timeoutMs, DWORD* exitCode)
{
    std::wstring exe = myPathGlobal;
    exe += L"Folcolor.exe";

    if (!FileExists(exe.c_str()))
    {
        WCHAR module[MAX_PATH] = {};
        if (!::GetModuleFileNameW(NULL, module, _countof(module)))
            return FALSE;
        exe = module;
    }

    std::wstring commandLine = L"\"";
    commandLine += exe;
    commandLine += L"\" ";
    commandLine += argument;

    return CreateProcessWithShellToken(exe.c_str(), commandLine, 0, timeoutMs, exitCode);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------
BOOL IsWin11MenuSupported()
{
    return g_isWin11 ? TRUE : FALSE;
}

BOOL IsWin11MenuEnabled()
{
    DWORD value = 1;
    DWORD size = sizeof(value);
    LSTATUS status = ::RegGetValueW(HKEY_CURRENT_USER, L"Software\\Folcolor",
                                    L"EnableWin11Menu", RRF_RT_REG_DWORD, NULL,
                                    &value, &size);
    if (status != ERROR_SUCCESS)
        return TRUE;

    return value != 0;
}

void SetWin11MenuEnabled(BOOL enabled)
{
    DWORD value = enabled ? 1 : 0;
    ::RegSetKeyValueW(HKEY_CURRENT_USER, L"Software\\Folcolor", L"EnableWin11Menu",
                      REG_DWORD, &value, (DWORD)sizeof(value));
}

BOOL InstallSparsePkg()
{
    if (!IsWin11MenuSupported())
        return FALSE;

    if (!CopyMenuPayload())
        return FALSE;

    // Clear registrations left behind by older builds (per-machine and
    // per-user). This is best-effort and needs the elevation we already have.
    RunMenuScript(L"cleanup", TRUE);

    // Add-AppxPackage validates the signature in the machine-scoped AppX
    // deployment service, which only sees LocalMachine\TrustedPeople (a
    // CurrentUser store is invisible to it). The install script also ensures
    // this immediately before Add-AppxPackage when it runs elevated, so a
    // failure here is not fatal.
    RunMenuScript(L"trust", TRUE);

    if (!IsShellUserSameAsCurrent())
    {
        DWORD exitCode = 1;
        if (!RunHelperAsShellUser(L"--install-menu", 120000, &exitCode))
            return FALSE;
        return exitCode == 0;
    }

    return RegisterMenuForCurrentUser();
}

BOOL UninstallSparsePkg()
{
    if (!IsWin11MenuSupported())
        return TRUE;

    BOOL ok = TRUE;
    if (!IsShellUserSameAsCurrent())
    {
        DWORD exitCode = 1;
        if (RunHelperAsShellUser(L"--uninstall-menu", 120000, &exitCode))
            ok = (exitCode == 0);
        else
            ok = UnregisterMenuForCurrentUser();
    }
    else
    {
        ok = UnregisterMenuForCurrentUser();
    }

    // Remove machine-wide leftovers (provisioned package / LocalMachine cert)
    // from older builds as well.
    RunMenuScript(L"cleanup", TRUE);
    return ok;
}

// ---------------------------------------------------------------------------
// Explorer restart
// ---------------------------------------------------------------------------
static BOOL IsProcessElevated()
{
    HANDLE token = NULL;
    if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &token))
        return FALSE;

    TOKEN_ELEVATION elevation = {};
    DWORD size = 0;
    BOOL elevated = FALSE;
    if (::GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size))
        elevated = elevation.TokenIsElevated != 0;

    ::CloseHandle(token);
    return elevated;
}

// Medium-integrity caller (the normal UI): Explorer belongs to the same user,
// so kill it and start a normal (non-elevated) copy.
static BOOL RestartExplorerDirect()
{
    HWND shell = ::GetShellWindow();
    if (!shell)
        return FALSE;

    DWORD pid = 0;
    ::GetWindowThreadProcessId(shell, &pid);
    if (!pid)
        return FALSE;

    HANDLE process = ::OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE | PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!process)
        return FALSE;

    ::TerminateProcess(process, 0);
    ::WaitForSingleObject(process, 5000);
    ::CloseHandle(process);
    ::Sleep(800);

    WCHAR explorer[MAX_PATH] = {};
    if (::GetWindowsDirectoryW(explorer, _countof(explorer)) == 0)
        return FALSE;
    if (wcscat_s(explorer, _countof(explorer), L"\\explorer.exe") != 0)
        return FALSE;

    std::wstring commandLine = L"\"";
    commandLine += explorer;
    commandLine += L"\"";

    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};
    BOOL ok = ::CreateProcessW(explorer, &commandLine[0], NULL, NULL, FALSE, 0,
                               NULL, NULL, &si, &pi) != FALSE;
    if (ok)
    {
        ::CloseHandle(pi.hProcess);
        ::CloseHandle(pi.hThread);
    }

    return ok;
}

static BOOL RestartExplorerAsShellUser()
{
    HWND shell = ::GetShellWindow();
    if (!shell)
        return FALSE;

    DWORD pid = 0;
    ::GetWindowThreadProcessId(shell, &pid);
    if (!pid)
        return FALSE;

    HANDLE process = ::OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE | PROCESS_QUERY_INFORMATION,
                                   FALSE, pid);
    if (!process)
        return FALSE;

    HANDLE processToken = NULL;
    if (!::OpenProcessToken(process, TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY | TOKEN_QUERY,
                            &processToken))
    {
        ::CloseHandle(process);
        return FALSE;
    }

    HANDLE primaryToken = NULL;
    BOOL ok = ::DuplicateTokenEx(processToken, MAXIMUM_ALLOWED, NULL,
                                 SecurityImpersonation, TokenPrimary, &primaryToken) != FALSE;
    ::CloseHandle(processToken);

    if (!ok)
    {
        ::CloseHandle(process);
        return FALSE;
    }

    ::TerminateProcess(process, 0);
    ::WaitForSingleObject(process, 5000);
    ::CloseHandle(process);
    ::Sleep(800);

    WCHAR explorer[MAX_PATH] = {};
    if (::GetWindowsDirectoryW(explorer, _countof(explorer)) == 0)
    {
        ::CloseHandle(primaryToken);
        return FALSE;
    }
    if (wcscat_s(explorer, _countof(explorer), L"\\explorer.exe") != 0)
    {
        ::CloseHandle(primaryToken);
        return FALSE;
    }

    std::wstring commandLine = L"\"";
    commandLine += explorer;
    commandLine += L"\"";

    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};
    ok = ::CreateProcessWithTokenW(primaryToken, 0, explorer, &commandLine[0],
                                   0, NULL, NULL, &si, &pi) != FALSE;
    ::CloseHandle(primaryToken);

    if (ok)
    {
        ::CloseHandle(pi.hProcess);
        ::CloseHandle(pi.hThread);
    }

    return ok;
}

void PromptRestartExplorer(HWND owner)
{
    const wchar_t* prompt = GetLangString(g_currentLang, STR_RESTART_EXPLORER);
    if (::MessageBoxW(owner, prompt, PROJECT_NAME_W, MB_YESNO | MB_ICONQUESTION) != IDYES)
        return;

    if (!(IsProcessElevated() ? RestartExplorerAsShellUser() : RestartExplorerDirect()))
    {
        const wchar_t* failed = GetLangString(g_currentLang, STR_RESTART_FAILED);
        ::MessageBoxW(owner, failed, PROJECT_NAME_W, MB_OK | MB_ICONWARNING);
    }
}

// Log path is exposed for diagnostics in the failure message.
const wchar_t* GetMenuLogPath()
{
    static std::wstring path;
    path = LogPath();
    return path.c_str();
}
