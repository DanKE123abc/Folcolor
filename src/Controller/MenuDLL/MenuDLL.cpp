// Folcolor(tm) (c) 2020 Kevin Weatherman
// MIT license https://opensource.org/licenses/MIT
//
// Win11 modern context menu handler (Sparse Package + IExplorerCommand).
// Mirrors the pattern used by Microsoft's PhotoStoreContextMenu sparse
// package sample and the ContextMenuShortcuts generator, extended with an
// EnumSubCommands cascade matching Folcolor's classic cascading menu:
//   "Color Folder" parent -> 14 colors + "Restore Default" + "Launch Folcolor"
//
// The DLL is registered via a signed sparse package (AppxManifest) with an
// out-of-proc COM surrogate; Explorer activates it for the modern context menu.

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shlwapi.h>
#include <shobjidl_core.h>
#include <wrl/client.h>
#include <wrl/implements.h>
#include <wrl/module.h>
#include "MenuStrings.h"
#include "FolderColorize.h"

#include <string>
#include <vector>
#include <cwchar>
#include <cstring>

using Microsoft::WRL::ComPtr;
using Microsoft::WRL::Make;
using Microsoft::WRL::RuntimeClass;
using Microsoft::WRL::RuntimeClassFlags;
using Microsoft::WRL::ClassicCom;

#define CLSID_STRING "6D3B1C8E-9A14-4C7E-B1F0-5A2C3D4E5F60"

static HMODULE g_module = NULL;

// The parent command owns the CLSID above. The 16 child commands derive a
// stable canonical name from it (last byte 0x61..0x70) so Explorer can tell
// them apart; the parent keeps 0x60.
static const GUID kFolcolorCommandGuid =
{ 0x6d3b1c8e, 0x9a14, 0x4c7e, { 0xb1, 0xf0, 0x5a, 0x2c, 0x3d, 0x4e, 0x5f, 0x60 } };

static GUID MakeSubCommandGuid(int subIndex)
{
    GUID guid = kFolcolorCommandGuid;
    guid.Data4[7] = (BYTE)(0x61 + (subIndex & 0x0F));
    return guid;
}

// Only show the command when Explorer actually passed a selection. This is
// also what keeps the DLL out of the "background" context (we do not register
// a Directory\Background item for the modern menu).
static EXPCMDSTATE StateForSelection(_In_opt_ IShellItemArray* selection)
{
    if (!selection)
        return ECS_HIDDEN;

    DWORD count = 0;
    if (FAILED(selection->GetCount(&count)) || count == 0)
        return ECS_HIDDEN;

    return ECS_ENABLED;
}

// CommandLineToArgvW treats a backslash before the closing quote as an
// escape, so double a trailing backslash (drive roots such as C:\) before
// quoting.
static std::wstring QuoteArgument(const std::wstring& value)
{
    std::wstring quoted = L"\"";
    quoted += value;
    if (!value.empty() && value.back() == L'\\')
        quoted += L'\\';
    quoted += L"\"";
    return quoted;
}

static BOOL LaunchCommandLine(const std::wstring& commandLine)
{
    std::wstring mutableCommandLine = commandLine;
    if (mutableCommandLine.empty())
        return FALSE;

    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi = {};
    BOOL created = ::CreateProcessW(nullptr, &mutableCommandLine[0], nullptr, nullptr,
                                   FALSE, 0, nullptr, nullptr, &si, &pi);
    if (pi.hProcess)
        ::CloseHandle(pi.hProcess);
    if (pi.hThread)
        ::CloseHandle(pi.hThread);
    return created;
}

namespace
{
    // Directory that holds this DLL (== install dir where Folcolor.exe lives).
    std::wstring GetModuleDir()
    {
        WCHAR path[MAX_PATH] = { 0 };
        ::GetModuleFileNameW(g_module, path, _countof(path));
        ::PathRemoveFileSpecW(path);
        return std::wstring(path);
    }

    std::wstring GetExePath()
    {
        return GetModuleDir() + L"\\" L"Folcolor.exe";
    }

    std::wstring GetExeIcon(int iconIndex)
    {
        WCHAR buf[64];
        swprintf_s(buf, _countof(buf), L",%d", iconIndex);
        return GetExePath() + buf;
    }
}

// ---------------------------------------------------------------------------
// A single color / restore / launch verb shown in the modern context menu.
// m_subIndex: 0..13 colors, 14 = restore default, 15 = launch Folcolor.
// ---------------------------------------------------------------------------
class CFolcolorSubCommand
    : public RuntimeClass<RuntimeClassFlags<ClassicCom>, IExplorerCommand>
{
public:
    explicit CFolcolorSubCommand(int subIndex) : m_subIndex(subIndex) {}

    IFACEMETHODIMP GetTitle(_In_opt_ IShellItemArray*, _Outptr_result_nullonfailure_ PWSTR* name)
    {
        *name = nullptr;
        int lang = mfDetectLanguage();
        const wchar_t* title = nullptr;
        if (m_subIndex < COLOR_ICON_COUNT)
            title = mfColorName(lang, m_subIndex);
        else if (m_subIndex == COLOR_ICON_COUNT)
            title = mfMenuLabel(lang, MMENU_RESTORE_DEFAULT);
        else
            title = mfMenuLabel(lang, MMENU_LAUNCH);

        WCHAR* out = (WCHAR*)::CoTaskMemAlloc((wcslen(title) + 1) * sizeof(WCHAR));
        if (!out)
            return E_OUTOFMEMORY;
        wcscpy_s(out, wcslen(title) + 1, title);
        *name = out;
        return S_OK;
    }

    IFACEMETHODIMP GetIcon(_In_opt_ IShellItemArray*, _Outptr_result_nullonfailure_ PWSTR* icon)
    {
        *icon = nullptr;
        std::wstring iconPath;
        if (m_subIndex < COLOR_ICON_COUNT)
            iconPath = GetExeIcon(WIN11_ICON_OFFSET + m_subIndex);
        else if (m_subIndex == COLOR_ICON_COUNT)
            iconPath = L"%SystemRoot%\\system32\\shell32.dll,4";
        else
            iconPath = GetExeIcon(0);

        WCHAR* out = (WCHAR*)::CoTaskMemAlloc((iconPath.length() + 1) * sizeof(WCHAR));
        if (!out)
            return E_OUTOFMEMORY;
        wcscpy_s(out, iconPath.length() + 1, iconPath.c_str());
        *icon = out;
        return S_OK;
    }

    IFACEMETHODIMP GetToolTip(_In_opt_ IShellItemArray*, _Outptr_result_nullonfailure_ PWSTR* infoTip)
    {
        *infoTip = nullptr;
        return E_NOTIMPL;
    }

    IFACEMETHODIMP GetCanonicalName(_Out_ GUID* guidCommandName)
    {
        *guidCommandName = MakeSubCommandGuid(m_subIndex);
        return S_OK;
    }

    IFACEMETHODIMP GetState(_In_opt_ IShellItemArray* selection, _In_ BOOL okToBeSlow, _Out_ EXPCMDSTATE* cmdState)
    {
        (void)selection;
        (void)okToBeSlow;
        // The parent command gates the whole flyout; keeping the children
        // enabled avoids an empty submenu if Explorer queries them with no
        // selection array.
        *cmdState = ECS_ENABLED;
        return S_OK;
    }

    IFACEMETHODIMP Invoke(_In_opt_ IShellItemArray* selection, _In_opt_ IBindCtx*) noexcept
    {
        const std::wstring exe = GetExePath();

        // "Launch Folcolor" does not need a selection.
        if (m_subIndex > COLOR_ICON_COUNT)
        {
            LaunchCommandLine(L"\"" + exe + L"\"");
            return S_OK;
        }

        if (!selection)
            return S_OK;

        DWORD count = 0;
        if (FAILED(selection->GetCount(&count)) || count == 0)
            return S_OK;

        // Color / Restore: collect every selected folder and hand them to a
        // single Folcolor.exe process. This avoids the old one-process-per-item
        // race where each copy saw the others as a duplicate instance.
        std::vector<std::wstring> folders;
        folders.reserve(count);
        for (DWORD i = 0; i < count; ++i)
        {
            ComPtr<IShellItem> psi;
            if (FAILED(selection->GetItemAt(i, &psi)))
                continue;
            PWSTR itemName = nullptr;
            if (FAILED(psi->GetDisplayName(SIGDN_FILESYSPATH, &itemName)))
                continue;
            folders.push_back(itemName);
            ::CoTaskMemFree(itemName);
        }

        if (folders.empty())
            return S_OK;

        std::wstring commandLine = L"\"" + exe + L"\" --index=" + std::to_wstring(m_subIndex);
        for (size_t i = 0; i < folders.size(); ++i)
            commandLine += L" --folder=" + QuoteArgument(folders[i]);

        // CreateProcess rejects command lines longer than 32767 characters.
        // Fall back to one process per folder for very large selections.
        if (commandLine.size() > 30000)
        {
            const std::wstring prefix = L"\"" + exe + L"\" --index=" + std::to_wstring(m_subIndex);
            for (size_t i = 0; i < folders.size(); ++i)
                LaunchCommandLine(prefix + L" --folder=" + QuoteArgument(folders[i]));
        }
        else
        {
            LaunchCommandLine(commandLine);
        }

        return S_OK;
    }

    IFACEMETHODIMP GetFlags(_Out_ EXPCMDFLAGS* flags)
    {
        *flags = ECF_DEFAULT;
        return S_OK;
    }

    IFACEMETHODIMP EnumSubCommands(_COM_Outptr_ IEnumExplorerCommand** enumCommands)
    {
        *enumCommands = nullptr;
        return E_NOTIMPL;
    }

private:
    int m_subIndex;
};

// ---------------------------------------------------------------------------
// Simple IEnumExplorerCommand over a vector of child IExplorerCommand objects.
// ---------------------------------------------------------------------------
class CFolcolorCommandList
    : public RuntimeClass<RuntimeClassFlags<ClassicCom>, IEnumExplorerCommand>
{
public:
    IFACEMETHODIMP Next(ULONG celt, IExplorerCommand** rgelt, ULONG* pceltFetched)
    {
        if (pceltFetched)
            *pceltFetched = 0;
        ULONG fetched = 0;
        while (fetched < celt && m_pos < m_items.size())
        {
            rgelt[fetched] = m_items[m_pos].Get();
            if (rgelt[fetched])
                rgelt[fetched]->AddRef();
            ++m_pos;
            ++fetched;
        }
        if (pceltFetched)
            *pceltFetched = fetched;
        return (fetched == celt) ? S_OK : S_FALSE;
    }

    IFACEMETHODIMP Skip(ULONG celt)
    {
        if (m_pos + celt > m_items.size())
        {
            m_pos = m_items.size();
            return S_FALSE;
        }
        m_pos += celt;
        return S_OK;
    }

    IFACEMETHODIMP Reset()
    {
        m_pos = 0;
        return S_OK;
    }

    IFACEMETHODIMP Clone(IEnumExplorerCommand** ppenum)
    {
        *ppenum = nullptr;
        ComPtr<CFolcolorCommandList> clone = Make<CFolcolorCommandList>();
        if (!clone)
            return E_OUTOFMEMORY;
        clone->m_items = m_items;
        *ppenum = clone.Detach();
        return S_OK;
    }

    void Add(const ComPtr<IExplorerCommand>& cmd)
    {
        m_items.push_back(cmd);
    }

private:
    std::vector<ComPtr<IExplorerCommand>> m_items;
    size_t m_pos = 0;
};

// ---------------------------------------------------------------------------
// Parent "Color Folder" command: cascading menu via EnumSubCommands.
// ---------------------------------------------------------------------------
class CFolcolorCommand
    : public RuntimeClass<RuntimeClassFlags<ClassicCom>, IExplorerCommand, IObjectWithSite>
{
public:
    IFACEMETHODIMP GetTitle(_In_opt_ IShellItemArray*, _Outptr_result_nullonfailure_ PWSTR* name)
    {
        *name = nullptr;
        const wchar_t* title = mfMenuLabel(mfDetectLanguage(), MMENU_COLOR_FOLDER);
        WCHAR* out = (WCHAR*)::CoTaskMemAlloc((wcslen(title) + 1) * sizeof(WCHAR));
        if (!out)
            return E_OUTOFMEMORY;
        wcscpy_s(out, wcslen(title) + 1, title);
        *name = out;
        return S_OK;
    }

    IFACEMETHODIMP GetIcon(_In_opt_ IShellItemArray*, _Outptr_result_nullonfailure_ PWSTR* icon)
    {
        *icon = nullptr;
        std::wstring iconPath = GetExeIcon(0);
        WCHAR* out = (WCHAR*)::CoTaskMemAlloc((iconPath.length() + 1) * sizeof(WCHAR));
        if (!out)
            return E_OUTOFMEMORY;
        wcscpy_s(out, iconPath.length() + 1, iconPath.c_str());
        *icon = out;
        return S_OK;
    }

    IFACEMETHODIMP GetToolTip(_In_opt_ IShellItemArray*, _Outptr_result_nullonfailure_ PWSTR* infoTip)
    {
        *infoTip = nullptr;
        return E_NOTIMPL;
    }

    IFACEMETHODIMP GetCanonicalName(_Out_ GUID* guidCommandName)
    {
        *guidCommandName = kFolcolorCommandGuid;
        return S_OK;
    }

    IFACEMETHODIMP GetState(_In_opt_ IShellItemArray* selection, _In_ BOOL okToBeSlow, _Out_ EXPCMDSTATE* cmdState)
    {
        (void)okToBeSlow;
        *cmdState = StateForSelection(selection);
        return S_OK;
    }

    IFACEMETHODIMP Invoke(_In_opt_ IShellItemArray*, _In_opt_ IBindCtx*) noexcept
    {
        // Parent is not directly invocable; children drive the action.
        return S_OK;
    }

    IFACEMETHODIMP GetFlags(_Out_ EXPCMDFLAGS* flags)
    {
        *flags = ECF_HASSUBCOMMANDS;
        return S_OK;
    }

    IFACEMETHODIMP EnumSubCommands(_COM_Outptr_ IEnumExplorerCommand** enumCommands)
    {
        *enumCommands = nullptr;
        ComPtr<CFolcolorCommandList> list = Make<CFolcolorCommandList>();
        if (!list)
            return E_OUTOFMEMORY;

        int last = COLOR_ICON_COUNT + 2; // colors + restore + launch
        for (int i = 0; i < last; ++i)
        {
            ComPtr<IExplorerCommand> sub = Make<CFolcolorSubCommand>(i);
            if (sub)
                list->Add(sub);
        }
        *enumCommands = list.Detach();
        return S_OK;
    }

    // IObjectWithSite
    IFACEMETHODIMP SetSite(_In_ IUnknown* site) noexcept
    {
        m_site = site;
        return S_OK;
    }
    IFACEMETHODIMP GetSite(_In_ REFIID riid, _COM_Outptr_ void** site) noexcept
    {
        return m_site ? m_site.CopyTo(riid, site) : E_FAIL;
    }

private:
    ComPtr<IUnknown> m_site;
};

class __declspec(uuid(CLSID_STRING)) CFolcolorCommandRegistered final : public CFolcolorCommand
{
};

CoCreatableClass(CFolcolorCommandRegistered);
CoCreatableClassWrlCreatorMapInclude(CFolcolorCommandRegistered);

STDAPI DllGetActivationFactory(_In_ HSTRING activatableClassId, _COM_Outptr_ IActivationFactory** factory)
{
    return Microsoft::WRL::Module<Microsoft::WRL::ModuleType::InProc>::GetModule().GetActivationFactory(activatableClassId, factory);
}

STDAPI DllCanUnloadNow()
{
    return Microsoft::WRL::Module<Microsoft::WRL::ModuleType::InProc>::GetModule().GetObjectCount() == 0 ? S_OK : S_FALSE;
}

STDAPI DllGetClassObject(_In_ REFCLSID rclsid, _In_ REFIID riid, _COM_Outptr_ void** instance)
{
    return Microsoft::WRL::Module<Microsoft::WRL::ModuleType::InProc>::GetModule().GetClassObject(rclsid, riid, instance);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
        g_module = hModule;
    return TRUE;
}
