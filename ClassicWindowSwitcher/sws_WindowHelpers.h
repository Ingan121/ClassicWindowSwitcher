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

extern ULONG_PTR _sws_gdiplus_token;

// References:
// RealEnumWindows: https://stackoverflow.com/questions/38205375/enumwindows-function-in-win10-enumerates-only-desktop-apps
// IsAltTabWindow: https://devblogs.microsoft.com/oldnewthing/20071008-00/?p=24863
// GetIconFromHWND: https://github.com/cairoshell/ManagedShell/blob/master/src/ManagedShell.WindowsTasks/ApplicationWindow.cs

// bcc18b79-ba16-442f-80c4-8a59c30c463b
DEFINE_GUID(__uuidof_IShellItemImageFactory,
	0xbcc18b79,
	0xba16, 0x442f, 0x80, 0xc4,
	0x8a, 0x59, 0xc3, 0x0c, 0x46, 0x3b
);

DEFINE_GUID(__uuidof_IShellItem2,
	0x7e9fb0d3,
	0x919f, 0x4307, 0xab, 0x2e,
	0x9b, 0x18, 0x60, 0x31, 0x0c, 0x93
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

// {46a6eeff-908e-4dc6-92a6-64be9177b41c}
DEFINE_GUID(IID_IAppResolver_7, 0x46a6eeff, 0x908e, 0x4dc6, 0x92, 0xa6, 0x64, 0xbe, 0x91, 0x77, 0xb4, 0x1c);

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

#define STATUS_BUFFER_TOO_SMALL 0xC0000023
typedef NTSTATUS(WINAPI* NtUserBuildHwndList)
(
	HDESK in_hDesk,
	HWND  in_hWndNext,
	BOOL  in_EnumChildren,
	BOOL  in_RemoveImmersive,
	DWORD in_ThreadID,
	UINT  in_Max,
	HWND* out_List,
	UINT* out_Cnt
	);
extern HMODULE _sws_hWin32u;
extern NtUserBuildHwndList _sws_pNtUserBuildHwndList;

typedef struct
{
	int nAccentState;
	int nFlags;
	int nColor;
	int nAnimationId;
} ACCENTPOLICY;
typedef struct
{
	int nAttribute;
	PVOID pData;
	ULONG ulDataSize;
} WINCOMPATTRDATA;
extern HINSTANCE _sws_hUser32;
typedef BOOL(WINAPI* pSetWindowCompositionAttribute)(HWND, WINCOMPATTRDATA*);
extern pSetWindowCompositionAttribute _sws_SetWindowCompositionAttribute;
typedef BOOL(WINAPI* pIsShellManagedWindow)(HWND);
extern pIsShellManagedWindow _sws_IsShellManagedWindow;
extern pIsShellManagedWindow sws_IsShellFrameWindow;
typedef HWND(WINAPI* pHungWindowFromGhostWindow)(HWND);
extern pHungWindowFromGhostWindow _sws_HungWindowFromGhostWindow;
typedef HWND(WINAPI* pGhostWindowFromHungWindow)(HWND);
extern pGhostWindowFromHungWindow _sws_GhostWindowFromHungWindow;
typedef HICON(WINAPI* pInternalGetWindowIcon)(HWND,UINT);
extern pInternalGetWindowIcon _sws_InternalGetWindowIcon;
typedef HWND(WINAPI* pIsCoreWindow)(HWND);
extern pIsCoreWindow _sws_IsCoreWindow;

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

typedef BOOL(WINAPI* pSetWindowBand)(HWND hWnd, HWND hwndInsertAfter, DWORD dwBand);
extern pSetWindowBand _sws_SetWindowBand;

typedef BOOL(WINAPI* pGetWindowBand)(HWND hWnd, PDWORD pdwBand);
extern pGetWindowBand _sws_GetWindowBand;

extern FARPROC sws_SHRegGetValueFromHKCUHKLM;

extern BOOL(*_sws_ShouldSystemUseDarkMode)();
extern void(*_sws_RefreshImmersiveColorPolicyState)();
extern HINSTANCE _sws_hUxtheme;

BOOL(*_sws_IsTopLevelWindow)(HWND);

HWND(*_sws_GetProgmanWindow)();

int(*sws_InternalGetWindowText)(HWND, LPWSTR, int);

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

static BOOL _sws_IsBandValidToInclude[19] = { FALSE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, FALSE, FALSE };

HWND* _sws_WindowHelpers_Gui_BuildWindowList
(
	NtUserBuildHwndList pNtUserBuildHwndList,
	HDESK in_hDesk,
	HWND  in_hWnd,
	BOOL  in_EnumChildren,
	BOOL  in_RemoveImmersive,
	UINT  in_ThreadID,
	INT* out_Cnt
);

/********************************************************/
/* enumerate all top level windows including metro apps */
/********************************************************/
sws_error_t sws_WindowHelpers_RealEnumWindows(
	WNDENUMPROC in_Proc,
	LPARAM in_Param
);

wchar_t* sws_WindowHelpers_GetAUMIDForHWND(HWND hWnd);

BOOL sws_WindowHelpers_IsValidMonitor(HMONITOR hMonitor, HDC unnamedParam2, LPRECT unnamedParam3, HMONITOR* pMonitor);

BOOL sws_WindowHelpers_IsTaskbarWindow(HWND hWnd, HWND hWndWallpaper);

BOOL sws_WindowHelpers_IsAltTabWindow(HWND hwnd);

void sws_WindowHelpers_GetDesktopText(wchar_t* wszTitle);

BOOL sws_WindowHelpers_EnsureWallpaperHWND();

HWND sws_WindowHelpers_GetWallpaperHWND();

HBITMAP sws_WindowHelpers_CreateAlphaTextBitmap(LPCWSTR inText, HFONT inFont, DWORD dwTextFlags, SIZE size, COLORREF inColour);

HWND sws_WindowHelpers_GetLastActivePopup(HWND hWnd);

void sws_WindowHelpers_Clear();

sws_error_t sws_WindowHelpers_Initialize();

extern BOOL g_bIsDesktopRaised;
inline BOOL _sws_WindowHelpers_IsDesktopRaised()
{
	return g_bIsDesktopRaised;
}

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
	return sws_IsShellFrameWindow(hWnd);
}

BOOL CALLBACK sws_WindowHelpers_AddAltTabWindowsToTimeStampedHWNDList(HWND hWnd, HDPA hdpa);

BOOL sws_WindowHelpers_AreAnimationsAllowed();

void sws_WindowHelpers_GetWindowText(HWND hWnd, LPCWSTR lpWStr, DWORD dwLength);
#endif