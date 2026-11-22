
// Folcolor(tm) (c) 2020 Kevin Weatherman
// MIT license https://opensource.org/licenses/MIT
#include "StdAfx.h"
#include <versionhelpers.h>
#include "resource.h"
#include "FolderColorize.h"
#include "Language.h"
#include "SparsePkg.h"

#include <string>
#include <vector>

#define APP_URL "http://www.folcolor.com/"

static BOOL isInstalled = FALSE;
WCHAR myPathGlobal[MAX_PATH] = {};
int iconOffsetGlobal = WIN7_ICON_OFFSET;
extern void Install();
extern int Uninstall();
extern BOOL HasInstallRegistry();
extern BOOL g_isWin11;

// ntdll.lib
typedef long NTSTATUS;
extern "C" NTSYSAPI NTSTATUS NTAPI RtlGetVersion(__out PRTL_OSVERSIONINFOEXW VersionInformation /*PRTL_OSVERSIONINFOW*/);


// If there is another instance of us running, pull it into focus and return TRUE
static BOOL FindDoppelganger()
{
	BOOL found = FALSE;

	UINT myPid = GetCurrentProcessId();
	char myName[_MAX_FNAME];
	if (!GetModuleBaseNameA(GetCurrentProcess(), NULL, myName, _countof(myName)))
		strcpy_s(myName, TARGET_NAME);

	PROCESSENTRY32 nfo;
	nfo.dwSize = sizeof(nfo);
	HANDLE handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (handle != INVALID_HANDLE_VALUE)
	{
		if (Process32First(handle, &nfo))
		{
			do
			{
				if ((strcmp(myName, nfo.szExeFile) == 0) && (nfo.th32ProcessID != myPid))
				{
					HWND hwnd = GetHwndForPid(nfo.th32ProcessID);
					if(hwnd)
						ForceWindowFocus(hwnd);
					found = TRUE;
					break;
				}

			} while (Process32Next(handle, &nfo));
		}

		CloseHandle(handle);
	}

	return found;
}


// ----------------------------------------------------------------------------
// Custom hyperlink control
#define VISITED_COLOR RGB(160, 160, 260)
#define URL_BACK_COLOR RGB(76, 76, 76)
#define URL_FONT_HEIGHT 18
#define URL_FONT_WIDTH FW_SEMIBOLD
static HFONT tweakedFont = NULL, undlineFont = NULL;
static BOOL isClicking = FALSE, isVisited = FALSE;
static COLORREF urlColor = RGB(245, 245, 245);
static HBRUSH buttonBrush = NULL;
static HCURSOR mouseOverCursor = NULL;

static void OpenLinkUrl()
{
	SHELLEXECUTEINFOA nfo;
	ZeroMemory(&nfo, sizeof(SHELLEXECUTEINFOA));
	nfo.cbSize = sizeof(SHELLEXECUTEINFOA);
	nfo.lpVerb = "open";
	nfo.lpFile = APP_URL;
	nfo.fMask  = SEE_MASK_ASYNCOK;
	nfo.nShow  = SW_SHOWNORMAL;
	ShellExecuteExA(&nfo);
}

static LRESULT CALLBACK HypLinkSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData)
{
	switch(uMsg)
	{
		case WM_NCDESTROY:
		RemoveWindowSubclass(hWnd, HypLinkSubclass, uIdSubclass);
		break;

		case WM_NCHITTEST:
		return HTCLIENT;
		break;

		// Switch font when use hovers over us
		case WM_MOUSEMOVE:
		{
			if(GetCapture() != hWnd)
			{
				SendMessage(hWnd, WM_SETFONT, (WPARAM) undlineFont, FALSE);
				//UpdateWinDatForegroundColor(hWnd, pcHyperlink->m_HighlightColor);
				InvalidateRect(hWnd, NULL, FALSE);
				SetCapture(hWnd);
			}
			else
			{
				RECT rect;
				GetWindowRect(hWnd, &rect);
				POINT pt = { LOWORD(lParam), HIWORD(lParam)};
				ClientToScreen(hWnd, &pt);

				if(!PtInRect(&rect, pt))
				{
					SendMessage(hWnd, WM_SETFONT, (WPARAM) tweakedFont, FALSE);
					//UpdateWinDatForegroundColor(hWnd, pcHyperlink->m_CurrentColor);
					InvalidateRect(hWnd, NULL, FALSE);
					ReleaseCapture();
				}
			}
		}
		break;

		// Finger point cursor when over the control
		case WM_SETCURSOR:
		if(mouseOverCursor)
		{
			SetCursor(mouseOverCursor);
			return 1;
		}
		break;

		case WM_LBUTTONDOWN:
		{
			SetFocus(hWnd);
			SetCapture(hWnd);
			isClicking = TRUE;
		}
		break;

		case WM_LBUTTONUP:
		{
			ReleaseCapture();

			if(isClicking)
			{
				isClicking = FALSE;
				POINT pt;
				pt.x = (short) LOWORD(lParam);
				pt.y = (short) HIWORD(lParam);
				ClientToScreen(hWnd, &pt);
				RECT rc;
				GetWindowRect(hWnd, &rc);

				if(PtInRect(&rc,pt))
				{
					if(!isVisited)
					{
						isVisited = TRUE;
						//UpdateWinDatForegroundColor(hWnd, pcHyperlink->m_CurrentColor = pcHyperlink->m_VisitedColor);
						urlColor = VISITED_COLOR;
						InvalidateRect(hWnd, NULL, TRUE);
					}

					OpenLinkUrl();
				}
			}
		}
		break;

		case WM_SETFOCUS:
		{
			InvalidateRect(hWnd, NULL, TRUE);
			UpdateWindow(hWnd);
		}
		break;

		case WM_KILLFOCUS:
		{
			SendMessage(hWnd, WM_SETFONT, (WPARAM) tweakedFont, FALSE);
			//UpdateWinDatForegroundColor(hWnd, pcHyperlink->m_CurrentColor);
			InvalidateRect(hWnd, NULL, TRUE);
			UpdateWindow(hWnd);
		}
		break;

		case WM_GETDLGCODE:
		return DLGC_WANTCHARS;
		break;

		// Space key activate?
		case WM_KEYDOWN:
		if(wParam == VK_SPACE)
		{
			if(!isVisited)
			{
				isVisited = TRUE;
				//UpdateWinDatForegroundColor(hWnd, pcHyperlink->m_CurrentColor = pcHyperlink->m_VisitedColor);
				urlColor = VISITED_COLOR;
				InvalidateRect(hWnd, NULL, TRUE);
			}

			OpenLinkUrl();
			return 0;
		}
		break;

		case WM_KEYUP:
		if(wParam == VK_SPACE)
			return 0;
		break;
	};

	return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

// ----------------------------------------------------------------------------

// Apply language-dependent dialog texts (buttons) for the current language
static void ApplyLanguageTexts(HWND hWnd)
{
	if (isInstalled)
	{
		const wchar_t* uninstallStr = GetLangString(g_currentLang, STR_UNINSTALL);
		SetDlgItemTextW(hWnd, IDC_INSTALL_UNINSTALL, uninstallStr);
	}
	else
	{
		const wchar_t* installStr = GetLangString(g_currentLang, STR_INSTALL);
		SetDlgItemTextW(hWnd, IDC_INSTALL_UNINSTALL, installStr);
	}
	{
		const wchar_t* refreshStr = GetLangString(g_currentLang, STR_REFRESH_WINDOW);
		SetDlgItemTextW(hWnd, IDC_REFRESH, refreshStr);
	}
	{
		HWND hMenu = GetDlgItem(hWnd, IDC_WIN11MENU);
		if (hMenu)
		{
			const wchar_t* menuStr = GetLangString(g_currentLang,
				IsWin11MenuEnabled() ? STR_WIN11_MENU_ON : STR_WIN11_MENU_OFF);
			SetWindowTextW(hMenu, menuStr);
		}
	}
}

// Fill the language combo box and select the current language
static void InitLanguageCombo(HWND hWnd)
{
	HWND hCombo = GetDlgItem(hWnd, IDC_LANGUAGE);
	if (!hCombo)
		return;

	for (int i = 0; i < LANG_COUNT; i++)
	{
		const wchar_t* name = GetLangDisplayName((LANG_ID) i);
		SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM) name);
	}
	SendMessageW(hCombo, CB_SETCURSEL, (WPARAM) g_currentLang, 0);
}

// Our dialog Window message handler
static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			// Add info to caption
			{
				wchar_t title[256];
				swprintf_s(title, _countof(title), L"%s %s Built: %S", PROJECT_NAME_W, APP_VERSION_W, __DATE__);
				SetWindowTextW(hWnd, title);
			}

			// Set dialog icon
			HICON hIcon = LoadIconA((HINSTANCE) GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_APP));
			SendMessage(hWnd, WM_SETICON, ICON_BIG, (LPARAM) hIcon);
			SendMessage(hWnd, WM_SETICON, ICON_SMALL, (LPARAM) hIcon);

			// Setup customized hyperlink control
			HWND hWndCtrl = GetDlgItem(hWnd, IDC_HYPERLINK);
			if (hWndCtrl)
			{
				// Font options
				HFONT fontHandle = (HFONT) SendMessage(hWndCtrl, WM_GETFONT, 0, 0);
				if (!fontHandle)
					fontHandle = (HFONT) GetStockObject(DEFAULT_GUI_FONT);

				if (fontHandle)
				{
					LOGFONT lf;
					if (GetObject(fontHandle, sizeof(LOGFONT), &lf) != 0)
					{
						// Emphasized font
						lf.lfHeight = URL_FONT_HEIGHT;
						lf.lfWeight = URL_FONT_WIDTH;
						tweakedFont = CreateFontIndirect(&lf);
						if (tweakedFont)
							SendMessageA(hWndCtrl, WM_SETFONT, WPARAM(tweakedFont), TRUE);

						// Underlined version
						lf.lfUnderline = TRUE;
						undlineFont = CreateFontIndirect(&lf);
					}
				}

				// Hyperlink finger cursor
				static const BYTE curAND[128] =
				{
					0xF9,0xFF,0xFF,0xFF, 0xF0,0xFF,0xFF,0xFF, 0xF0,0xFF,0xFF,0xFF, 0xF0,0xFF,0xFF,0xFF,
					0xF0,0xFF,0xFF,0xFF, 0xF0,0x3F,0xFF,0xFF, 0xF0,0x07,0xFF,0xFF, 0xF0,0x01,0xFF,0xFF,
					0xF0,0x00,0xFF,0xFF, 0x10,0x00,0x7F,0xFF, 0x00,0x00,0x7F,0xFF, 0x00,0x00,0x7F,0xFF,
					0x80,0x00,0x7F,0xFF, 0xC0,0x00,0x7F,0xFF, 0xC0,0x00,0x7F,0xFF, 0xE0,0x00,0x7F,0xFF,
					0xE0,0x00,0xFF,0xFF, 0xF0,0x00,0xFF,0xFF, 0xF0,0x00,0xFF,0xFF, 0xF8,0x01,0xFF,0xFF,
					0xF8,0x01,0xFF,0xFF, 0xF8,0x01,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF,
					0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF,
					0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF, 0xFF,0xFF,0xFF,0xFF
				};
				static const BYTE curXOR[128] =
				{
					0x00,0x00,0x00,0x00, 0x06,0x00,0x00,0x00, 0x06,0x00,0x00,0x00, 0x06,0x00,0x00,0x00,
					0x06,0x00,0x00,0x00, 0x06,0x00,0x00,0x00, 0x06,0xC0,0x00,0x00, 0x06,0xD8,0x00,0x00,
					0x06,0xDA,0x00,0x00, 0x06,0xDB,0x00,0x00, 0x67,0xFB,0x00,0x00, 0x77,0xFF,0x00,0x00,
					0x37,0xFF,0x00,0x00, 0x17,0xFF,0x00,0x00, 0x1F,0xFF,0x00,0x00, 0x0F,0xFF,0x00,0x00,
					0x0F,0xFE,0x00,0x00, 0x07,0xFE,0x00,0x00, 0x07,0xFE,0x00,0x00, 0x03,0xFC,0x00,0x00,
					0x03,0xFC,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
					0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00,
					0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00, 0x00,0x00,0x00,0x00
				};
				mouseOverCursor = CreateCursor(GetModuleHandle(NULL), 5, 0, 32, 32, curAND, curXOR);

				// Subclasses the control
				SetWindowSubclass(hWndCtrl, HypLinkSubclass, 1138, 0);
			}

			// Tweak the button font too
			HFONT buttonFont = NULL;
			HFONT fontHandle = (HFONT) SendMessage(GetDlgItem(hWnd, IDC_INSTALL_UNINSTALL), WM_GETFONT, 0, 0);
			if (!fontHandle)
				fontHandle = (HFONT) GetStockObject(DEFAULT_GUI_FONT);
			if (fontHandle)
			{
				LOGFONT lf;
				if (GetObject(fontHandle, sizeof(LOGFONT), &lf) != 0)
				{
					// Emphasized font
					lf.lfHeight = 14;
					lf.lfWeight = FW_MEDIUM;
					buttonFont = CreateFontIndirect(&lf);
					if (buttonFont)
					{
						SendMessageA(GetDlgItem(hWnd, IDC_INSTALL_UNINSTALL), WM_SETFONT, WPARAM(buttonFont), TRUE);
						SendMessageA(GetDlgItem(hWnd, IDC_REFRESH), WM_SETFONT, WPARAM(buttonFont), TRUE);
					}
				}
			}

			// Windows 11 modern-menu toggle. Created dynamically so the dialog
			// resource does not need a new control (and stays easy to hide on
			// Windows 10).
			if (g_isWin11 && isInstalled)
			{
				HWND hMenu = CreateWindowExW(0, L"BUTTON", L"",
					WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | BS_FLAT,
					9, 66, 182, 14, hWnd, (HMENU)IDC_WIN11MENU,
					GetModuleHandleW(NULL), NULL);
				if (hMenu && buttonFont)
					SendMessageW(hMenu, WM_SETFONT, (WPARAM)buttonFont, TRUE);
			}

			// Apply language-dependent button texts and initialize language selector
			ApplyLanguageTexts(hWnd);
			InitLanguageCombo(hWnd);

			return (INT_PTR) TRUE;
		}
		break;

		case WM_COMMAND:
		{
			switch (LOWORD(wParam))
			{
				// Install/Uninstall
case IDC_INSTALL_UNINSTALL:
			{
				const wchar_t* installStr = GetLangString(g_currentLang, STR_INSTALL);
				const wchar_t* uninstallStr = GetLangString(g_currentLang, STR_UNINSTALL);
				const wchar_t* completionStr = GetLangString(g_currentLang, STR_COMPLETION);
				const wchar_t* installCompleteStr = GetLangString(g_currentLang, STR_INSTALL_COMPLETE);
				const wchar_t* confirmUninstallStr = GetLangString(g_currentLang, STR_CONFIRM_UNINSTALL);
				const wchar_t* registryUninstalledStr = GetLangString(g_currentLang, STR_REGISTRY_UNINSTALLED);
				const wchar_t* manualDeleteStr = GetLangString(g_currentLang, STR_MANUAL_DELETE);
				const wchar_t* uninstallCompleteStr = GetLangString(g_currentLang, STR_UNINSTALL_COMPLETE);

				if (!isInstalled)
				{
					Install();
					isInstalled = TRUE;
					SetDlgItemTextW(hWnd, IDC_INSTALL_UNINSTALL, uninstallStr);
					MessageBoxW(hWnd, installCompleteStr, completionStr, (MB_OK | MB_ICONASTERISK));
					EndDialog(hWnd, 0);
				}
				else
				{
					wchar_t confirmMsg[512];
					swprintf_s(confirmMsg, _countof(confirmMsg), L"%s %s?", PROJECT_NAME_W, confirmUninstallStr);
					if (MessageBoxW(hWnd, confirmMsg, completionStr, (MB_OKCANCEL | MB_ICONQUESTION)) == IDOK)
					{
						int ur = Uninstall();
						isInstalled = FALSE;
						SetDlgItemTextW(hWnd, IDC_INSTALL_UNINSTALL, installStr);

						if (ur == 0)
						{
							wchar_t msg[512];
							swprintf_s(msg, _countof(msg), L"%s\n\"%s\"\n%s.", registryUninstalledStr, myPathGlobal, manualDeleteStr);
							MessageBoxW(hWnd, msg, completionStr, (MB_OK | MB_ICONASTERISK));
						}
						else
							MessageBoxW(hWnd, uninstallCompleteStr, completionStr, (MB_OK | MB_ICONASTERISK));

						EndDialog(hWnd, 0);
					}
				}

				return (INT_PTR) TRUE;
			}
				break;

				// Toggle the Windows 11 modern context menu
				case IDC_WIN11MENU:
				{
					BOOL enable = !IsWin11MenuEnabled();
					BOOL ok = enable ? InstallSparsePkg() : UninstallSparsePkg();
					if (!ok)
					{
						wchar_t message[768];
						swprintf_s(message, _countof(message), L"%s\n\n%s",
							GetLangString(g_currentLang, STR_WIN11_MENU_FAIL), GetMenuLogPath());
						MessageBoxW(hWnd, message, PROJECT_NAME_W, (MB_OK | MB_ICONWARNING));
					}
					else
					{
						SetWin11MenuEnabled(enable);
						HWND hMenu = GetDlgItem(hWnd, IDC_WIN11MENU);
						if (hMenu)
						{
							SetWindowTextW(hMenu, GetLangString(g_currentLang,
								enable ? STR_WIN11_MENU_ON : STR_WIN11_MENU_OFF));
						}
						PromptRestartExplorer(hWnd);
					}
					return (INT_PTR) TRUE;
				}
				break;

				// Refresh Windows icon cache DB
				case IDC_REFRESH:
				{
					ResetWindowsIconCache();
					return (INT_PTR) TRUE;
				}
				break;

				// Language selector changed
				case IDC_LANGUAGE:
				{
					if (HIWORD(wParam) == CBN_SELCHANGE)
					{
						LRESULT sel = SendMessageW((HWND)lParam, CB_GETCURSEL, 0, 0);
						if (sel != CB_ERR && sel >= 0 && sel < LANG_COUNT)
						{
							g_currentLang = (LANG_ID) sel;
							ApplyLanguageTexts(hWnd);
						}
					}
					return (INT_PTR) TRUE;
				}
				break;
			};
		}
		break;

		case WM_CTLCOLORSTATIC:
		{
			DWORD id = GetDlgCtrlID((HWND)lParam);
			if (id == IDC_HYPERLINK)
			{
				HDC hdcStatic = (HDC) wParam;
				SetTextColor(hdcStatic, urlColor);
				SetBkColor(hdcStatic, URL_BACK_COLOR);
				return (INT_PTR) GetStockObject(NULL_PEN);
			}
		}
		break;

		case WM_CTLCOLORBTN:
		{
			if (!buttonBrush)
				buttonBrush = CreateSolidBrush(RGB(128, 100, 100));
			return (LRESULT) buttonBrush;
		}
		break;

		case WM_CLOSE:		
		EndDialog(hWnd, 0);		
		break;
	}

	return (INT_PTR) FALSE;
}


struct MenuCommandLine
{
	BOOL installMenu;
	BOOL uninstallMenu;
	int colorIndex;
	std::vector<std::wstring> folders;

	MenuCommandLine() : installMenu(FALSE), uninstallMenu(FALSE), colorIndex(-1) {}
};

// Parses the arguments produced by the classic registry verbs and by the
// Windows 11 MenuDLL. Both "--folder=<path>" and "--folder <path>" are
// accepted, and a legacy unquoted path containing spaces is reassembled from
// the tokens Windows split it into.
static void ParseMenuCommandLine(MenuCommandLine& parsed)
{
	int argc = 0;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (!argv)
		return;

	for (int i = 1; i < argc; i++)
	{
		const wchar_t* arg = argv[i];

		if (wcscmp(arg, L"--install-menu") == 0)
			parsed.installMenu = TRUE;
		else if (wcscmp(arg, L"--uninstall-menu") == 0)
			parsed.uninstallMenu = TRUE;
		else if (wcsncmp(arg, L"--index=", 8) == 0)
			parsed.colorIndex = _wtoi(arg + 8);
		else if (wcscmp(arg, L"--index") == 0 && (i + 1) < argc)
			parsed.colorIndex = _wtoi(argv[++i]);
		else if (wcsncmp(arg, L"--folder=", 9) == 0)
		{
			std::wstring folder(arg + 9);
			while (((i + 1) < argc) && (wcsncmp(argv[i + 1], L"--", 2) != 0))
			{
				folder += L' ';
				folder += argv[++i];
			}
			parsed.folders.push_back(folder);
		}
		else if (wcscmp(arg, L"--folder") == 0 && (i + 1) < argc)
		{
			parsed.folders.push_back(argv[++i]);
		}
	}

	LocalFree(argv);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	(void) hPrevInstance;
	(void) pCmdLine;
	(void) nCmdShow;

	// Detect system language first
	g_currentLang = DetectSystemLanguage();

	// Our path from "%ProgramFiles(x86)%" base
	C_ASSERT(_countof(myPathGlobal) >= MAX_PATH);
	HRESULT hr = SHGetSpecialFolderPathW(0, myPathGlobal, CSIDL_PROGRAM_FILES, FALSE);
	if (FAILED(hr))
		CRITICAL_API_FAIL(SHGetSpecialFolderPathW, HRESULT_CODE(hr));
	if (wcscat_s(myPathGlobal, _countof(myPathGlobal), L"\\" INSTALL_FOLDER L"\\") != 0)
		CRITICAL("Path size limit error!");

	// Icon resource set index per OS version
	OSVERSIONINFOEXW versionInfo = {};
	versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
	RtlGetVersion((PRTL_OSVERSIONINFOEXW) &versionInfo);
	if (versionInfo.dwMajorVersion >= 10)
	{
		if (versionInfo.dwBuildNumber >= 22000)
		{
			iconOffsetGlobal = WIN11_ICON_OFFSET;
			g_isWin11 = TRUE;
		}
		else
			iconOffsetGlobal = WIN10_ICON_OFFSET;
	}

	MenuCommandLine commandLine;
	ParseMenuCommandLine(commandLine);

	// Hidden helper modes used when the elevated installer has to register the
	// per-user package in the signed-in user's context.
	if (commandLine.installMenu || commandLine.uninstallMenu)
	{
		BOOL ok = commandLine.installMenu ? RegisterMenuForCurrentUser()
		                                  : UnregisterMenuForCurrentUser();
		return ok ? EXIT_SUCCESS : EXIT_FAILURE;
	}

	// We're installed? Have registry, or have install folder?
	isInstalled = HasInstallRegistry();
	DWORD attr = GetFileAttributesW(myPathGlobal);
	isInstalled |= ((attr != INVALID_FILE_ATTRIBUTES) && (attr & FILE_ATTRIBUTE_DIRECTORY));

	// Context-menu action: one process handles every selected folder. The
	// single-instance check below is intentionally skipped here so a menu
	// click still works while the main dialog is open.
	if ((commandLine.colorIndex >= 0) && !commandLine.folders.empty())
	{
		if (isInstalled)
		{
			for (size_t i = 0; i < commandLine.folders.size(); i++)
			{
				SetFolderColor(commandLine.colorIndex,
					const_cast<LPWSTR>(commandLine.folders[i].c_str()));
			}
		}
		return EXIT_SUCCESS;
	}

	// Should only be one instance running
	if (FindDoppelganger())
		return EXIT_FAILURE;

	return (int) DialogBoxParamW(hInstance, (LPCWSTR) IDD_MAIN, 0, &DlgProc, 0);
}
