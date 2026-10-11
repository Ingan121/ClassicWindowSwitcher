#ifndef _H_SWS_WINDOWSWITCHER_H_
#define _H_SWS_WINDOWSWITCHER_H_
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#include <initguid.h>
#include <Windows.h>
#include <windowsx.h>
#include <tchar.h>
#pragma comment(lib, "Uxtheme.lib")
#include <Shlobj_core.h>
#pragma comment(lib, "Shlwapi.lib")
#include <roapi.h>
#pragma comment(lib, "runtimeobject.lib")
#include <oleacc.h>
#include "sws_def.h"
#include "sws_error.h"
#include "sws_window.h"
#include "sws_utility.h"
#include "sws_WindowSwitcherLayout.h"
#include "sws_tshwnd.h"
#include "sws_WindowSwitcherSettings.h"

typedef struct _sws_WindowSwitcher
{
	DWORD dwInitFlags;
    BOOL bIsDynamic;
    HRESULT hrCo;
    HWND hWnd;
    UINT msgShellHook;
    sws_WindowSwitcherLayout layout;
    int initialDirection;
    int direction;
	int scrollDirection;
    int lastKey;
    HBRUSH hBackgroundBrush;
    BOOL bPartialRedraw;
    HWND hWndLast;
    BOOL bWasControl;
    HMONITOR hMonitor;
    INT cwIndex;
    DWORD cwMask;
    BOOL bIsMouseClicking;
    sws_vector pHWNDList;
    HDPA htshwnds;
    UINT mode;
    HWND lastMiniModehWnd;
    HWINEVENTHOOK hookForeground;
    HWINEVENTHOOK hookCreateDestroy;
    DWORD dwShowDelay;
    BOOL bPrimaryOnly;
    sws_IInputSwitchCallback InputSwitchCallback;
    sws_IInputSwitchControl* pInputSwitchControl;
    UINT vkTilde;
    HANDLE hShowThread;
    HANDLE hShowSignal;
    BOOL bIsInitialized;
    HWND hWndAccessible;
    IAccPropServices* pAccPropServices;
    HBRUSH hFlashBrush;
    DWORD dwPaintFlags;
    HANDLE hFlashAnimationThread;
    HANDLE hFlashAnimationSignal;
    HDC hdcWindow;
    HDC hdcPaint;
    HPAINTBUFFER hBufferedPaint;
    INT cwOldIndex;
    DWORD cwOldMask;
    long long lastUpdateTime;
    BOOL bShouldStartFlashTimerWhenShowing;
	BOOL bIsCursorOnSwitcher;
	BOOL bSkipIfOneWindow;

    sws_WindowSwitcherSettings settings;

	wchar_t wszVersionString[32];
} sws_WindowSwitcher;

typedef struct _sws_WindowSwitcher_EndTaskThreadParams
{
    HWND hWnd;
    sws_WindowSwitcher* sws;
    HDESK hDesktop;
} sws_WindowSwitcher_EndTaskThreadParams;

static void _sws_WindowSwitcher_UpdateAccessibleText(sws_WindowSwitcher* _this);

sws_error_t sws_WindowSwitcher_RegisterHotkeys(sws_WindowSwitcher* _this);

void sws_WindowSwitcher_UnregisterHotkeys(sws_WindowSwitcher* _this, BOOL tildeOnly);

void _sws_WindowSwitcher_SwitchToSelectedItemAndDismiss(sws_WindowSwitcher* _this);

static void WINAPI _sws_WindowSwitcher_Calculate(sws_WindowSwitcher* _this, sws_WindowSwitcherLayout* pOldLayout);

static sws_error_t _sws_WindowSwitcher_GetCloseButtonRectFromIndex(sws_WindowSwitcher* _this, DWORD dwIndex, LPRECT lpRect);

static LRESULT _sws_WindowsSwitcher_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

static sws_error_t _sws_WindowSwitcher_RegisterWindowClass(sws_WindowSwitcher* _this);

__declspec(dllexport) sws_error_t sws_WindowSwitcher_RunMessageQueue(sws_WindowSwitcher* _this);

__declspec(dllexport) void sws_WindowSwitcher_Clear(sws_WindowSwitcher* _this);

__declspec(dllexport) sws_error_t sws_WindowSwitcher_Initialize(sws_WindowSwitcher** __this, DWORD initFlags);

void sws_WindowSwitcher_Paint(sws_WindowSwitcher* _this, DWORD dwFlags);

void sws_WindowSwitcher_InitializeDefaultSettings(sws_WindowSwitcher* _this);

__declspec(dllexport) void sws_WindowSwitcher_LoadSettings(sws_WindowSwitcher* _this);

#endif
