#ifndef _H_SWS_WINDOWHELPERS_H_
#define _H_SWS_WINDOWHELPERS_H_
#include <initguid.h>
#include <Windows.h>
#include <psapi.h>
#include <propsys.h>
#include <appmodel.h>
#include <shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
#include <dwmapi.h>
#pragma comment(lib, "Dwmapi.lib")
#include <ShlObj.h>
#include <commctrl.h>
#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Gdiplus.lib")
#include <ExDisp.h>
#include <ShlGuid.h>
#include <Propkey.h>
#include "sws_def.h"
#include "sws_error.h"
#include "sws_tshwnd.h"
//#include "sws_WindowSwitcher.h"
//#include "sws_WindowSwitcherLayoutWindow.h"

extern ULONG_PTR _sws_gdiplus_token;

// References:
// IsAltTabWindow: https://devblogs.microsoft.com/oldnewthing/20071008-00/?p=24863
// GetIconFromHWND: https://github.com/cairoshell/ManagedShell/blob/master/src/ManagedShell.WindowsTasks/ApplicationWindow.cs

// bcc18b79-ba16-442f-80c4-8a59c30c463b
DEFINE_GUID(__uuidof_IShellItemImageFactory,
	0xbcc18b79,
	0xba16, 0x442f, 0x80, 0xc4,
	0x8a, 0x59, 0xc3, 0x0c, 0x46, 0x3b
);

DEFINE_GUID(__uuidof_IPropertyStore,
	0x886D8EEB,
	0x8CF2, 0x4446, 0x8D, 0x02,
	0xCD, 0xBA, 0x1D, 0xBD, 0xCF, 0x99
);

DEFINE_GUID(__uuidof_AppUserModelIdProperty,
	0x9F4C2855,
	0x9F79, 0x4B39, 0xA8, 0xD0,
	0xE1, 0xD4, 0x2D, 0xE1, 0xD5, 0xF3
);

// https://gist.github.com/m417z/451dfc2dad88d7ba88ed1814779a26b4

// {c8900b66-a973-584b-8cae-355b7f55341b}
DEFINE_GUID(CLSID_StartMenuCacheAndAppResolver, 0x660b90c8, 0x73a9, 0x4b58, 0x8c, 0xae, 0x35, 0x5b, 0x7f, 0x55, 0x34, 0x1b);

// {de25675a-72de-44b4-9373-05170450c140}
DEFINE_GUID(IID_IAppResolver_8, 0xde25675a, 0x72de, 0x44b4, 0x93, 0x73, 0x05, 0x17, 0x04, 0x50, 0xc1, 0x40);

typedef interface IAppResolver_8 IAppResolver_8;

typedef struct IAppResolver_8Vtbl
{
	BEGIN_INTERFACE

	HRESULT(STDMETHODCALLTYPE* QueryInterface)(
		IAppResolver_8* This,
		/* [in] */ REFIID riid,
		/* [annotation][iid_is][out] */
		_COM_Outptr_  void** ppvObject);

	ULONG(STDMETHODCALLTYPE* AddRef)(
		IAppResolver_8* This);

	ULONG(STDMETHODCALLTYPE* Release)(
		IAppResolver_8* This);

	HRESULT (STDMETHODCALLTYPE* GetAppIDForShortcut)(IAppResolver_8* This);
	HRESULT (STDMETHODCALLTYPE* GetAppIDForShortcutObject)(IAppResolver_8* This);
	HRESULT (STDMETHODCALLTYPE* GetAppIDForWindow)(IAppResolver_8* This, HWND hWnd, WCHAR** pszAppId, int* pUnknown1, int* pUnknown2, int* pUnknown3);
	HRESULT (STDMETHODCALLTYPE* GetAppIDForProcess)(IAppResolver_8* This, DWORD dwProcessId, WCHAR** pszAppId, int* pUnknown1, int* pUnknown2, int* pUnknown3);

	END_INTERFACE
} IAppResolver_8Vtbl;

interface IAppResolver_8
{
	CONST_VTBL struct IAppResolver_8Vtbl* lpVtbl;
};

typedef BOOL(WINAPI* pIsShellManagedWindow)(HWND);
extern pIsShellManagedWindow sws_IsShellFrameWindow;
typedef HWND(WINAPI* pHungWindowFromGhostWindow)(HWND);
extern pHungWindowFromGhostWindow _sws_HungWindowFromGhostWindow;
typedef HWND(WINAPI* pGhostWindowFromHungWindow)(HWND);
extern pGhostWindowFromHungWindow _sws_GhostWindowFromHungWindow;
typedef DWORD_PTR(WINAPI* pForceFocusBasedMouseWheelRouting)(BOOL enabled);
extern pForceFocusBasedMouseWheelRouting _sws_ForceFocusBasedMouseWheelRouting;
typedef BOOL(WINAPI* pSHWindowsPolicy)(REFGUID riid);

typedef HWND(WINAPI* pCreateWindowInBand)(
	_In_ DWORD dwExStyle,
	_In_opt_ LPCWSTR lpClassName,
	_In_opt_ LPCWSTR lpWindowName,
	_In_ DWORD dwStyle,
	_In_ int X,
	_In_ int Y,
	_In_ int nWidth,
	_In_ int nHeight,
	_In_opt_ HWND hWndParent,
	_In_opt_ HMENU hMenu,
	_In_opt_ HINSTANCE hInstance,
	_In_opt_ LPVOID lpParam,
	DWORD band
	);
extern pCreateWindowInBand _sws_CreateWindowInBand;

FILETIME sws_start_ft;
FILETIME sws_ancient_ft;

HICON sws_DefAppIcon;
HICON sws_LegacyDefAppIcon;

IAppResolver_8* sws_AppResolver;

inline FILETIME sws_WindowHelpers_GetStartTime()
{
	return sws_start_ft;
}

inline FILETIME sws_WindowHelpers_GetAncientTime()
{
	ULARGE_INTEGER uli;
	uli.LowPart = sws_ancient_ft.dwLowDateTime;
	uli.HighPart = sws_ancient_ft.dwHighDateTime;
	uli.QuadPart--;
	sws_ancient_ft.dwHighDateTime = uli.HighPart;
	sws_ancient_ft.dwLowDateTime = uli.LowPart;
	return sws_ancient_ft;
}

enum ZBID
{
	ZBID_DEFAULT = 0,
	ZBID_DESKTOP = 1,
	ZBID_UIACCESS = 2,
	ZBID_IMMERSIVE_IHM = 3,
	ZBID_IMMERSIVE_NOTIFICATION = 4,
	ZBID_IMMERSIVE_APPCHROME = 5,
	ZBID_IMMERSIVE_MOGO = 6,
	ZBID_IMMERSIVE_EDGY = 7,
	ZBID_IMMERSIVE_INACTIVEMOBODY = 8,
	ZBID_IMMERSIVE_INACTIVEDOCK = 9,
	ZBID_IMMERSIVE_ACTIVEMOBODY = 10,
	ZBID_IMMERSIVE_ACTIVEDOCK = 11,
	ZBID_IMMERSIVE_BACKGROUND = 12,
	ZBID_IMMERSIVE_SEARCH = 13,
	ZBID_GENUINE_WINDOWS = 14,
	ZBID_IMMERSIVE_RESTRICTED = 15,
	ZBID_SYSTEM_TOOLS = 16,
	ZBID_LOCK = 17,
	ZBID_ABOVELOCK_UX = 18,
};

wchar_t* sws_WindowHelpers_GetAUMIDForHWND(HWND hWnd);

BOOL sws_WindowHelpers_IsValidMonitor(HMONITOR hMonitor, HDC unnamedParam2, LPRECT unnamedParam3, HMONITOR* pMonitor);

BOOL sws_WindowHelpers_IsAltTabWindow(HWND hwnd);

void sws_WindowHelpers_GetDesktopText(wchar_t* wszTitle);

HWND sws_WindowHelpers_GetLastActivePopup(HWND hWnd);

void sws_WindowHelpers_Clear();

sws_error_t sws_WindowHelpers_Initialize();

inline void _sws_WindowHelpers_ToggleDesktop()
{
	keybd_event(VK_LMENU, 0, KEYEVENTF_KEYUP, 0); // ensure alt is up
	keybd_event(VK_LWIN, 0, 0, 0);
	keybd_event('D', 0, 0, 0);
	keybd_event('D', 0, KEYEVENTF_KEYUP, 0);
	keybd_event(VK_LWIN, 0, KEYEVENTF_KEYUP, 0);
}

inline BOOL sws_WindowHelpers_IsWindowUWP(HWND hWnd)
{
	return sws_IsShellFrameWindow && sws_IsShellFrameWindow(hWnd);
}

BOOL CALLBACK sws_WindowHelpers_AddAltTabWindowsToTimeStampedHWNDList(HWND hWnd, HDPA hdpa);

BOOL sws_WindowHelpers_AreAnimationsAllowed();

void sws_WindowHelpers_GetWindowText(HWND hWnd, LPCWSTR lpWStr, DWORD dwLength);

//extern struct sws_WindowSwitcher;
//extern struct sws_WindowSwitcherLayoutWindow;
//void sws_WindowHelpers_GetDesiredWindowText(sws_WindowSwitcher* _this, sws_WindowSwitcherLayoutWindow* window, LPWSTR wszTitle);
#endif