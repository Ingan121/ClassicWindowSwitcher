#include "sws_WindowSwitcherLayout.h"

static BOOL CALLBACK _sws_WindowSwitcherLayout_EnumWindowsCallback(_In_ HWND hWnd, _In_ sws_WindowSwitcherLayout* _this)
{
	if (sws_WindowHelpers_IsAltTabWindow(hWnd) || (_this->bIncludeWallpaper && !_this->bWallpaperAlwaysLast && hWnd == _this->hWndWallpaper))
	{
		sws_WindowSwitcherLayoutWindow swsLayoutWindow;
		sws_WindowSwitcherLayoutWindow_Initialize(&swsLayoutWindow, hWnd, NULL);
		sws_vector_PushBack(&_this->pWindowList, &swsLayoutWindow);
		DWORD band;
		_sws_GetWindowBand(hWnd, &band);
		if (band != ZBID_DESKTOP) _this->numTopMost++;
	}

	return TRUE;
}

sws_error_t sws_WindowSwitcherLayout_InvalidateLayout(sws_WindowSwitcherLayout* _this)
{
	sws_error_t rv = SWS_ERROR_SUCCESS;

	sws_WindowSwitcherLayoutWindow* pWindowList = _this->pWindowList.pList;
	for (int iCurrentWindow = _this->pWindowList.cbSize - 1; iCurrentWindow >= 0; iCurrentWindow--)
	{
		sws_WindowSwitcherLayoutWindow_Erase(&(pWindowList[iCurrentWindow]));
	}

	return rv;
}

sws_error_t sws_WindowSwitcherLayout_ComputeLayout(sws_WindowSwitcherLayout* _this, int direction, HWND hTarget, UINT col, UINT maxRow)
{
	sws_error_t rv = SWS_ERROR_SUCCESS;

	if (!rv)
	{
		int iObtainedIndex = 0;

		sws_WindowSwitcherLayoutWindow* pWindowList = _this->pWindowList.pList;

		BOOL bHasTarget = FALSE;
		int iTarget = -1;

		if (direction != SWS_WINDOWSWITCHERLAYOUT_COMPUTE_DIRECTION_INITIAL)
		{
			if (direction == SWS_WINDOWSWITCHERLAYOUT_COMPUTE_DIRECTION_BACKWARD)
			{
				bHasTarget = TRUE;
				iObtainedIndex = _this->pWindowList.cbSize - 1;
			}
			else if (direction == SWS_WINDOWSWITCHERLAYOUT_COMPUTE_DIRECTION_FORWARD)
			{
				if (_this->iIndex == _this->pWindowList.cbSize - 1)
				{
					iObtainedIndex = _this->iIndex;
				}
			}
			sws_WindowSwitcherLayout_InvalidateLayout(_this);
		}

		BOOL bFinishedLayout = FALSE;

		while (1)
		{
			int iCurrentCount = 0;

			for (int iCurrentWindow = iObtainedIndex ? iObtainedIndex : _this->iIndex; iCurrentWindow >= 0; iCurrentWindow--)
			{
				//TCHAR name[200];
				//GetWindowTextW(pWindowList[iCurrentWindow].hWnd, name, 200);
				//wprintf(L"%d %s ", pWindowList[iCurrentWindow].hWnd, name);

				if (pWindowList[iCurrentWindow].hWnd == _this->hWnd)
				{
					continue;
				}

				if (!bFinishedLayout)
				{
					pWindowList[iCurrentWindow].iRowMax = -1;
				}

				iTarget = iCurrentWindow;
				iCurrentCount++;
				if (iCurrentCount == _this->pWindowList.cbSize)
				{
					break;
				}
			}

			if (hTarget && direction == SWS_WINDOWSWITCHERLAYOUT_COMPUTE_DIRECTION_INITIAL)
			{
				BOOL bShouldBreak = FALSE;
				int iObtained = 0;
				int iTmpTop = pWindowList[iObtained].rcWindow.top;
				for (int j = iObtained; j >= 0; j--)
				{
					if (pWindowList[j].rcWindow.top != iTmpTop)
					{
						iObtained = j;
						break;
					}
				}
				sws_WindowSwitcherLayout_InvalidateLayout(_this);
				iObtainedIndex = iObtained;
				bFinishedLayout = FALSE;
				continue;
			}
			if (!bHasTarget)
			{
				break;
			}
		}

		UINT row = _this->pWindowList.cbSize / col;
		if (_this->pWindowList.cbSize % col)
		{
			row++;
		}
		if (row > maxRow)
		{
			row = maxRow;
		}

		if (!_this->iWidth)
		{
			_this->iWidth = col * SWS_WINDOWSWITCHERLAYOUT_ITEMSIZE * (_this->cbDpiX / DEFAULT_DPI_X) + 23 * (_this->cbDpiX / DEFAULT_DPI_X) + _this->cbBorderSize * 6;
			_this->iHeight = row * SWS_WINDOWSWITCHERLAYOUT_ITEMSIZE * (_this->cbDpiX / DEFAULT_DPI_X) + 32 * (_this->cbDpiX / DEFAULT_DPI_X) + _this->cbFontHeight * 5 / 2;
			_this->iX = ((_this->mi.rcWork.right - _this->mi.rcWork.left) - _this->iWidth) / 2 + _this->mi.rcWork.left;
			_this->iY = ((_this->mi.rcWork.bottom - _this->mi.rcWork.top) - _this->iHeight) / 2 + _this->mi.rcWork.top;
			//printf("height: %d, cbCurrentTop: %d, %f %f %f\n", _this->iHeight, cbCurrentTop, _this->cbThumbnailAvailableHeight, _this->cbBottomPadding, _this->cbPadding);
		}
	}

	if (!rv)
	{
		/*printf("\n");
		sws_WindowSwitcherLayoutWindow* pWindowList = _this->pWindowList.pList;
		for (UINT i = 0; i < _this->pWindowList.cbSize; ++i)
		{
			TCHAR name[200];
			GetWindowText(pWindowList[i].hWnd, name, 200);
			wprintf(L"%d %s\n", pWindowList[i].hWnd, name);
		}*/
	}

	return rv;
}

void sws_WindowSwitcherLayout_Clear(sws_WindowSwitcherLayout* _this)
{
	if (_this)
	{
		DeleteObject(_this->hFontRegular);
		sws_WindowSwitcherLayoutWindow* pWindowList = _this->pWindowList.pList;
		if (pWindowList)
		{
			for (int iCurrentWindow = 0; iCurrentWindow < _this->pWindowList.cbSize; ++iCurrentWindow)
			{
				sws_WindowSwitcherLayoutWindow_Clear(&(pWindowList[iCurrentWindow]));
			}
			sws_vector_Clear(&(_this->pWindowList));
		}
		memset(_this, 0, sizeof(sws_WindowSwitcherLayout));
	}
}

sws_error_t sws_WindowSwitcherLayout_Initialize(
	sws_WindowSwitcherLayout* _this, 
	HMONITOR hMonitor, 
	HWND hWnd, 
	DWORD* settings, 
	sws_vector* pHWNDList, 
	HWND hWndTarget,
	HWND hWndWallpaper
)
{
	sws_error_t rv = SWS_ERROR_SUCCESS;

	if (!rv)
	{
		if (!_this)
		{
			rv = sws_error_Report(sws_error_GetFromInternalError(SWS_ERROR_NO_MEMORY), NULL);
		}
		memset(_this, 0, sizeof(sws_WindowSwitcherLayout));
	}
	if (!rv)
	{
		rv = sws_WindowHelpers_Initialize();
	}
	if (!rv)
	{
		rv = sws_vector_Initialize(&(_this->pWindowList), sizeof(sws_WindowSwitcherLayoutWindow));
	}
	_this->mi.cbSize = sizeof(MONITORINFO);
	if (!rv)
	{
		if (!GetMonitorInfoW(
			hMonitor,
			&(_this->mi)
		))
		{
			rv = sws_error_Report(sws_error_GetFromWin32Error(GetLastError()), NULL);
		}
	}
	if (!rv)
	{
		_this->bWallpaperAlwaysLast = SWS_WINDOWSWITCHERLAYOUT_WALLPAPER_ALWAYS_LAST;
		_this->bIncludeWallpaper = SWS_WINDOWSWITCHERLAYOUT_INCLUDE_WALLPAPER;
		if (settings) _this->bIncludeWallpaper = settings[0] && settings[3];
		_this->bWallpaperToggleBehavior = SWS_WINDOWSWITCHERLAYOUT_WALLPAPER_TOGGLE;
		_this->hWndWallpaper = hWndWallpaper;
		if (_this->bIncludeWallpaper)
		{
			if (_this->bWallpaperAlwaysLast && !hWndTarget)
			{
				sws_WindowSwitcherLayoutWindow swsLayoutWindow;
				sws_WindowSwitcherLayoutWindow_Initialize(&swsLayoutWindow, _this->hWndWallpaper, NULL);
				sws_vector_PushBack(&_this->pWindowList, &swsLayoutWindow);
			}
		}
	}
	if (!rv)
	{
		if (pHWNDList)
		{
			wchar_t* targetAUMID = sws_WindowHelpers_GetAUMIDForHWND(hWndTarget);
			sws_window* windowList = pHWNDList->pList;
			sws_window* window = NULL;
			if (hWndTarget)
			{
				for (int i = 0; i < pHWNDList->cbSize; ++i)
				{
					if (windowList[i].hWnd == hWndTarget)
					{
						window = &(windowList[i]);
						break;
					}
				}
			}
			{
				WCHAR wszRundll32Path[MAX_PATH];
				GetSystemDirectoryW(wszRundll32Path, MAX_PATH);
				wcscat_s(wszRundll32Path, MAX_PATH, L"\\rundll32.exe");
				//HWND hWndForeground = GetForegroundWindow();
				for (int i = pHWNDList->cbSize - 1; i >= 0; i--)
				{
					BOOL isCloaked;
					DwmGetWindowAttribute(windowList[i].hWnd, DWMWA_CLOAKED, &isCloaked, sizeof(BOOL));
					if (isCloaked)
					{
						continue;
					}
					if (hWndTarget && hWndTarget != windowList[i].hWnd)
					{
						if (targetAUMID)
						{
							if (!(windowList[i].wszAUMID && !wcscmp(targetAUMID, windowList[i].wszAUMID))) continue;
						}
						else
						{
							if (!window)
							{
								continue;
							}
							else if (window->dwProcessId != windowList[i].dwProcessId && _wcsicmp(window->wszPath, windowList[i].wszPath))
							{
								continue;
							}
						}
					}
					if (!hWndTarget && settings[4] && _wcsicmp(windowList[i].wszPath, wszRundll32Path))
					{
						BOOL bShouldContinue = FALSE;
						for (int j = i - 1; j >= 0; j--)
						{
							if (sws_WindowHelpers_IsAltTabWindow(windowList[j].hWnd) && windowList[i].wszAUMID && windowList[j].wszAUMID)
							{
								if (!wcscmp(windowList[i].wszAUMID, windowList[j].wszAUMID) && (settings[1] ? MonitorFromWindow(windowList[i].hWnd, MONITOR_DEFAULTTONULL) == MonitorFromWindow(windowList[j].hWnd, MONITOR_DEFAULTTONULL) : TRUE))
								{
									windowList[j].pNextWindow = windowList + i;
									bShouldContinue = TRUE;
									break;
								}
							}
							else if (sws_WindowHelpers_IsAltTabWindow(windowList[j].hWnd) &&
								(windowList[i].dwProcessId == windowList[j].dwProcessId || !_wcsicmp(windowList[i].wszPath, windowList[j].wszPath)) &&
								(settings[1] ? MonitorFromWindow(windowList[i].hWnd, MONITOR_DEFAULTTONULL) == MonitorFromWindow(windowList[j].hWnd, MONITOR_DEFAULTTONULL) : TRUE))
							{
								bShouldContinue = TRUE;
								break;
							}
						}
						if (bShouldContinue)
						{
							continue;
						}
					}
					if (settings[1] && hMonitor != MonitorFromWindow(windowList[i].hWnd, MONITOR_DEFAULTTONULL))
					{
						continue;
					}
					sws_WindowSwitcherLayoutWindow swsLayoutWindow;
					sws_WindowSwitcherLayoutWindow_Initialize(&swsLayoutWindow, windowList[i].hWnd, windowList[i].wszPath);
					for (sws_window* pcw = windowList + i; pcw != NULL; pcw = pcw->pNextWindow) sws_WindowSwitcherLayoutWindow_AddGroupedWnd(&swsLayoutWindow, pcw->hWnd);
					sws_vector_PushBack(&_this->pWindowList, &swsLayoutWindow);
				}
			}
			if (targetAUMID) CoTaskMemFree(targetAUMID);
		}
	}
	_this->hWnd = hWnd;
	_this->hMonitor = hMonitor;
	_this->iIndex = _this->pWindowList.cbSize - 1;

	if (!rv)
	{
		HRESULT hr = GetDpiForMonitor(
			hMonitor,
			MDT_DEFAULT,
			&(_this->cbDpiX),
			&(_this->cbDpiY)
		);
		rv = sws_error_Report(sws_error_GetFromHRESULT(hr), NULL);
	}
	if (!rv)
	{
		sws_WindowSwitcherLayoutWindow* pWindowList = _this->pWindowList.pList;
		for (int iCurrentWindow = _this->pWindowList.cbSize - 1; iCurrentWindow >= 0; iCurrentWindow--)
		{
			if (!pWindowList[iCurrentWindow].hIcon)
			{
				double factor = SWS_UWP_ICON_SCALE_FACTOR;
				pWindowList[iCurrentWindow].rcIcon.left = 0;
				pWindowList[iCurrentWindow].rcIcon.top = 0;
				pWindowList[iCurrentWindow].rcIcon.right = SWS_WINDOWSWITCHERLAYOUT_ICONSIZE * (_this->cbDpiX / DEFAULT_DPI_X);
				pWindowList[iCurrentWindow].rcIcon.bottom = pWindowList[iCurrentWindow].rcIcon.right;
				pWindowList[iCurrentWindow].szIcon = pWindowList[iCurrentWindow].rcIcon.right;
				pWindowList[iCurrentWindow].hIcon = sws_DefAppIcon;
			}
		}
	}
	if (!rv)
	{
		NONCLIENTMETRICS ncm;
		ncm.cbSize = sizeof(NONCLIENTMETRICS);
		SystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0, _this->cbDpiX);

		_this->cbBorderSize = ncm.iBorderWidth;

		_this->hFontRegular = CreateFontIndirectW(&ncm.lfCaptionFont);
		if (!_this->hFontRegular)
		{
			rv = sws_error_Report(sws_error_GetFromWin32Error(GetLastError()), NULL);
		}
		else
		{
			HDC hdc = GetDC(_this->hWnd);
			HGDIOBJ hOldFont = SelectObject(hdc, _this->hFontRegular);

			TEXTMETRICW tm;
			GetTextMetricsW(hdc, &tm);

			SelectObject(hdc, hOldFont);
			ReleaseDC(_this->hWnd, hdc);

			_this->cbFontHeight = tm.tmHeight;
			//printf("font width: %d, height: %d\n", tm.tmAveCharWidth, tm.tmHeight);
		}
	}

	return rv;
}
