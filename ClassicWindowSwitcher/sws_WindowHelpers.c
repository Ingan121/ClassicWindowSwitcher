#include "sws_WindowHelpers.h"
#include "sws_WindowSwitcher.h"

ULONG_PTR _sws_gdiplus_token;
pCreateWindowInBand _sws_CreateWindowInBand;
pIsShellManagedWindow _sws_IsShellManagedWindow;
pIsShellManagedWindow sws_IsShellFrameWindow;
HINSTANCE _sws_hUser32 = 0;
HINSTANCE _sws_hShcore = 0;
HMODULE _sws_ExplorerFrame = 0;
HMODULE _sws_Explorer = 0;
pHungWindowFromGhostWindow _sws_HungWindowFromGhostWindow;
pGhostWindowFromHungWindow _sws_GhostWindowFromHungWindow;
pForceFocusBasedMouseWheelRouting _sws_ForceFocusBasedMouseWheelRouting;
pSHWindowsPolicy sws_SHWindowsPolicy;
DEFINE_GUID(POLID_TurnOffSPIAnimations, 0xD7AF00A, 0xB468, 0x4A39, 0xB0, 0x16, 0x33, 0x3E, 0x22, 0x77, 0xAB, 0xED);

BOOL sws_WindowHelpers_IsValidMonitor(HMONITOR hMonitor, HDC unnamedParam2, LPRECT unnamedParam3, HMONITOR* pMonitor)
{
	if (!pMonitor || !*pMonitor) return FALSE;
	if (hMonitor == *pMonitor)
	{
		*pMonitor = NULL;
		return FALSE;
	}
	return TRUE;
}

BOOL _sws_TestExStyle(HWND hWnd, DWORD dwExStyle)
{
	return dwExStyle == (dwExStyle & (DWORD)GetWindowLongPtrW(hWnd, GWL_EXSTYLE));
}

BOOLEAN _sws_IsOwnerToolWindow(HWND hwnd)
{
	BOOLEAN bRet = FALSE;

	HWND hwndCurrent = hwnd;
	HWND hwndOwner = GetWindow(hwnd, GW_OWNER);
	while (!_sws_TestExStyle(hwndCurrent, WS_EX_APPWINDOW) && hwndOwner)
	{
		HWND hwndPrev = hwndCurrent;
		hwndCurrent = hwndOwner;
		hwndOwner = GetWindow(hwndOwner, GW_OWNER);
		if (_sws_TestExStyle(hwndCurrent, WS_EX_TOOLWINDOW))
		{
			bRet = !_sws_TestExStyle(hwndPrev, WS_EX_CONTROLPARENT) || hwndOwner != NULL;
			break;
		}
	}

	return bRet;
}

BOOL _sws_IsReallyVisible(HWND hWnd)
{
	RECT rc;
	GetWindowRect(hWnd, &rc);
	return IsWindowVisible(hWnd) && !IsRectEmpty(&rc);
}

BOOL _sws_IsGhosted(HWND hwnd)
{
	return _sws_GhostWindowFromHungWindow && _sws_GhostWindowFromHungWindow(hwnd) != NULL;
}

BOOL _sws_ShouldListWindowInAltTab(HWND hwnd)
{
	BOOL bRet = FALSE;

	if (IsWindow(hwnd) /*&& hwnd != _hwnd*/)
	{
		DWORD dwExStyle = (DWORD)GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
		HWND hwndOwner = GetWindow(hwnd, GW_OWNER);
		BOOLEAN bOwnerVisible = IsWindow(hwndOwner) && IsWindowEnabled(hwndOwner) && _sws_IsReallyVisible(hwndOwner);
		BOOLEAN bNoActivate = (dwExStyle & WS_EX_NOACTIVATE) != 0 || (dwExStyle & WS_EX_TOOLWINDOW) != 0;
		BOOLEAN bAppWindow = (dwExStyle & WS_EX_APPWINDOW) != 0;
		if (bAppWindow)
		{
			bNoActivate = FALSE;
		}
		bRet = _sws_IsReallyVisible(hwnd)
			// && IsWindowEnabled(hwnd)
			&& !bNoActivate
			&& (bAppWindow || !bOwnerVisible && !_sws_IsOwnerToolWindow(hwnd))
			&& !_sws_IsGhosted(hwnd);
	}

	return bRet;
}

BOOL _sws__IsTaskWindow(HWND hwnd)
{
	DWORD dwExStyle = (DWORD)GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
	return ((dwExStyle & WS_EX_APPWINDOW) != 0 || ((dwExStyle & WS_EX_TOOLWINDOW) == 0 && (dwExStyle & WS_EX_NOACTIVATE) == 0))
		&& IsWindowVisible(hwnd)
		&& !_sws_IsGhosted(hwnd);
}

BOOL _sws_IsTaskWindow(HWND hwnd, HWND* phwndTaskWindow)
{
	BOOL bRet = FALSE;

	if (_sws_ShouldListWindowInAltTab(hwnd))
	{
		HWND hwndTaskWindow = hwnd;
		HWND hwndCurrent = hwnd;
		while ((hwndCurrent = GetWindow(hwndCurrent, GW_OWNER)))
		{
			if (!_sws__IsTaskWindow(hwndCurrent))
				break;
			hwndTaskWindow = hwndCurrent;
		}
		*phwndTaskWindow = hwndTaskWindow;
		bRet = TRUE;
	}

	return bRet;
}

wchar_t* sws_WindowHelpers_GetAUMIDForHWND(HWND hWnd)
{
	WCHAR* pszAppId;
	if (SUCCEEDED(sws_AppResolver->lpVtbl->GetAppIDForWindow(sws_AppResolver, hWnd, &pszAppId, NULL, NULL, NULL)) && pszAppId) return pszAppId;
	return NULL;
}

BOOL sws_WindowHelpers_IsWindowShellManagedByExplorerPatcher(HWND hWnd)
{
	return GetPropW(hWnd, L"valinet.ExplorerPatcher.ShellManagedWindow") != 0;
}

BOOL sws_WindowHelpers_ShouldTreatShellManagedWindowAsNotShellManaged(HWND hWnd)
{
	return GetPropW(hWnd, L"Microsoft.Windows.ShellManagedWindowAsNormalWindow") != 0;
}

BOOL sws_WindowHelpers_IsAltTabWindow(HWND hWnd)
{
	// This identifies whether a window is a shell frame and includes those
	// A shell frame corresponds to, as far as I can tell, the frame of a UWP app
	// and we want those in the Alt-Tab list
	// Bugfix: Exclude hung shell frame (immersive) UWP windows, as we already include
	// ghost app windows in their place already
	if (sws_WindowHelpers_IsWindowUWP(hWnd) && (!_sws_GhostWindowFromHungWindow || !_sws_GhostWindowFromHungWindow(hWnd)))
	{
		return TRUE;
	}
	// Next, we need to check whether the window is shell managed and exclude it if so
	// Shell managed windows, as far as I can tell, represent all immersive UI the
	// Windows shell might present the user with, like: Start menu, Search (Win+Q),
	// notifications, taskbars etc
	if (_sws_IsShellManagedWindow && _sws_IsShellManagedWindow(hWnd) && !sws_WindowHelpers_ShouldTreatShellManagedWindowAsNotShellManaged(hWnd))
	{
		return FALSE;
	}
	// Also, exclude some windows created by ExplorerPatcher
	if (sws_WindowHelpers_IsWindowShellManagedByExplorerPatcher(hWnd))
	{
		return FALSE;
	}
	// Lastly, this check works with the remaining classic window and determines if it is a
	// "task window" and only includes it in Alt-Tab if so; this check is taken from
	// "AltTab.dll" in Windows 7 and this is how that OS decided to include a window in its
	// window switcher
	HWND hwndTaskWindow = NULL;
	return _sws_IsTaskWindow(hWnd, &hwndTaskWindow);
}

void sws_WindowHelpers_GetDesktopText(wchar_t* wszTitle)
{
	if (_sws_ExplorerFrame)
	{
		if (LoadStringW(_sws_ExplorerFrame, 13140, wszTitle, MAX_PATH))
		{
			// Strip CJK "(D)" from it
			wchar_t* pos = wcsstr(wszTitle, L"(&D)");
			if (pos != NULL)
			{
				*pos = L'\0';
			}
		}
		return;
	}
	wcscat_s(wszTitle, MAX_PATH, L"Desktop");
}

BOOL CALLBACK sws_WindowHelpers_AddAltTabWindowsToTimeStampedHWNDList(HWND hWnd, HDPA hdpa)
{
	if (!hdpa)
	{
		return FALSE;
	}
	if (sws_WindowHelpers_IsAltTabWindow(hWnd))
	{
		sws_tshwnd* tshWnd = malloc(sizeof(sws_tshwnd));
		if (tshWnd)
		{
			sws_tshwnd_Initialize(tshWnd, hWnd);
			sws_tshwnd_ModifyTimestamp(tshWnd, sws_WindowHelpers_GetStartTime());
			DPA_AppendPtr(hdpa, tshWnd);
		}
	}
	return TRUE;
}

BOOL sws_WindowHelpers_AreAnimationsAllowed()
{
	if (sws_SHWindowsPolicy && sws_SHWindowsPolicy(&POLID_TurnOffSPIAnimations))
	{
		return FALSE;
	}

	BOOL bAnimationsEnabled = FALSE;
	SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &bAnimationsEnabled, 0);
	return bAnimationsEnabled;
}

void sws_WindowHelpers_GetWindowText(HWND hWnd, LPWSTR lpWStr, DWORD dwLength)
{
	HWND hWndReal = hWnd;
	if (_sws_GhostWindowFromHungWindow)
	{
		HWND hWndGhost = _sws_GhostWindowFromHungWindow(hWnd);
		if (hWndGhost)
		{
			hWndReal = hWndGhost;
		}
	}
	InternalGetWindowText(hWndReal, lpWStr, dwLength);
}

void sws_WindowHelpers_GetDesiredWindowText(sws_WindowSwitcher* _this, sws_WindowSwitcherLayoutWindow* window, LPWSTR wszTitle)
{
	if (_this->layout.bIncludeWallpaper && window->hWnd == GetShellWindow())
	{
		sws_WindowHelpers_GetDesktopText(wszTitle);
	}
	else
	{
		WCHAR wszRundll32Path[MAX_PATH];
		GetSystemDirectoryW(wszRundll32Path, MAX_PATH);
		wcscat_s(wszRundll32Path, MAX_PATH, L"\\rundll32.exe");
		if (_this->settings.bAlwaysUseWindowTitleAndIcon || _this->mode != SWS_WINDOWSWITCHER_LAYOUTMODE_FULL || !_this->settings.bSwitcherIsPerApplication || !_wcsicmp(window->wszPath, wszRundll32Path))
		{
			sws_WindowHelpers_GetWindowText(window->hWnd, wszTitle, MAX_PATH);
		}
		else
		{
			if (window->dwCount > 1)
			{
				DWORD dwPrefixLen = 0;
				BOOL bAUMIDOk = FALSE;
				if (window->wszAUMID)
				{
					IShellItem2* pItem = NULL;
					if (SUCCEEDED(SHCreateItemInKnownFolder(&FOLDERID_AppsFolder, KF_FLAG_DONT_VERIFY, window->wszAUMID, &IID_IShellItem2, &pItem)) && pItem)
					{
						LPWSTR pDisplayName = NULL;
						if (SUCCEEDED(pItem->lpVtbl->GetDisplayName(pItem, SIGDN_NORMALDISPLAY, &pDisplayName)) && pDisplayName)
						{
							bAUMIDOk = TRUE;
							wcscpy_s(wszTitle + dwPrefixLen, MAX_PATH - dwPrefixLen, pDisplayName);
							CoTaskMemFree(pDisplayName);
						}
						pItem->lpVtbl->Release(pItem);
					}
				}
				if (!bAUMIDOk)
				{
					IShellItem2* pIShellItem2 = NULL;
					if (SUCCEEDED(SHCreateItemFromParsingName(window->wszPath, NULL, &IID_IShellItem2, &pIShellItem2)))
					{
						LPWSTR wszOutText = NULL;
						if (SUCCEEDED(pIShellItem2->lpVtbl->GetString(pIShellItem2, &PKEY_FileDescription, &wszOutText)))
						{
							int len = wcslen(wszOutText);
							if (len >= 4 && wszOutText[len - 1] == L'e' && wszOutText[len - 2] == L'x' && wszOutText[len - 3] == L'e' && wszOutText[len - 4] == L'.')
							{
								CoTaskMemFree(wszOutText);
								if (SUCCEEDED(pIShellItem2->lpVtbl->GetString(pIShellItem2, &PKEY_Software_ProductName, &wszOutText)))
								{
									wcscpy_s(wszTitle + dwPrefixLen, MAX_PATH - dwPrefixLen, wszOutText);
									CoTaskMemFree(wszOutText);
								}
								else
								{
									sws_WindowHelpers_GetWindowText(window->hWnd, wszTitle + dwPrefixLen, MAX_PATH - dwPrefixLen);
								}
							}
							else
							{
								wcscpy_s(wszTitle + dwPrefixLen, MAX_PATH - dwPrefixLen, wszOutText);
								CoTaskMemFree(wszOutText);
							}
						}
						else
						{
							sws_WindowHelpers_GetWindowText(window->hWnd, wszTitle + dwPrefixLen, MAX_PATH - dwPrefixLen);
						}
						pIShellItem2->lpVtbl->Release(pIShellItem2);
					}
				}

				WCHAR wszTitle2[MAX_PATH];
				wcscpy_s(wszTitle2, MAX_PATH, wszTitle);

				if (_sws_Explorer)
				{
					WCHAR wszFormat[MAX_PATH] = {0};
					if (LoadStringW(_sws_Explorer, 11115, wszFormat, MAX_PATH))
					{
						_snwprintf_s(wszTitle, MAX_PATH, _TRUNCATE, wszFormat, wszTitle2, window->dwCount);
						return;
					}
				}

				_snwprintf_s(wszTitle, MAX_PATH, _TRUNCATE, L"%s - %u running windows", wszTitle2, window->dwCount);
			}
			else
			{
				sws_WindowHelpers_GetWindowText(window->hWnd, wszTitle, MAX_PATH);
			}
		}
	}
}

HWND sws_WindowHelpers_GetLastActivePopup(HWND hWnd)
{
	HWND hOwner = GetWindow(hWnd, GW_OWNER);
	return GetLastActivePopup(hOwner ? hOwner : hWnd);
}

void sws_WindowHelpers_Clear()
{
	GdiplusShutdown(_sws_gdiplus_token);
	_sws_gdiplus_token = 0;
	if (sws_DefAppIcon)
	{
		DestroyIcon(sws_DefAppIcon);
		sws_DefAppIcon = NULL;
	}
	if (_sws_hUser32)
	{
		FreeLibrary(_sws_hUser32);
		_sws_hUser32 = NULL;
	}
	if (_sws_hShcore)
	{
		FreeLibrary(_sws_hShcore);
		_sws_hShcore = NULL;
	}
	if (sws_AppResolver)
	{
		sws_AppResolver->lpVtbl->Release(sws_AppResolver);
		sws_AppResolver = NULL;
	}
	if (_sws_ExplorerFrame)
	{
		FreeLibrary(_sws_ExplorerFrame);
		_sws_ExplorerFrame = NULL;
	}
	if (_sws_Explorer)
	{
		FreeLibrary(_sws_Explorer);
		_sws_Explorer = NULL;
	}
}

sws_error_t sws_WindowHelpers_Initialize()
{
	sws_error_t rv = SWS_ERROR_SUCCESS;

	if (_sws_gdiplus_token)
	{
		return rv;
	}
	GetSystemTimeAsFileTime(&sws_ancient_ft);
	GetSystemTimeAsFileTime(&sws_start_ft);
	if (!rv)
	{
		LoadIconWithScaleDown(
			(HINSTANCE)NULL,
			(PCWSTR)MAKEINTRESOURCEW(IDI_APPLICATION),
			(int)32,
			(int)32,
			(HICON*)(&(sws_DefAppIcon))
		);
		LoadIconWithScaleDown(
			(HINSTANCE)NULL,
			(PCWSTR)MAKEINTRESOURCEW(IDI_APPLICATION),
			(int)32,
			(int)32,
			(HICON*)(&(sws_LegacyDefAppIcon))
		);
		//sws_LegacyDefAppIcon = LoadIconW(NULL, MAKEINTRESOURCEW(IDI_APPLICATION));
	}
	if (!rv)
	{
		UINT32 gdiplusStartupInput[100];
		ZeroMemory(gdiplusStartupInput, 100);
		gdiplusStartupInput[0] = 1;
		rv = sws_error_Report(sws_error_GetFromGdiplusStatus(GdiplusStartup(&_sws_gdiplus_token, &gdiplusStartupInput)), NULL);
	}
	if (!rv)
	{
		if (!_sws_hUser32)
		{
			_sws_hUser32 = LoadLibraryExW(L"user32.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
		}
		if (_sws_hUser32)
		{
			if (!_sws_HungWindowFromGhostWindow)
			{
				_sws_HungWindowFromGhostWindow = (pHungWindowFromGhostWindow)GetProcAddress(_sws_hUser32, "HungWindowFromGhostWindow");
			}
			if (!_sws_GhostWindowFromHungWindow)
			{
				_sws_GhostWindowFromHungWindow = (pGhostWindowFromHungWindow)GetProcAddress(_sws_hUser32, "GhostWindowFromHungWindow");
			}
			if (!_sws_ForceFocusBasedMouseWheelRouting)
			{
				_sws_ForceFocusBasedMouseWheelRouting = (pForceFocusBasedMouseWheelRouting)GetProcAddress(_sws_hUser32, (LPCSTR)2575);
			}
			if (!_sws_IsShellManagedWindow)
			{
				_sws_IsShellManagedWindow = (pIsShellManagedWindow)GetProcAddress(_sws_hUser32, (LPCSTR)2574);
			}
			if (!sws_IsShellFrameWindow)
			{
				sws_IsShellFrameWindow = (pIsShellManagedWindow)GetProcAddress(_sws_hUser32, (LPCSTR)2573);
			}
			if (!_sws_CreateWindowInBand)
			{
				_sws_CreateWindowInBand = (pCreateWindowInBand)GetProcAddress(_sws_hUser32, "CreateWindowInBand");
			}

			if (!_sws_hShcore)
			{
				_sws_hShcore = LoadLibraryExW(L"shcore.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
			}
			if (_sws_hShcore && !sws_SHWindowsPolicy)
			{
				sws_SHWindowsPolicy = (pSHWindowsPolicy)GetProcAddress(_sws_hShcore, (LPCSTR)190);
			}

			if (!sws_AppResolver)
			{
				CoCreateInstance(&CLSID_StartMenuCacheAndAppResolver, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER, &IID_IAppResolver_8, (void**)&sws_AppResolver);
				if (!sws_AppResolver)
				{
					rv = sws_error_Report(sws_error_GetFromInternalError(SWS_ERROR_APPRESOLVER_NOT_AVAILABLE), NULL);
				}
			}
		}
	}

	if (!_sws_ExplorerFrame)
	{
		_sws_ExplorerFrame = LoadLibraryExW(L"ExplorerFrame.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32 | LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
	}
	if (!_sws_Explorer)
	{
		wchar_t explorerPath[MAX_PATH] = {0};
		GetWindowsDirectoryW(explorerPath, MAX_PATH);
		wcscat_s(explorerPath, MAX_PATH, L"\\explorer.exe");
		_sws_Explorer = LoadLibraryExW(explorerPath, NULL, LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE);
		printf("sws explorer %p", _sws_Explorer);
	}
	return rv;
}
