#include <Windows.h>
#include "resource.h"
#include "../ClassicWindowSwitcher/sws_def.h"
#include "../ClassicWindowSwitcher/sws_WindowSwitcher.h"

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define WM_TRAY (WM_USER + 1)

bool g_isStandaloneConfigurator = false;
HINSTANCE g_hInst = nullptr;
void* g_unloadFn = nullptr;
void* g_getInstanceFn = nullptr;
sws_WindowSwitcher* g_pSwitcher = nullptr;

void SaveSettings(HWND hDlg);
void ShowConfDlg(HWND hDlg);
void HideConfDlg(HWND hDlg);

long long CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG: {
		if (!g_isStandaloneConfigurator) {
			NOTIFYICONDATA nid = {};
			nid.cbSize = sizeof(NOTIFYICONDATA);
			nid.hWnd = hDlg;
			nid.uID = 1;
			nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
			nid.uCallbackMessage = WM_TRAY;
			nid.hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_CWS));
			wcscpy_s(nid.szTip, L"ClassicWindowSwitcher");
			Shell_NotifyIconW(NIM_ADD, &nid);
		}

		SendDlgItemMessageW(hDlg, IDC_WHEELCOMBO, CB_ADDSTRING, 0, (LPARAM)L"Disabled");
		SendDlgItemMessageW(hDlg, IDC_WHEELCOMBO, CB_ADDSTRING, 0, (LPARAM)L"Seek Windows");
		SendDlgItemMessageW(hDlg, IDC_WHEELCOMBO, CB_ADDSTRING, 0, (LPARAM)L"Seek Windows (Full screen)");
		SendDlgItemMessageW(hDlg, IDC_WHEELCOMBO, CB_ADDSTRING, 0, (LPARAM)L"Grid Scroll");
		SendDlgItemMessageW(hDlg, IDC_WHEELCOMBO, CB_ADDSTRING, 0, (LPARAM)L"Grid scroll on hover else seek items");
		SendDlgItemMessageW(hDlg, IDC_WHEELCOMBO, CB_ADDSTRING, 0, (LPARAM)L"Grid Scroll (Full screen)");
		SendDlgItemMessageW(hDlg, IDC_WHEELCOMBO, CB_ADDSTRING, 0, (LPARAM)L"Seek items on hover else grid scroll");
	} break;
	case WM_TRAY: {
		switch (LOWORD(lParam))
		{
		case WM_RBUTTONUP:
		{
			POINT pt;
			GetCursorPos(&pt);
			HMENU hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_TRAY));
			if (hMenu) {
				HMENU hSubMenu = GetSubMenu(hMenu, 0);
				SetForegroundWindow(hDlg); // Workaround for menu not disappearing bug
				TrackPopupMenu(hSubMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hDlg, nullptr);
			}
			DestroyMenu(hMenu);
		} break;
		case WM_LBUTTONDBLCLK:
			ShowConfDlg(hDlg);
		}
	} break;
	case WM_COMMAND: {
		switch (LOWORD(wParam))
		{
		case ID_TRAY_PROPERTIES:
		{
			ShowConfDlg(hDlg);
		} break;
		case ID_TRAY_EXIT:
			DestroyWindow(hDlg);
			break;
		case IDC_GITHUB:
			ShellExecuteW(NULL, L"open", L"https://github.com/Ingan121/ClassicWindowSwitcher", NULL, NULL, SW_SHOWNORMAL);
			break;
		case IDOK:
		case IDAPPLY:
		{
			SaveSettings(hDlg);
			if (g_pSwitcher)
			{
				g_pSwitcher->bIncludeWallpaper = IsDlgButtonChecked(hDlg, IDC_INCLUDEWP);
				g_pSwitcher->dwWallpaperSupport = g_pSwitcher->bIncludeWallpaper ? SWS_WALLPAPERSUPPORT_EXPLORER : SWS_WALLPAPERSUPPORT_NONE;
				g_pSwitcher->bPrimaryOnly = IsDlgButtonChecked(hDlg, IDC_PRIMARYONLY);
				g_pSwitcher->bPerMonitor = IsDlgButtonChecked(hDlg, IDC_PERMONITOR);
				g_pSwitcher->bNoPerApplicationList = IsDlgButtonChecked(hDlg, IDC_NO_PERAPP_LIST);
				g_pSwitcher->bSwitcherIsPerApplication = IsDlgButtonChecked(hDlg, IDC_PERAPP);
				g_pSwitcher->bAlwaysUseWindowTitleAndIcon = IsDlgButtonChecked(hDlg, IDC_ALWAYS_WIN_TITLE);
				g_pSwitcher->bScrollWheelInvert = IsDlgButtonChecked(hDlg, IDC_INVWHEEL);
				g_pSwitcher->bSkipIfOneWindow = IsDlgButtonChecked(hDlg, IDC_SKIPIFONE);
				g_pSwitcher->dwShowDelay = GetDlgItemInt(hDlg, IDC_SHOWDELAY, NULL, FALSE);
				g_pSwitcher->dwGridColumns = GetDlgItemInt(hDlg, IDC_COLUMN, NULL, FALSE);
				g_pSwitcher->dwGridRows = GetDlgItemInt(hDlg, IDC_ROW, NULL, FALSE);
				g_pSwitcher->dwScrollWheelBehavior = (UINT)SendDlgItemMessageW(hDlg, IDC_WHEELCOMBO, CB_GETCURSEL, 0, 0);
			}
			else
			{
				HWND existingSwitcher = FindWindowA(SWS_WINDOWSWITCHER_CLASSNAME, NULL);
				if (existingSwitcher)
				{
					SendMessageW(existingSwitcher, SWS_WINDOWSWITCHER_RELOAD_CONFIG_MSG, 0, 0);
				}
			}
			if (LOWORD(wParam) == IDOK)
			{
				HideConfDlg(hDlg);
			}
		} break;
		case IDCANCEL:
			HideConfDlg(hDlg);
			break;
		}
	} break;
	case WM_CLOSE:
		HideConfDlg(hDlg);
		break;
	case WM_DESTROY: {
		if (!g_isStandaloneConfigurator) {
			NOTIFYICONDATA nid = {};
			nid.cbSize = sizeof(NOTIFYICONDATA);
			nid.hWnd = hDlg;
			nid.uID = 1;
			Shell_NotifyIconW(NIM_DELETE, &nid);
		}
		PostQuitMessage(0);
	} break;
	default:
		return FALSE;
	}
	return TRUE;
}

int WINAPI WinMain(
	_In_ HINSTANCE hInstance,
	_In_ HINSTANCE hPrevInstance,
	_In_ LPSTR     lpCmdLine,
	_In_ int       nCmdShow
)
{
	g_hInst = hInstance;

	if (FindWindowA(SWS_WINDOWSWITCHER_CLASSNAME, NULL) || strcmp(lpCmdLine, "/config") == 0)
	{
		g_isStandaloneConfigurator = true;
		HWND hWnd = CreateDialogW(
			hInstance,
			MAKEINTRESOURCE(IDD_CONFDLG),
			NULL,
			DlgProc
		);
		if (!hWnd)
		{
			MessageBoxW(NULL, L"Failed to create a configurator window", L"ClassicWindowSwitcher Error", MB_OK | MB_ICONERROR);
			return 1;
		}
		ShowConfDlg(hWnd);
		MSG msg;
		while (GetMessageW(&msg, NULL, 0, 0))
		{
			if (!IsDialogMessageW(hWnd, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessageW(&msg);
			}
		}
		return 0;
	}

	HMODULE hModule = LoadLibraryW(L"ClassicWindowSwitcher.dll");
	if (!hModule)
	{
		MessageBoxW(NULL, L"Failed to load ClassicWindowSwitcher.dll", L"ClassicWindowSwitcher Error", MB_OK | MB_ICONERROR);
		return 1;
	}
	void* main = GetProcAddress(hModule, "sws_main");
	if (!main)
	{
		MessageBoxW(NULL, L"Failed to find main function in ClassicWindowSwitcher.dll", L"ClassicWindowSwitcher Error", MB_OK | MB_ICONERROR);
		FreeLibrary(hModule);
		return 1;
	}
	g_unloadFn = GetProcAddress(hModule, "sws_unload");
	g_getInstanceFn = GetProcAddress(hModule, "sws_getInstance");

	HWND hWnd = CreateDialogW(
		hInstance,
		MAKEINTRESOURCE(IDD_CONFDLG),
		NULL,
		DlgProc
	);
	if (!hWnd)
	{
		MessageBoxW(NULL, L"Failed to create a tray window", L"ClassicWindowSwitcher Error", MB_OK | MB_ICONERROR);
		FreeLibrary(hModule);
		return 1;
	}

	HANDLE hThread = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)main,
		(LPVOID)SWS_WINDOWSWITCHER_INITFLAG_NO_CONF_RELOAD_ON_SHOW,
		0,
		NULL
	);

	MSG msg;
	if (hThread)
	{
		HANDLE waitHandles[1] = { hThread };
		BOOL bQuit = FALSE;
		while (TRUE)
		{
			DWORD result = MsgWaitForMultipleObjects(
				1,
				waitHandles,
				FALSE,
				INFINITE,
				QS_ALLINPUT
			);

			if (result == WAIT_OBJECT_0)
			{
				DWORD exitCode;
				GetExitCodeThread(hThread, &exitCode);
				if (exitCode)
				{
					WCHAR errorMsg[256];
					FormatMessageW(
						FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
						NULL,
						exitCode,
						MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
						errorMsg,
						256,
						NULL
					);
					if (exitCode == 0x80070581) // ERROR_HOTKEY_ALREADY_REGISTERED
					{
						wcscat_s(errorMsg, L"\nMake sure you have disabled explorer.exe's default Alt+Tab.");
					}
					MessageBoxW(NULL, errorMsg, L"ClassicWindowSwitcher Error", MB_OK | MB_ICONERROR);
				}
				// Switcher thread exited first
				DestroyWindow(hWnd);
				break;
			}
			else if (result == WAIT_OBJECT_0 + 1)
			{
				while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
				{
					if (msg.message == WM_QUIT)
					{
						bQuit = TRUE;
						break;
					}
					if (!IsDialogMessageW(hWnd, &msg))
					{
						TranslateMessage(&msg);
						DispatchMessageW(&msg);
					}
				}
				if (bQuit)
				{
					// Tray window exited first
					break;
				}
			}
			else
			{
				break;
			}
		}
	}
	else
	{
		MessageBoxW(NULL, L"Failed to create a window switcher thread", L"ClassicWindowSwitcher Error", MB_OK | MB_ICONERROR);
		DestroyWindow(hWnd);
		FreeLibrary(hModule);
		return 1;
	}

	if (g_unloadFn)
	{
		((void(*)())g_unloadFn)();
	}
	if (hThread)
	{
		WaitForSingleObject(hThread, INFINITE);
		CloseHandle(hThread);
	}

	FreeLibrary(hModule);
	return 0;
}

void SaveSettings(HWND hDlg)
{
	DWORD includeWallpaper = IsDlgButtonChecked(hDlg, IDC_INCLUDEWP);
	DWORD primaryOnly = IsDlgButtonChecked(hDlg, IDC_PRIMARYONLY);
	DWORD perMonitor = IsDlgButtonChecked(hDlg, IDC_PERMONITOR);
	DWORD noPerAppList = IsDlgButtonChecked(hDlg, IDC_NO_PERAPP_LIST);
	DWORD perApp = IsDlgButtonChecked(hDlg, IDC_PERAPP);
	DWORD alwaysWinTitle = IsDlgButtonChecked(hDlg, IDC_ALWAYS_WIN_TITLE);
	DWORD invWheel = IsDlgButtonChecked(hDlg, IDC_INVWHEEL);
	DWORD skipIfOne = IsDlgButtonChecked(hDlg, IDC_SKIPIFONE);
	DWORD showDelay = GetDlgItemInt(hDlg, IDC_SHOWDELAY, NULL, FALSE);
	DWORD scrollWheelBehavior = (UINT)SendDlgItemMessageW(hDlg, IDC_WHEELCOMBO, CB_GETCURSEL, 0, 0);
	DWORD gridColumns = GetDlgItemInt(hDlg, IDC_COLUMN, NULL, FALSE);
	DWORD gridRows = GetDlgItemInt(hDlg, IDC_ROW, NULL, FALSE);

	HKEY hKey;
	if (RegCreateKeyExW(
		HKEY_CURRENT_USER,
		SWS_REGISTRY_KEY,
		0,
		NULL,
		REG_OPTION_NON_VOLATILE,
		KEY_WRITE,
		NULL,
		&hKey,
		NULL
	) == ERROR_SUCCESS)
	{
		RegSetValueExW(hKey, L"IncludeWallpaper", 0, REG_DWORD, (BYTE*)&includeWallpaper, sizeof(DWORD));
		RegSetValueExW(hKey, L"PrimaryOnly", 0, REG_DWORD, (BYTE*)&primaryOnly, sizeof(DWORD));
		RegSetValueExW(hKey, L"PerMonitor", 0, REG_DWORD, (BYTE*)&perMonitor, sizeof(DWORD));
		RegSetValueExW(hKey, L"NoPerApplicationList", 0, REG_DWORD, (BYTE*)&noPerAppList, sizeof(DWORD));
		RegSetValueExW(hKey, L"SwitcherIsPerApplication", 0, REG_DWORD, (BYTE*)&perApp, sizeof(DWORD));
		RegSetValueExW(hKey, L"AlwaysUseWindowTitleAndIcon", 0, REG_DWORD, (BYTE*)&alwaysWinTitle, sizeof(DWORD));
		RegSetValueExW(hKey, L"ScrollWheelInvert", 0, REG_DWORD, (BYTE*)&invWheel, sizeof(DWORD));
		RegSetValueExW(hKey, L"SkipIfOneWindow", 0, REG_DWORD, (BYTE*)&skipIfOne, sizeof(DWORD));
		RegSetValueExW(hKey, L"ShowDelay", 0, REG_DWORD, (BYTE*)&showDelay, sizeof(DWORD));
		RegSetValueExW(hKey, L"ScrollWheelBehavior", 0, REG_DWORD, (BYTE*)&scrollWheelBehavior, sizeof(DWORD));
		RegCloseKey(hKey);
	}
	if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Desktop", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS)
	{
		WCHAR valueData[16];
		swprintf_s(valueData, L"%d", gridColumns);
		RegSetValueExW(hKey, L"CoolSwitchColumns", 0, REG_SZ, (BYTE*)valueData, (lstrlenW(valueData) + 1) * sizeof(WCHAR));
		swprintf_s(valueData, L"%d", gridRows);
		RegSetValueExW(hKey, L"CoolSwitchRows", 0, REG_SZ, (BYTE*)valueData, (lstrlenW(valueData) + 1) * sizeof(WCHAR));
		RegCloseKey(hKey);
	}
}

void ShowConfDlg(HWND hDlg)
{
	if (!g_pSwitcher && g_getInstanceFn)
	{
		g_pSwitcher = ((sws_WindowSwitcher * (*)())g_getInstanceFn)();
	}
	if (g_pSwitcher)
	{
		CheckDlgButton(hDlg, IDC_INCLUDEWP, g_pSwitcher->bIncludeWallpaper);
		CheckDlgButton(hDlg, IDC_PRIMARYONLY, g_pSwitcher->bPrimaryOnly);
		CheckDlgButton(hDlg, IDC_PERMONITOR, g_pSwitcher->bPerMonitor);
		CheckDlgButton(hDlg, IDC_NO_PERAPP_LIST, g_pSwitcher->bNoPerApplicationList);
		CheckDlgButton(hDlg, IDC_PERAPP, g_pSwitcher->bSwitcherIsPerApplication);
		CheckDlgButton(hDlg, IDC_ALWAYS_WIN_TITLE, g_pSwitcher->bAlwaysUseWindowTitleAndIcon);
		CheckDlgButton(hDlg, IDC_INVWHEEL, g_pSwitcher->bScrollWheelInvert);
		CheckDlgButton(hDlg, IDC_SKIPIFONE, g_pSwitcher->bSkipIfOneWindow);
		SetDlgItemInt(hDlg, IDC_SHOWDELAY, g_pSwitcher->dwShowDelay, FALSE);
		SetDlgItemInt(hDlg, IDC_COLUMN, g_pSwitcher->dwGridColumns, FALSE);
		SetDlgItemInt(hDlg, IDC_ROW, g_pSwitcher->dwGridRows, FALSE);
		SendDlgItemMessageW(hDlg, IDC_WHEELCOMBO, CB_SETCURSEL, g_pSwitcher->dwScrollWheelBehavior, 0);
	}
	else
	{
		HKEY hKey;
		if (RegOpenKeyExW(HKEY_CURRENT_USER, SWS_REGISTRY_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
		{
			DWORD dwType, dwData, dwSize = sizeof(dwData);
			if (RegQueryValueExW(hKey, L"ShowDelay", NULL, &dwType, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
			{
				SetDlgItemInt(hDlg, IDC_SHOWDELAY, dwData, FALSE);
			}
			else
			{
				SetDlgItemInt(hDlg, IDC_SHOWDELAY, SWS_WINDOWSWITCHER_SHOWDELAY, FALSE);
			}
			if (RegQueryValueExW(hKey, L"IncludeWallpaper", NULL, &dwType, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
			{
				CheckDlgButton(hDlg, IDC_INCLUDEWP, dwData);
			}
			else
			{
				CheckDlgButton(hDlg, IDC_INCLUDEWP, FALSE);
			}
			if (RegQueryValueExW(hKey, L"PrimaryOnly", NULL, &dwType, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
			{
				CheckDlgButton(hDlg, IDC_PRIMARYONLY, dwData);
			}
			else
			{
				CheckDlgButton(hDlg, IDC_PRIMARYONLY, FALSE);
			}
			if (RegQueryValueExW(hKey, L"PerMonitor", NULL, &dwType, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
			{
				CheckDlgButton(hDlg, IDC_PERMONITOR, dwData);
			}
			else
			{
				CheckDlgButton(hDlg, IDC_PERMONITOR, FALSE);
			}
			if (RegQueryValueExW(hKey, L"NoPerApplicationList", NULL, &dwType, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
			{
				CheckDlgButton(hDlg, IDC_NO_PERAPP_LIST, dwData);
			}
			else
			{
				CheckDlgButton(hDlg, IDC_NO_PERAPP_LIST, FALSE);
			}
			if (RegQueryValueExW(hKey, L"SwitcherIsPerApplication", NULL, &dwType, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
			{
				CheckDlgButton(hDlg, IDC_PERAPP, dwData);
			}
			else
			{
				CheckDlgButton(hDlg, IDC_PERAPP, FALSE);
			}
			if (RegQueryValueExW(hKey, L"AlwaysUseWindowTitleAndIcon", NULL, &dwType, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
			{
				CheckDlgButton(hDlg, IDC_ALWAYS_WIN_TITLE, dwData);
			}
			else
			{
				CheckDlgButton(hDlg, IDC_ALWAYS_WIN_TITLE, FALSE);
			}
			if (RegQueryValueExW(hKey, L"ScrollWheelBehavior", NULL, &dwType, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
			{
				if (dwData > SWS_SCROLLWHEELBEHAVIOR_EVERYWHERE_GRIDSCROLL)
				{
					CheckDlgButton(hDlg, IDC_WHEELCOMBO, SWS_SCROLLWHEELBEHAVIOR_DISABLED);
				}
				else
				{
					SendDlgItemMessageW(hDlg, IDC_WHEELCOMBO, CB_SETCURSEL, dwData, 0);
				}
			}
			else
			{
				SendDlgItemMessageW(hDlg, IDC_WHEELCOMBO, CB_SETCURSEL, SWS_SCROLLWHEELBEHAVIOR_DISABLED, 0);
			}
			if (RegQueryValueExW(hKey, L"ScrollWheelInvert", NULL, &dwType, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
			{
				CheckDlgButton(hDlg, IDC_INVWHEEL, dwData);
			}
			else
			{
				CheckDlgButton(hDlg, IDC_INVWHEEL, FALSE);
			}
			if (RegQueryValueExW(hKey, L"SkipIfOneWindow", NULL, &dwType, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
			{
				CheckDlgButton(hDlg, IDC_SKIPIFONE, dwData);
			}
			else
			{
				CheckDlgButton(hDlg, IDC_SKIPIFONE, TRUE);
			}
			RegCloseKey(hKey);
		}

		if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Desktop", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
		{
			WCHAR wszValue[16];
			DWORD dwSize = sizeof(wszValue);
			DWORD dwGridColumns = SWS_WINDOWSWITCHERLAYOUT_DEFAULT_GRID_COLUMNS;
			DWORD dwGridRows = SWS_WINDOWSWITCHERLAYOUT_DEFAULT_GRID_ROWS;
			if (RegQueryValueExW(hKey, L"CoolSwitchColumns", NULL, NULL, (LPBYTE)wszValue, &dwSize) == ERROR_SUCCESS)
			{
				dwGridColumns = (UINT)_wtoi(wszValue);
				if (dwGridColumns < 1)
				{
					dwGridColumns = SWS_WINDOWSWITCHERLAYOUT_DEFAULT_GRID_COLUMNS;
				}
			}
			dwSize = sizeof(wszValue);
			if (RegQueryValueExW(hKey, L"CoolSwitchRows", NULL, NULL, (LPBYTE)wszValue, &dwSize) == ERROR_SUCCESS)
			{
				dwGridRows = (UINT)_wtoi(wszValue);
				if (dwGridRows < 1)
				{
					dwGridRows = SWS_WINDOWSWITCHERLAYOUT_DEFAULT_GRID_ROWS;
				}
			}
			SetDlgItemInt(hDlg, IDC_COLUMN, dwGridColumns, FALSE);
			SetDlgItemInt(hDlg, IDC_ROW, dwGridRows, FALSE);
			RegCloseKey(hKey);
		}
	}
	int screenX = GetSystemMetrics(SM_CXSCREEN);
	int screenY = GetSystemMetrics(SM_CYSCREEN);
	RECT rect;
	GetWindowRect(hDlg, &rect);
	SetWindowPos(hDlg, NULL, (screenX - (rect.right - rect.left)) / 2, (screenY - (rect.bottom - rect.top)) / 2, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	ShowWindow(hDlg, SW_SHOW);
	SetForegroundWindow(hDlg);
}

void HideConfDlg(HWND hDlg)
{
	if (!g_isStandaloneConfigurator)
	{
		ShowWindow(hDlg, SW_HIDE);
	}
	else
	{
		DestroyWindow(hDlg);
	}
}