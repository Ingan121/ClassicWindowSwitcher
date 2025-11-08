#include <Windows.h>
#include "resource.h"

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define CWS_WINDOWSWITCHER_CLASSNAME L"ClassicWindowSwitcher_{BEA057BB-66C7-4758-A610-FAE6013E9F98}"

#define WM_TRAY (WM_USER + 1)

HINSTANCE g_hInst;
void* g_unloadFn = nullptr;

long long CALLBACK DlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG: {
		NOTIFYICONDATA nid = {};
		nid.cbSize = sizeof(NOTIFYICONDATA);
		nid.hWnd = hwndDlg;
		nid.uID = 1;
		nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
		nid.uCallbackMessage = WM_TRAY;
		nid.hIcon = LoadIcon(g_hInst, MAKEINTRESOURCE(IDI_CWS));
		wcscpy_s(nid.szTip, L"ClassicWindowSwitcher");
		Shell_NotifyIcon(NIM_ADD, &nid);
	} break;
	case WM_TRAY: {
		switch (LOWORD(lParam))
		{
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		{
			POINT pt;
			GetCursorPos(&pt);
			HMENU hMenu = LoadMenu(g_hInst, MAKEINTRESOURCE(IDR_TRAY));
			if (hMenu) {
				HMENU hSubMenu = GetSubMenu(hMenu, 0);
				SetForegroundWindow(hwndDlg); // Workaround for menu not disappearing bug
				TrackPopupMenu(hSubMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwndDlg, nullptr);
			}
			DestroyMenu(hMenu);
		} break;
		}
	} break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_TRAY_EXIT:
			EndDialog(hwndDlg, 0);
			PostQuitMessage(0);
			break;
		}
		break;
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

	if (FindWindowW(CWS_WINDOWSWITCHER_CLASSNAME, NULL))
	{
		MessageBoxW(NULL, L"ClassicWindowSwitcher is already running!", L"Info", MB_OK | MB_ICONINFORMATION);
		return 2;
	}

	HMODULE hModule = LoadLibraryW(L"ClassicWindowSwitcher.dll");
	if (!hModule)
	{
		MessageBoxW(NULL, L"Failed to load ClassicWindowSwitcher.dll", L"Error", MB_OK | MB_ICONERROR);
		return 1;
	}
	void* main = GetProcAddress(hModule, "sws_main");
	if (!main)
	{
		MessageBoxW(NULL, L"Failed to load ClassicWindowSwitcher.dll", L"Error", MB_OK | MB_ICONERROR);
		FreeLibrary(hModule);
		return 1;
	}
	g_unloadFn = GetProcAddress(hModule, "sws_unload");

	HWND hWnd = CreateDialogW(
		hInstance,
		MAKEINTRESOURCE(IDD_CONFDLG),
		NULL,
		DlgProc
	);
	if (!hWnd)
	{
		MessageBoxW(NULL, L"Failed to create a tray window", L"Error", MB_OK | MB_ICONERROR);
		FreeLibrary(hModule);
		return 1;
	}

	HANDLE hThread = CreateThread(
		NULL,
		0,
		(LPTHREAD_START_ROUTINE)main,
		0,
		0,
		NULL
	);

	MSG msg;
	while (GetMessageW(&msg, NULL, 0, 0))
	{
		if (!IsDialogMessageW(hWnd, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
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