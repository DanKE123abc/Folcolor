// Folcolor(tm) (c) 2020 Kevin Weatherman
// MIT license https://opensource.org/licenses/MIT
//
// Windows 11 sparse-package registration for the modern context menu.
// The signed package (Folcolor-sparse.appx) and its files
// (FolcolorMenu.dll, Folcolor.cer) ship alongside Folcolor.exe. At install
// time we copy them into the install folder, trust the code-signing cert in
// the current user's Trusted People store, and register the identity
// package with Add-AppxPackage -ExternalLocation. Uninstall reverses each
// step so nothing is left behind.

#include "StdAfx.h"
#include "Language.h"
#include "SparsePkg.h"

#include <string>
#include <string.h>
#include <wchar.h>

extern WCHAR myPathGlobal[MAX_PATH];
extern BOOL g_isWin11;

#define PKG_NAME      L"Folcolor"
#define PKG_DLL       L"FolcolorMenu.dll"
#define PKG_APPX      L"Folcolor-sparse.appx"
#define PKG_CER       L"Folcolor.cer"
#define PKG_SUBJECT   L"CN=Folcolor"

static DWORD RunPowerShellVisibleTimeout(LPCWSTR script, DWORD timeoutMs)
{
    static const wchar_t B64T[] = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    size_t textLen = wcslen(script);
    const BYTE* bytes = (const BYTE*)script;
    size_t byteLen = textLen * 2;
    size_t b64Len = ((byteLen + 2) / 3) * 4;
    WCHAR* encoded = (WCHAR*)calloc(b64Len + 1, sizeof(WCHAR));
    if (!encoded)
        return (DWORD)-1;

    size_t si = 0, oi = 0;
    while (si + 3 <= byteLen)
    {
        DWORD v = (((DWORD)bytes[si]) << 16) | (((DWORD)bytes[si + 1]) << 8) | bytes[si + 2];
        encoded[oi++] = B64T[(v >> 18) & 63];
        encoded[oi++] = B64T[(v >> 12) & 63];
        encoded[oi++] = B64T[(v >> 6) & 63];
        encoded[oi++] = B64T[v & 63];
        si += 3;
    }
    DWORD rem = (DWORD)(byteLen - si);
    if (rem == 1)
    {
        DWORD v = ((DWORD)bytes[si]) << 16;
        encoded[oi++] = B64T[(v >> 18) & 63];
        encoded[oi++] = B64T[(v >> 12) & 63];
        encoded[oi++] = L'=';
        encoded[oi++] = L'=';
    }
    else if (rem == 2)
    {
        DWORD v = (((DWORD)bytes[si]) << 16) | (((DWORD)bytes[si + 1]) << 8);
        encoded[oi++] = B64T[(v >> 18) & 63];
        encoded[oi++] = B64T[(v >> 12) & 63];
        encoded[oi++] = B64T[(v >> 6) & 63];
        encoded[oi++] = L'=';
    }

    std::wstring cmd = L"powershell.exe -NoLogo -NoProfile -NonInteractive -ExecutionPolicy Bypass -EncodedCommand ";
    cmd += encoded;
    free(encoded);

    STARTUPINFOW siInfo = {};
    siInfo.cb = sizeof(siInfo);
    siInfo.dwFlags = STARTF_USESHOWWINDOW;
    siInfo.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {};

    if (!CreateProcessW(NULL, &cmd[0], NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &siInfo, &pi))
        return (DWORD)-1;

    WaitForSingleObject(pi.hProcess, timeoutMs);
    DWORD exitCode = (DWORD)-1;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return exitCode;
}

// Returns TRUE (no error) if none of our PKG_* files exist in the install folder.
static BOOL HasSparsePkgFiles()
{
    WCHAR file[MAX_PATH];
    _snwprintf_s(file, _countof(file), MAX_PATH - 1, L"%s%s", myPathGlobal, PKG_APPX);
    if (GetFileAttributesW(file) == INVALID_FILE_ATTRIBUTES)
        return FALSE;
    _snwprintf_s(file, _countof(file), MAX_PATH - 1, L"%s%s", myPathGlobal, PKG_DLL);
    if (GetFileAttributesW(file) == INVALID_FILE_ATTRIBUTES)
        return FALSE;
    _snwprintf_s(file, _countof(file), MAX_PATH - 1, L"%s%s", myPathGlobal, PKG_CER);
    if (GetFileAttributesW(file) == INVALID_FILE_ATTRIBUTES)
        return FALSE;
    return TRUE;
}

// Copy one of PKG_* from the directory the running exe lives in to the install folder.
static BOOL CopyPkgFile(LPCWSTR name)
{
    WCHAR srcDir[MAX_PATH];
    WCHAR src[MAX_PATH];
    WCHAR dst[MAX_PATH];
    if (!GetModuleFileNameW(NULL, srcDir, _countof(srcDir)))
        return FALSE;
    PathRemoveFileSpecW(srcDir);
    _snwprintf_s(src, _countof(src), MAX_PATH - 1, L"%s\\%s", srcDir, name);
    _snwprintf_s(dst, _countof(dst), MAX_PATH - 1, L"%s%s", myPathGlobal, name);
    if (GetFileAttributesW(src) == INVALID_FILE_ATTRIBUTES)
        return FALSE;
    return CopyFileW(src, dst, FALSE) != FALSE;
}

static LPCWSTR QuotePath(LPCWSTR path, WCHAR* out, size_t outLen)
{
    swprintf_s(out, outLen, L"'%s'", path);
    return out;
}

BOOL InstallSparsePkg()
{
    // The distribution must include the DLL + appx + cert next to Folcolor.exe.
    if (!CopyPkgFile(PKG_DLL))
        return FALSE;
    if (!CopyPkgFile(PKG_APPX))
        return FALSE;
    if (!CopyPkgFile(PKG_CER))
        return FALSE;
    if (!HasSparsePkgFiles())
        return FALSE;

    WCHAR qDll[MAX_PATH * 2], qCer[MAX_PATH * 2], qAppx[MAX_PATH * 2], qDir[MAX_PATH * 2];
    QuotePath(myPathGlobal, qDir, _countof(qDir));
    WCHAR appxPath[MAX_PATH];
    _snwprintf_s(appxPath, _countof(appxPath), MAX_PATH - 1, L"%s%s", myPathGlobal, PKG_APPX);
    QuotePath(appxPath, qAppx, _countof(qAppx));
    WCHAR cerPath[MAX_PATH];
    _snwprintf_s(cerPath, _countof(cerPath), MAX_PATH - 1, L"%s%s", myPathGlobal, PKG_CER);
    QuotePath(cerPath, qCer, _countof(qCer));
    (void)qDll;

    std::wstring script;
    // Unregister stale package first, then trust the dev cert, then register.
    script += L"$p=Get-AppxPackage -Name " PKG_NAME L" -ErrorAction SilentlyContinue; if($p){Remove-AppxPackage $p}";
    script += L" Get-ChildItem Cert:\\CurrentUser\\TrustedPeople | Where-Object {$_.Subject -match '" PKG_SUBJECT L"'} | Remove-Item -ErrorAction SilentlyContinue;";
    script += L" Import-Certificate -FilePath " + std::wstring(qCer) + L" -CertStoreLocation Cert:\\CurrentUser\\TrustedPeople | Out-Null;";
    script += L" Add-AppxPackage -Path " + std::wstring(qAppx) + L" -ExternalLocation " + std::wstring(qDir);

    DWORD rc = RunPowerShellVisibleTimeout(script.c_str(), 120000);
    return (rc == 0);
}

BOOL UninstallSparsePkg()
{
    std::wstring script;
    script += L"$p=Get-AppxPackage -Name " PKG_NAME L" -ErrorAction SilentlyContinue; if($p){Remove-AppxPackage $p}";
    script += L" Get-ChildItem Cert:\\CurrentUser\\TrustedPeople | Where-Object {$_.Subject -match '" PKG_SUBJECT L"'} | Remove-Item -ErrorAction SilentlyContinue";

    DWORD rc = RunPowerShellVisibleTimeout(script.c_str(), 120000);
    return (rc == 0);
}