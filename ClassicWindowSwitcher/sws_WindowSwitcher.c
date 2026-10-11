#include "sws_WindowSwitcher.h"

static void _sws_WindowSwitcher_UpdateAccessibleText(sws_WindowSwitcher* _this)
{
    sws_WindowSwitcherLayoutWindow* pWindowList = (sws_WindowSwitcherLayoutWindow*)_this->layout.pWindowList.pList;
    if (pWindowList)
    {
        if (!_this->layout.pWindowList.cbSize)
        {
            SetWindowTextW(_this->hWndAccessible, L"");
        }
        else
        {
            WCHAR wszAccText[MAX_PATH * 2], wszTitle[MAX_PATH];
            ZeroMemory(wszAccText, MAX_PATH * 2 * sizeof(WCHAR));
            ZeroMemory(wszTitle, MAX_PATH * sizeof(WCHAR));
            sws_WindowHelpers_GetDesiredWindowText(_this, &pWindowList[_this->layout.iIndex], wszTitle);
            swprintf_s(
                wszAccText,
                MAX_PATH * 2,
                L"%s: %d of %d",
                wszTitle,
                _this->layout.pWindowList.cbSize - _this->layout.iIndex,
                _this->layout.pWindowList.cbSize
            );
            //printf("[sws] Accesible text: %s.\n", wszAccText);
            SetWindowTextW(_this->hWndAccessible, wszAccText);
            NotifyWinEvent(
                EVENT_OBJECT_LIVEREGIONCHANGED,
                _this->hWndAccessible,
                OBJID_CLIENT,
                CHILDID_SELF
            );
        }
    }
}

static int _sws_WindowSwitcher_free_stub(void* p, void* pData)
{
    // This enables correct reporting of DPAs being freed in Debug builds
#if defined(DEBUG) | defined(_DEBUG)
    printf("[sws] tshwnd::free: destroy [[ %p ]]\n", p);
#endif
    free(p);
    return 1;
}

static HRESULT STDMETHODCALLTYPE _sws_WindowsSwitcher_IInputSwitchCallback_OnUpdateProfile(sws_IInputSwitchCallback* _this, IInputSwitchCallbackUpdateData* ud)
{
    // useful info: https://referencesource.microsoft.com/#system.windows.forms/winforms/Managed/System/WinForms/InputLanguage.cs,a01e59da9681988c
    uint16_t language = ud->dwID & 0xffff;
    uint16_t device = (ud->dwID >> 16) & 0x0fff;
    printf("OnUpdateProfile %d %d:", language, device);
    HWND hSwitcher = FindWindowW(SWS_WINDOWSWITCHER_CLASSNAME, NULL);
    if (hSwitcher)
    {
        PostMessageW(hSwitcher, WM_INPUTLANGCHANGE, 0, 0);
    }
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE _sws_WindowsSwitcher_IInputSwitchCallback_QueryInterface(sws_IInputSwitchCallback* _this, REFIID riid, void** ppvObject)
{
    if (!IsEqualIID(riid, &sws_IID_IInputSwitchCallback) && !IsEqualIID(riid, &IID_IUnknown))
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }
    *ppvObject = _this;
    return S_OK;
}

static ULONG STDMETHODCALLTYPE _sws_WindowsSwitcher_IInputSwitchCallback_AddRefRelease(sws_IInputSwitchCallback* _this)
{
    return 1;
}

static HRESULT STDMETHODCALLTYPE _sws_WindowsSwitcher_IInputSwitchCallback_Stub(sws_IInputSwitchCallback* _this)
{
    return S_OK;
}

static const sws_IInputSwitchCallbackVtbl _sws_WindowSwitcher_InputSwitchCallbackVtbl = {
    _sws_WindowsSwitcher_IInputSwitchCallback_QueryInterface,
    _sws_WindowsSwitcher_IInputSwitchCallback_AddRefRelease,
    _sws_WindowsSwitcher_IInputSwitchCallback_AddRefRelease,
    _sws_WindowsSwitcher_IInputSwitchCallback_OnUpdateProfile,
    _sws_WindowsSwitcher_IInputSwitchCallback_Stub,
    _sws_WindowsSwitcher_IInputSwitchCallback_Stub,
    _sws_WindowsSwitcher_IInputSwitchCallback_Stub,
    _sws_WindowsSwitcher_IInputSwitchCallback_Stub,
    _sws_WindowsSwitcher_IInputSwitchCallback_Stub,
    _sws_WindowsSwitcher_IInputSwitchCallback_Stub,
    _sws_WindowsSwitcher_IInputSwitchCallback_Stub
};

void _sws_WindowSwitcher_Wineventproc(
    HWINEVENTHOOK hWinEventHook,
    DWORD event,
    HWND hwnd,
    LONG idObject,
    LONG idChild,
    DWORD idEventThread,
    DWORD dwmsEventTime
)
{
    if ((event == EVENT_OBJECT_CREATE) && hwnd && idObject == OBJID_WINDOW && idChild == CHILDID_SELF && GetAncestor(hwnd, GA_PARENT) == GetDesktopWindow())
    {
        PostMessageW(FindWindowW(SWS_WINDOWSWITCHER_CLASSNAME, NULL), RegisterWindowMessageW(L"SHELLHOOK"), HSHELL_WINDOWCREATED, (LPARAM)hwnd);
    }
    else if ((event == EVENT_OBJECT_DESTROY) && hwnd && idObject == OBJID_WINDOW)
    {
        PostMessageW(FindWindowW(SWS_WINDOWSWITCHER_CLASSNAME, NULL), RegisterWindowMessageW(L"SHELLHOOK"), HSHELL_WINDOWDESTROYED, (LPARAM)hwnd);
    }
    else if ((event == EVENT_SYSTEM_FOREGROUND) && hwnd && (idObject == OBJID_WINDOW) && GetAncestor(hwnd, GA_PARENT) == GetDesktopWindow())
    {
        PostMessageW(FindWindowW(SWS_WINDOWSWITCHER_CLASSNAME, NULL), RegisterWindowMessageW(L"SHELLHOOK"), HSHELL_RUDEAPPACTIVATED, (LPARAM)hwnd);
    }
}

static void WINAPI _sws_WindowSwitcher_Calculate(sws_WindowSwitcher* _this, HWND* pOldHWNDs, DWORD cntOldHWNDs, DWORD dwOldIndex)
{
    while (TRUE)
    {
        long long start = sws_milliseconds_now();
        if (!_this->lastMiniModehWnd)
        {
            HWND hFw = GetForegroundWindow();
            HWND hOwner = GetWindow(hFw, GW_OWNER);
            _this->lastMiniModehWnd = (hOwner && IsWindowVisible(hOwner)) ? hOwner : hFw;
        }
        sws_WindowSwitcherLayout_Initialize(
            &(_this->layout),
            _this->hMonitor,
            _this->hWnd,
            _this->settings,
            &(_this->pHWNDList),
            (_this->mode ? _this->lastMiniModehWnd : NULL)
        );
        long long init = sws_milliseconds_now();
        sws_WindowSwitcherLayout_ComputeLayout(&(_this->layout), SWS_WINDOWSWITCHERLAYOUT_COMPUTE_DIRECTION_INITIAL, NULL, _this->settings.dwGridColumns, _this->settings.dwGridRows);
        long long fin = sws_milliseconds_now();
        printf("[sws] CalculateHelper %d [[ %lld + %lld = %lld ]].\n", _this->mode, init - start, fin - init, fin - start);

        sws_WindowSwitcherLayoutWindow* pWindowList = _this->layout.pWindowList.pList;
        int selectionIndex = -1;
        if (IsWindowVisible(_this->hWnd))
        {
            if (dwOldIndex >= 0 && dwOldIndex <= cntOldHWNDs - 1)
            {
                for (unsigned int i = 0; i < _this->layout.pWindowList.cbSize; ++i)
                {
                    if (pWindowList[i].hWnd == pOldHWNDs[dwOldIndex])
                    {
                        selectionIndex = i;
                        break;
                    }
                }
                if (selectionIndex < 0)
                {
                    BOOL bSuperBreak = FALSE;
                    for (int j = dwOldIndex - 1; j >= 0; j--)
                    {
                        for (unsigned int i = 0; i < _this->layout.pWindowList.cbSize; ++i)
                        {
                            if (pWindowList[i].hWnd == pOldHWNDs[j])
                            {
                                selectionIndex = i;
                                bSuperBreak = TRUE;
                                break;
                            }
                        }
                        if (bSuperBreak) break;
                    }
                }
                if (selectionIndex < 0)
                {
                    BOOL bSuperBreak = FALSE;
                    for (unsigned int i = 0; i < _this->layout.pWindowList.cbSize; ++i)
                    {
                        sws_window* pHWNDList = _this->pHWNDList.pList;
                        int k = -1;
                        if (pHWNDList)
                        {
                            for (unsigned int j = 0; j < _this->pHWNDList.cbSize; ++j)
                            {
                                if (pHWNDList[j].hWnd == pWindowList[i].hWnd)
                                {
                                    k = j;
                                    break;
                                }
                            }
                        }
                        if (pHWNDList && k >= 0)
                        {
                            for (sws_window* pcw = pHWNDList + k; pcw != NULL; pcw = pcw->pNextWindow)
                            {
                                if (pOldHWNDs[dwOldIndex] == pcw->hWnd)
                                {
                                    k = 0;
                                    for (unsigned int j = 0; j < _this->layout.pWindowList.cbSize; ++j)
                                    {
                                        if (pcw->hWnd == pWindowList[j].hWnd)
                                        {
                                            k = j;
                                            break;
                                        }
                                    }
                                    selectionIndex = i;
                                    bSuperBreak = TRUE;
                                    break;
                                }
                            }
                        }
                        if (bSuperBreak) break;
                    }
                }
            }
            if (selectionIndex != -1)
            {
                _this->layout.iIndex = selectionIndex;
                if (_this->layout.iIndex > _this->layout.pWindowList.cbSize - 1)
                {
                    _this->layout.iIndex = _this->layout.pWindowList.cbSize - 1;
                }
            }
        }
        if (selectionIndex == -1)
        {
            _this->layout.iIndex = _this->layout.pWindowList.cbSize == 1 ? 0 : _this->layout.iIndex - 1 - _this->layout.numTopMost;
        }
        if (_this->layout.iIndex < 0)
        {
            _this->layout.iIndex = 0;
        }
        if (_this->settings.bIncludeWallpaper && _this->layout.pWindowList.cbSize == 2 && IsIconic(pWindowList[1].hWnd))
        {
            _this->layout.iIndex = 1;
        }

        _this->cwIndex = -1;
        _this->cwMask = 0;
        _this->bPartialRedraw = FALSE;

        break;
    }
}

void _sws_WindowSwitcher_SwitchToSelectedItemAndDismiss(sws_WindowSwitcher* _this)
{
    sws_WindowSwitcherLayoutWindow* pWindowList = _this->layout.pWindowList.pList;
    if (_this->layout.bIncludeWallpaper && pWindowList[_this->layout.iIndex].hWnd == GetShellWindow())
    {
        _sws_WindowHelpers_ToggleDesktop();
    }
    else
    {
        sws_WindowSwitcherLayoutWindow* pWindowList = _this->layout.pWindowList.pList;
        if (pWindowList)
        {
            wchar_t tt[MAX_PATH];
            sws_WindowHelpers_GetWindowText(pWindowList[_this->layout.iIndex].hWnd, tt, MAX_PATH);
            wprintf(L"[sws] Chosen window: %s\n", tt);
            sws_WindowHelpers_GetWindowText(sws_WindowHelpers_GetLastActivePopup(pWindowList[_this->layout.iIndex].hWnd), tt, MAX_PATH);
            wprintf(L"[sws] Last active popup: %s\n", tt);
            GetClassNameW(GetWindow(pWindowList[_this->layout.iIndex].hWnd, GW_OWNER), tt, MAX_PATH);
            wprintf(L"[sws] Owner of window: %s\n", tt);
            HWND hLastActivePopup = sws_WindowHelpers_GetLastActivePopup(pWindowList[_this->layout.iIndex].hWnd);
            SwitchToThisWindow(IsWindowVisible(hLastActivePopup) ? hLastActivePopup : pWindowList[_this->layout.iIndex].hWnd, TRUE);
        }
    }
    ShowWindow(_this->hWnd, SW_HIDE);
}

static void _sws_WindowSwitcher_DrawContour(sws_WindowSwitcher* _this, HDC hdcPaint, RECT rc, int contour_size, RGBQUAD transparent)
{
    BYTE r = 0, g = 0, b = 0;
    COLORREF highlightColor = GetSysColor(COLOR_HIGHLIGHT);
    r = GetRValue(highlightColor);
    g = GetGValue(highlightColor);
    b = GetBValue(highlightColor);

    BITMAPINFO bi;
    ZeroMemory(&bi, sizeof(BITMAPINFO));
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = 1;
    bi.bmiHeader.biHeight = 1;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    RGBQUAD desiredColor = { b, g, r, 0xFF };

    int thickness = contour_size * (_this->layout.cbDpiX / DEFAULT_DPI_X);

    StretchDIBits(hdcPaint, rc.left, rc.top, thickness, rc.bottom - rc.top,
        0, 0, 1, 1, &desiredColor, &bi,
        DIB_RGB_COLORS, SRCCOPY);
    StretchDIBits(hdcPaint, rc.right - thickness, rc.top, thickness, rc.bottom - rc.top,
        0, 0, 1, 1, &desiredColor, &bi,
        DIB_RGB_COLORS, SRCCOPY);
    StretchDIBits(hdcPaint, rc.left, rc.top, rc.right - rc.left, thickness,
        0, 0, 1, 1, &desiredColor, &bi,
        DIB_RGB_COLORS, SRCCOPY);
    StretchDIBits(hdcPaint, rc.left, rc.bottom - thickness, rc.right - rc.left, thickness,
        0, 0, 1, 1, &desiredColor, &bi,
        DIB_RGB_COLORS, SRCCOPY);
}

sws_error_t sws_WindowSwitcher_RegisterHotkeys(sws_WindowSwitcher* _this)
{
    sws_error_t rv = SWS_ERROR_SUCCESS;

    if (!_this->settings.bNoPerApplicationList)
    {
        if (!rv)
        {
            if (!RegisterHotKey(_this->hWnd, -1, MOD_ALT, _this->vkTilde))
            {
                rv = sws_error_GetFromWin32Error(GetLastError());
            }
        }
        if (!rv)
        {
            if (!RegisterHotKey(_this->hWnd, -2, MOD_ALT | MOD_SHIFT, _this->vkTilde))
            {
                rv = sws_error_GetFromWin32Error(GetLastError());
            }
        }
        if (!rv)
        {
            if (!RegisterHotKey(_this->hWnd, -3, MOD_ALT | MOD_CONTROL, _this->vkTilde))
            {
                rv = sws_error_GetFromWin32Error(GetLastError());
            }
        }
        if (!rv)
        {
            if (!RegisterHotKey(_this->hWnd, -4, MOD_ALT | MOD_SHIFT | MOD_CONTROL, _this->vkTilde))
            {
                rv = sws_error_GetFromWin32Error(GetLastError());
            }
        }
    }
    if (!rv)
    {
        if (!RegisterHotKey(_this->hWnd, 1, MOD_ALT, VK_TAB))
        {
            rv = sws_error_GetFromWin32Error(GetLastError());
        }
    }
    if (!rv)
    {
        if (!RegisterHotKey(_this->hWnd, 2, MOD_ALT | MOD_SHIFT, VK_TAB))
        {
            rv = sws_error_GetFromWin32Error(GetLastError());
        }
    }
    if (!rv)
    {
        if (!RegisterHotKey(_this->hWnd, 3, MOD_ALT | MOD_CONTROL, VK_TAB))
        {
            rv = sws_error_GetFromWin32Error(GetLastError());
        }
    }
    if (!rv)
    {
        if (!RegisterHotKey(_this->hWnd, 4, MOD_ALT | MOD_SHIFT | MOD_CONTROL, VK_TAB))
        {
            rv = sws_error_GetFromWin32Error(GetLastError());
        }
    }
    //printf("[sws] Hotkey registration result: %d\n", rv);

    return rv;
}

void sws_WindowSwitcher_UnregisterHotkeys(sws_WindowSwitcher* _this, BOOL tildeOnly)
{
    if (!tildeOnly)
    {
        UnregisterHotKey(_this->hWnd, 1);
        UnregisterHotKey(_this->hWnd, 2);
        UnregisterHotKey(_this->hWnd, 3);
        UnregisterHotKey(_this->hWnd, 4);
    }
    UnregisterHotKey(_this->hWnd, -1);
    UnregisterHotKey(_this->hWnd, -2);
    UnregisterHotKey(_this->hWnd, -3);
    UnregisterHotKey(_this->hWnd, -4);
}

void sws_WindowSwitcher_Paint(sws_WindowSwitcher* _this, DWORD dwFlags)
{
    HWND hWnd = _this->hWnd;
    BOOL bIsWindowVisible = IsWindowVisible(_this->hWnd);

    PAINTSTRUCT ps;
    HDC hDC = BeginPaint(hWnd, &ps);

    sws_WindowSwitcherLayoutWindow* pWindowList = _this->layout.pWindowList.pList;

    RECT rc;
    GetClientRect(hWnd, &rc);
    POINT ptZero = { 0, 0 };
    SIZE siz = { rc.right - rc.left, rc.bottom - rc.top };
    BLENDFUNCTION bf = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };

    HDC hdcPaint = _this->hdcPaint;
    HFONT hOldFont = NULL;
    if (hdcPaint)
    {
        long long a0 = sws_milliseconds_now();

        hOldFont = SelectObject(hdcPaint, _this->layout.hFontRegular);

        BYTE r = 0, g = 0, b = 0, a = 255;
        COLORREF btnFace = GetSysColor(COLOR_BTNFACE);
        r = GetRValue(btnFace) * a / 255;
        g = GetGValue(btnFace) * a / 255;
        b = GetBValue(btnFace) * a / 255;
        RGBQUAD bkcol = { b, g, r, a };

        // Draw background
        if ((dwFlags & SWS_WINDOWSWITCHER_PAINTFLAGS_REDRAWENTIRE))
        {
            BITMAPINFO bi;
            ZeroMemory(&bi, sizeof(BITMAPINFO));
            bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bi.bmiHeader.biWidth = 1;
            bi.bmiHeader.biHeight = 1;
            bi.bmiHeader.biPlanes = 1;
            bi.bmiHeader.biBitCount = 32;
            bi.bmiHeader.biCompression = BI_RGB;
            StretchDIBits(hdcPaint, 0, 0, siz.cx, siz.cy, 0, 0, 1, 1, &bkcol, &bi, DIB_RGB_COLORS, SRCCOPY);
        }

        UINT col = _this->settings.dwGridColumns;
        UINT row = _this->settings.dwGridRows;

        int left = 11 * (_this->layout.cbDpiX / DEFAULT_DPI_X);
        int bottom = siz.cy - _this->layout.cbFontHeight;
        RECT rcTitleArea = { left, bottom - _this->layout.cbFontHeight, siz.cx - left, bottom };
        InflateRect(&rcTitleArea, 0, _this->layout.cbFontHeight / 2);
        DrawEdge(
            hdcPaint,
            &rcTitleArea,
            EDGE_SUNKEN,
            BF_RECT
        );

        SetTextColor(hdcPaint, GetSysColor(COLOR_BTNTEXT));
        SetBkMode(hdcPaint, TRANSPARENT);
        InflateRect(&rcTitleArea, -4, 0);

        void* pGdipGraphics = NULL;
        GdipCreateFromHDC(
            (HDC)hdcPaint,
            (void**)&pGdipGraphics
        );

        UINT gridX = -1;
        UINT gridY = 0;

        UINT selGridX = -1;
        UINT selGridY = -1;

        UINT i = _this->layout.iFirstItemIndex;

        BOOL recalcNeeded = FALSE;

        for (unsigned int j = 0; j < _this->layout.pWindowList.cbSize; ++j)
        {
            pWindowList[j].gridX = -1;
            pWindowList[j].gridY = -1;
        }
        while (TRUE)
        {
            if (i-- == 0)
            {
                if (_this->layout.pWindowList.cbSize <= col * row)
                {
                    break;
                }
                else
                {
                    i = _this->layout.pWindowList.cbSize - 1;
                }
            }
            gridX++;
            if (gridX > col - 1)
            {
                gridX = 0;
                gridY++;
            }
            pWindowList[i].gridX = gridX;
            pWindowList[i].gridY = gridY;
            if (i == _this->layout.iIndex)
            {
                if (_this->layout.pWindowList.cbSize > col)
                {
                    UINT lastColItemCnt = _this->layout.pWindowList.cbSize % col;
                    UINT lastRow = (_this->layout.pWindowList.cbSize / col) + (lastColItemCnt ? 1 : 0);
                    if (_this->lastKey == VK_UP)
                    {
                        selGridX = gridX;
                        selGridY = gridY - 1;
                        if (_this->layout.pWindowList.cbSize > col * row)
                        {
                            if (selGridY > row - 1)
                            {
                                selGridY = 0;
                                _this->layout.iFirstItemIndex += col;
                                if (_this->layout.iFirstItemIndex > _this->layout.pWindowList.cbSize - 1)
                                {
                                    _this->layout.iFirstItemIndex -= _this->layout.pWindowList.cbSize;
                                }
                                recalcNeeded = TRUE;
                            }
                        }
                        else if (selGridY == -1)
                        {
                            selGridY = lastRow - 1;
                            if (lastColItemCnt > 0 && selGridX >= lastColItemCnt)
                            {
                                selGridX = lastColItemCnt - 1;
                            }
                        }
                        if (selGridY < 0)
                        {
                            selGridY = 0;
                        }
                    }
                    else if (_this->lastKey == VK_DOWN)
                    {
                        selGridX = gridX;
                        selGridY = gridY + 1;
                        if (_this->layout.pWindowList.cbSize > col * row)
                        {
                            if (selGridY > row - 1)
                            {
                                selGridY = row - 1;
                                _this->layout.iFirstItemIndex -= col;
                                if (_this->layout.iFirstItemIndex < 0)
                                {
                                    _this->layout.iFirstItemIndex += _this->layout.pWindowList.cbSize;
                                }
                                recalcNeeded = TRUE;
                            }
                        }
                        else if (selGridY >= lastRow - 1)
                        {
                            if (selGridY > lastRow - 1)
                            {
                                selGridY = 0;
                            }
                            if (lastColItemCnt > 0 && selGridX >= lastColItemCnt)
                            {
                                selGridX = lastColItemCnt - 1;
                            }
                        }
                        //printf("asdf - selGridX >= lastColItemCnt: %d >= %d (%d) && selGridY == row - 1: %d == %d (%d)\n", selGridX, lastColItemCnt, selGridX >= lastColItemCnt, selGridY, row - 1, selGridY == row - 1);
                        if (selGridY < 0)
                        {
                            selGridY = 0;
                        }
                    }
                    else
                    {
                        selGridX = gridX;
                        selGridY = gridY;
                    }
                }
                else
                {
                    selGridX = gridX;
                    selGridY = gridY;
                }
                _this->lastKey = NULL;
            }
            if (gridY >= row)
            {
                break;
            }
        }

        if (recalcNeeded)
        {
            _this->cwMask = 0;
            _this->cwIndex = -1;

            gridX = -1;
            gridY = 0;
            for (unsigned int j = 0; j < _this->layout.pWindowList.cbSize; ++j)
            {
                pWindowList[j].gridX = -1;
                pWindowList[j].gridY = -1;
            }
            i = _this->layout.iFirstItemIndex;
            while (TRUE)
            {
                if (i-- == 0)
                {
                    if (_this->layout.pWindowList.cbSize <= col * row)
                    {
                        break;
                    }
                    else
                    {
                        i = _this->layout.pWindowList.cbSize - 1;
                    }
                }
                gridX++;
                if (gridX > col - 1)
                {
                    gridX = 0;
                    gridY++;
                }
                pWindowList[i].gridX = gridX;
                pWindowList[i].gridY = gridY;
                if (gridY >= row)
                {
                    break;
                }
            }
        }

        UINT duplicateCount = 0;

        for (unsigned int i = 0; i < _this->layout.pWindowList.cbSize; ++i)
        {
            UINT gridX = pWindowList[i].gridX;
            UINT gridY = pWindowList[i].gridY;
            if (gridY >= row || gridY == -1)
            {
                continue;
            }
            if (selGridX == gridX && selGridY == gridY)
            {
                _this->layout.iIndex = i;
            }
            if (i == _this->layout.iIndex && _this->layout.pWindowList.cbSize > col * row)
            {
                //printf("i nsgX nsgY gridX gridY: %d %d %d %d %d\n", i, selGridX, selGridY, gridX, gridY);
                if (_this->direction == SWS_WINDOWSWITCHERLAYOUT_COMPUTE_DIRECTION_FORWARD)
                {
                    if (gridX == col - 1 && gridY == row - 1)
                    {
                        _this->scrollDirection = SWS_WINDOWSWITCHERLAYOUT_COMPUTE_DIRECTION_FORWARD;
                    }
                    else
                    {
                        _this->scrollDirection = SWS_WINDOWSWITCHERLAYOUT_COMPUTE_DIRECTION_INITIAL;
                    }
                }
                else if (_this->direction == SWS_WINDOWSWITCHERLAYOUT_COMPUTE_DIRECTION_BACKWARD)
                {
                    if (gridX == 0 && gridY == 0)
                    {
                        _this->scrollDirection = SWS_WINDOWSWITCHERLAYOUT_COMPUTE_DIRECTION_BACKWARD;
                    }
                    else
                    {
                        _this->scrollDirection = SWS_WINDOWSWITCHERLAYOUT_COMPUTE_DIRECTION_INITIAL;
                    }
                }
            }

            RGBQUAD rgbStart = bkcol;

            COLORREF highlightColor = GetSysColor(COLOR_HIGHLIGHT);
            r = GetRValue(highlightColor);
            g = GetGValue(highlightColor);
            b = GetBValue(highlightColor);

            RGBQUAD rgbEnd = { b, g, r, 0xFF };
            RGBQUAD rgbFinal;


            sws_tshwnd* tshWnd = NULL;
            if (pWindowList)
            {
                if (_this->settings.bSwitcherIsPerApplication && _this->mode == SWS_WINDOWSWITCHER_LAYOUTMODE_FULL)
                {
                    tshWnd = pWindowList[i].last_flashing_tshwnd;
                    sws_window* pHWNDList = _this->pHWNDList.pList;
                    int k = -1;
                    if (pHWNDList)
                    {
                        for (unsigned int j = 0; j < _this->pHWNDList.cbSize; ++j)
                        {
                            if (pHWNDList[j].hWnd == pWindowList[i].hWnd)
                            {
                                k = j;
                                break;
                            }
                        }
                    }
                    if (pHWNDList && k >= 0)
                    {
                        for (sws_window* pcw = pHWNDList + k; pcw != NULL; pcw = pcw->pNextWindow)
                        {
                            if (pcw->tshWnd && sws_tshwnd_GetFlashState(pcw->tshWnd))
                            {
                                tshWnd = pcw->tshWnd;
                                pWindowList[i].last_flashing_tshwnd = tshWnd;
                                break;
                            }
                        }
                    }
                }
                else
                {
                    tshWnd = pWindowList[i].tshWnd;
                }
            }

            if (tshWnd)
            {
                if (sws_WindowHelpers_AreAnimationsAllowed())
                {
                    rgbFinal.rgbRed = sws_linear(sws_easing_easeOutQuad(tshWnd->cbFlashAnimationState), rgbStart.rgbRed, rgbEnd.rgbRed);
                    rgbFinal.rgbGreen = sws_linear(sws_easing_easeOutQuad(tshWnd->cbFlashAnimationState), rgbStart.rgbGreen, rgbEnd.rgbGreen);
                    rgbFinal.rgbBlue = sws_linear(sws_easing_easeOutQuad(tshWnd->cbFlashAnimationState), rgbStart.rgbBlue, rgbEnd.rgbBlue);
                    rgbFinal.rgbReserved = sws_linear(sws_easing_easeOutQuad(tshWnd->cbFlashAnimationState), rgbStart.rgbReserved, rgbEnd.rgbReserved);
                }
                else
                {
                    if (!sws_tshwnd_GetFlashState(tshWnd))
                    {
                        rgbFinal = rgbStart;
                    }
                    else
                    {
                        rgbFinal = rgbEnd;
                    }
                }
            }
            else
            {
                rgbFinal = rgbStart;
            }

            // Grid layout
            UINT leftStart = 11 * (_this->layout.cbDpiX / DEFAULT_DPI_X) + _this->layout.cbBorderSize;
            if (_this->layout.pWindowList.cbSize < col)
            {
                leftStart = (_this->layout.iWidth - _this->layout.pWindowList.cbSize * SWS_WINDOWSWITCHERLAYOUT_ITEMSIZE * (_this->layout.cbDpiX / DEFAULT_DPI_X)) / 2 - 2 * _this->layout.cbBorderSize;
            }
            pWindowList[i].rcWindow.left = leftStart + gridX * SWS_WINDOWSWITCHERLAYOUT_ITEMSIZE * (_this->layout.cbDpiX / DEFAULT_DPI_X);
            pWindowList[i].rcWindow.right = pWindowList[i].rcWindow.left + SWS_WINDOWSWITCHERLAYOUT_ITEMSIZE * (_this->layout.cbDpiX / DEFAULT_DPI_X);
            pWindowList[i].rcWindow.top = 16 * (_this->layout.cbDpiX / DEFAULT_DPI_X) + gridY * SWS_WINDOWSWITCHERLAYOUT_ITEMSIZE * (_this->layout.cbDpiX / DEFAULT_DPI_X);
            pWindowList[i].rcWindow.bottom = pWindowList[i].rcWindow.top + SWS_WINDOWSWITCHERLAYOUT_ITEMSIZE * (_this->layout.cbDpiX / DEFAULT_DPI_X);
            //printf("i gridX gridY x y: %d %d %d %d %d %d %d\n", i, gridX, gridY, pWindowList[i].rcWindow.left, pWindowList[i].rcWindow.top);

            // Draw flash rectangle
            BOOL bShouldDrawFlashRectangle = FALSE;
            if ((dwFlags & SWS_WINDOWSWITCHER_PAINTFLAGS_ISFLASHANIMATION) && tshWnd)
            {
                if (sws_tshwnd_GetFlashState(tshWnd))
                {
                    if (tshWnd->dwFlashAnimationState != SWS_WINDOWSWITCHER_ANIMATOR_FLASH_MAXSTATE)
                    {
                        if (!(tshWnd->dwFlashAnimationState % 2))
                        {
                            tshWnd->cbFlashAnimationState += SWS_WINDOWSWITCHER_ANIMATOR_FLASH_STEP;
                        }
                        else
                        {
                            tshWnd->cbFlashAnimationState -= SWS_WINDOWSWITCHER_ANIMATOR_FLASH_STEP;
                        }
                        if (tshWnd->cbFlashAnimationState <= 0.0)
                        {
                            tshWnd->cbFlashAnimationState = 0.0;
                        }
                        if (tshWnd->cbFlashAnimationState >= 1.0)
                        {
                            tshWnd->cbFlashAnimationState = 1.0;
                        }
                        bShouldDrawFlashRectangle = TRUE;
                    }

                    if (tshWnd->cbFlashAnimationState == 1.0 && tshWnd->dwFlashAnimationState >= SWS_WINDOWSWITCHER_ANIMATOR_FLASH_MAXSTATE)
                    {
                    }
                    else if (tshWnd->cbFlashAnimationState == 1.0 && tshWnd->dwFlashAnimationState == SWS_WINDOWSWITCHER_ANIMATOR_FLASH_MAXSTATE - 1)
                    {
                        tshWnd->dwFlashAnimationState = SWS_WINDOWSWITCHER_ANIMATOR_FLASH_MAXSTATE;
                    }
                    else if (tshWnd->cbFlashAnimationState == 1.0 && !(tshWnd->dwFlashAnimationState % 2))
                    {
                        tshWnd->dwFlashAnimationState = tshWnd->dwFlashAnimationState + 1;
                        tshWnd->cbFlashAnimationState -= SWS_WINDOWSWITCHER_ANIMATOR_FLASH_STEP;
                    }
                    else if (tshWnd->cbFlashAnimationState == 0.0 && (tshWnd->dwFlashAnimationState % 2))
                    {
                        tshWnd->dwFlashAnimationState = tshWnd->dwFlashAnimationState + 1;
                        tshWnd->cbFlashAnimationState += SWS_WINDOWSWITCHER_ANIMATOR_FLASH_STEP;
                    }
                }
                else
                {
                    tshWnd->dwFlashAnimationState = 0;
                    tshWnd->cbFlashAnimationState -= SWS_WINDOWSWITCHER_ANIMATOR_FLASH_STEP;
                    if (tshWnd->cbFlashAnimationState <= 0.0)
                    {
                        tshWnd->cbFlashAnimationState = 0.0;
                    }
                    else
                    {
                        bShouldDrawFlashRectangle = TRUE;
                    }
                }
            }
            if (
                pWindowList &&
                bIsWindowVisible &&
                ((dwFlags & SWS_WINDOWSWITCHER_PAINTFLAGS_REDRAWENTIRE) ||
                    ((dwFlags & SWS_WINDOWSWITCHER_PAINTFLAGS_ISFLASHANIMATION) && bShouldDrawFlashRectangle) ||
                    ((dwFlags & SWS_WINDOWSWITCHER_PAINTFLAGS_ACTIVEMASKORINDEXCHANGED) && (i == _this->cwIndex || i == _this->cwOldIndex))
                    ))
            {
                //printf("%d %d\n", dwFlags, i);

                RECT rc = pWindowList[i].rcWindow;
                rc.left += SWS_WINDOWSWITCHER_CONTOUR_SIZE;
                rc.top += SWS_WINDOWSWITCHER_CONTOUR_SIZE;
                rc.bottom -= SWS_WINDOWSWITCHER_CONTOUR_SIZE;
                rc.right -= SWS_WINDOWSWITCHER_CONTOUR_SIZE;

                BITMAPINFO bi;
                ZeroMemory(&bi, sizeof(BITMAPINFO));
                bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
                bi.bmiHeader.biWidth = 1;
                bi.bmiHeader.biHeight = 1;
                bi.bmiHeader.biPlanes = 1;
                bi.bmiHeader.biBitCount = 32;
                bi.bmiHeader.biCompression = BI_RGB;
                StretchDIBits(hdcPaint, rc.left + 1, rc.top + 1, rc.right - rc.left - 2, rc.bottom - rc.top - 2,
                    0, 0, 1, 1, &rgbFinal, &bi,
                    DIB_RGB_COLORS, SRCCOPY);
            }

            // Draw focus highlight rectangle
            if (pWindowList &&
                bIsWindowVisible &&
                gridX == selGridX &&
                gridY == selGridY &&
                ((dwFlags & SWS_WINDOWSWITCHER_PAINTFLAGS_REDRAWENTIRE) ||
                    ((dwFlags & SWS_WINDOWSWITCHER_PAINTFLAGS_ACTIVEMASKORINDEXCHANGED) && (i == _this->cwIndex || i == _this->cwOldIndex))
                    )
                )
            {
                duplicateCount++;
                _sws_WindowSwitcher_DrawContour(
                    _this,
                    hdcPaint,
                    pWindowList[_this->layout.iIndex].rcWindow,
                    SWS_WINDOWSWITCHER_CONTOUR_SIZE,
                    rgbFinal
                );
            }

            // Draw hover rectangle
            if (pWindowList &&
                bIsWindowVisible &&
                _this->cwIndex == i &&
                ((dwFlags & SWS_WINDOWSWITCHER_PAINTFLAGS_REDRAWENTIRE) ||
                    ((dwFlags & SWS_WINDOWSWITCHER_PAINTFLAGS_ISFLASHANIMATION) && bShouldDrawFlashRectangle) ||
                    ((dwFlags & SWS_WINDOWSWITCHER_PAINTFLAGS_ACTIVEMASKORINDEXCHANGED))
                    ) &&
                _this->cwIndex >= 0 &&
                _this->cwIndex < _this->layout.pWindowList.cbSize
                )
            {
                _sws_WindowSwitcher_DrawContour(
                    _this,
                    hdcPaint,
                    pWindowList[_this->cwIndex].rcWindow,
                    SWS_WINDOWSWITCHER_CONTOUR_SIZE,
                    rgbFinal
                );
            }

            // Draw title
            if ((pWindowList && _this->cwIndex == -1 &&
                gridX == selGridX && gridY == selGridY) ||
                (pWindowList && i == _this->cwIndex)
                )
            {
                WCHAR wszTitle[MAX_PATH];
                memset(wszTitle, 0, MAX_PATH * sizeof(wchar_t));
                if (_this->layout.bIncludeWallpaper && pWindowList[i].hWnd == GetShellWindow())
                {
                    sws_WindowHelpers_GetDesktopText(wszTitle);
                }
                else
                {
                    sws_WindowHelpers_GetWindowText(pWindowList[i].hWnd, wszTitle, MAX_PATH);
                }
                sws_WindowHelpers_GetDesiredWindowText(_this, &pWindowList[i], wszTitle);
                SetBkMode(hdcPaint, OPAQUE);
                SetBkColor(hdcPaint, GetSysColor(COLOR_BTNFACE));
                DrawTextW(
                    hdcPaint,
                    wszTitle,
                    -1,
                    &rcTitleArea,
                    DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_HIDEPREFIX
                );
            }

            // Draw icon
            if (pWindowList &&
                bIsWindowVisible &&
                ((dwFlags & SWS_WINDOWSWITCHER_PAINTFLAGS_REDRAWENTIRE) ||
                    ((dwFlags & SWS_WINDOWSWITCHER_PAINTFLAGS_ISFLASHANIMATION) && bShouldDrawFlashRectangle) ||
                    ((dwFlags & SWS_WINDOWSWITCHER_PAINTFLAGS_ACTIVEMASKORINDEXCHANGED) && (i == _this->cwIndex || i == _this->cwOldIndex))
                    ) &&
                pWindowList[i].hIcon &&
                pWindowList[i].iRowMax
                )
            {
                rc = pWindowList[i].rcWindow;
                INT x = rc.left + 5 * (_this->layout.cbDpiX / DEFAULT_DPI_X);
                INT y = rc.top + 5 * (_this->layout.cbDpiY / DEFAULT_DPI_Y);
                INT w = pWindowList[i].rcIcon.right;
                INT h = pWindowList[i].rcIcon.bottom;
                if (pWindowList[i].dwWindowFlags & SWS_WINDOWSWITCHERLAYOUT_WINDOWFLAGS_ISUWP)
                {
                    x = rc.left + 4 * (_this->layout.cbDpiX / DEFAULT_DPI_X);
                    y = rc.top + 3 * (_this->layout.cbDpiY / DEFAULT_DPI_Y);
                }
                //printf("i gridX gridY x y w h: %d %d %d %d %d %d %d\n", i, gridX, gridY, x, y, w, h);
                RGBQUAD bkcol2 = rgbFinal;
                // I don't understand why this is necessary, but otherwise icons
                // obtained from the file system have a black plate as background
                if (bkcol2.rgbReserved == 255) bkcol2.rgbReserved = 254;
                sws_IconPainter_DrawIcon(
                    pWindowList[i].hIcon,
                    hdcPaint,
                    (pWindowList[i].tshWnd && pWindowList[i].tshWnd->bFlash) ? _this->hFlashBrush : _this->hBackgroundBrush,
                    pGdipGraphics,
                    x, y, w, h,
                    bkcol2,
                    TRUE
                );
            }
        }
        if (duplicateCount > 1)
        {
            printf("[sws] DETECTED %d HIGHLIGHTED ITEMS\n", duplicateCount);
        }
        if (pGdipGraphics)
        {
            GdipDeleteGraphics((void*)pGdipGraphics);
        }

        BOOL bShouldDisableFlashAnimationTimer = TRUE;
        for (unsigned int i = 0; i < _this->layout.pWindowList.cbSize; ++i)
        {
            sws_tshwnd* tshWnd = ((_this->settings.bSwitcherIsPerApplication && _this->mode == SWS_WINDOWSWITCHER_LAYOUTMODE_FULL) ? pWindowList[i].last_flashing_tshwnd : pWindowList[i].tshWnd);

            if (pWindowList && tshWnd)
            {
                if (sws_tshwnd_GetFlashState(tshWnd))
                {
                    if (tshWnd->dwFlashAnimationState != SWS_WINDOWSWITCHER_ANIMATOR_FLASH_MAXSTATE)
                    {
                        bShouldDisableFlashAnimationTimer = FALSE;
                    }
                }
                else
                {
                    if (tshWnd->cbFlashAnimationState != 0.0)
                    {
                        bShouldDisableFlashAnimationTimer = FALSE;
                    }
                }
            }
        }
        if (bShouldDisableFlashAnimationTimer)
        {
            ResetEvent(_this->hFlashAnimationSignal);
        }

        if (bIsWindowVisible)
        {
            BitBlt(hDC, 0, 0, siz.cx, siz.cy, hdcPaint, 0, 0, SRCCOPY);
        }

        long long a1 = sws_milliseconds_now();
        //printf("[sws] WindowSwitcher::Paint [[ %lld ]]\n", a1 - _this->lastUpdateTime);
        _this->lastUpdateTime = a1;
    }

    EndPaint(hWnd, &ps);
}

static void WINAPI _sws_WindowSwitcher_Show(sws_WindowSwitcher* _this)
{
    if (!_this->dwInitFlags & SWS_WINDOWSWITCHER_INITFLAG_NO_CONF_RELOAD_ON_SHOW)
    {
        sws_WindowSwitcher_LoadSettings(_this);
    }
    long long a1 = sws_milliseconds_now();
    POINT pt;
    if (_this->bPrimaryOnly)
    {
        pt.x = 0;
        pt.y = 0;
    }
    else
    {
        GetCursorPos(&pt);
    }
    BOOL bIsMonitorValid = FALSE;
    if (_this->hMonitor && IsWindowVisible(_this->hWnd))
    {
        HMONITOR hSeekedMonitor = _this->hMonitor;
        EnumDisplayMonitors(NULL, NULL, sws_WindowHelpers_IsValidMonitor, &hSeekedMonitor);
        if (!hSeekedMonitor) bIsMonitorValid = TRUE;
    }
    if (!_this->hMonitor || !bIsMonitorValid) _this->hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);
    sws_WindowSwitcherLayoutWindow* pWindowList = _this->layout.pWindowList.pList;
    DWORD cntOldHWNDSs = _this->layout.pWindowList.cbSize;
    HWND* pOldHWNDs = NULL;
    DWORD dwOldIndex = _this->layout.iIndex;
    if (cntOldHWNDSs)
    {
        pOldHWNDs = calloc(cntOldHWNDSs, sizeof(HWND));
        if (pOldHWNDs)
        {
            for (unsigned int i = 0; i < _this->layout.pWindowList.cbSize; ++i)
            {
                pOldHWNDs[i] = pWindowList[i].hWnd;
            }
        }
    }
    sws_WindowSwitcherLayout_Clear(&(_this->layout));
    sws_window* pHWNDList = (sws_window*)_this->pHWNDList.pList;
    if (pHWNDList)
    {
        for (int i = 0; i < _this->pHWNDList.cbSize; ++i)
        {
            if (pHWNDList[i].wszAUMID)
            {
                CoTaskMemFree(pHWNDList[i].wszAUMID);
            }
        }
    }
    sws_vector_Clear(&(_this->pHWNDList));
    sws_vector_Initialize(&(_this->pHWNDList), sizeof(sws_window));
    HDPA hdpa = DPA_Create(SWS_VECTOR_CAPACITY);
    EnumWindows(sws_WindowHelpers_AddAltTabWindowsToTimeStampedHWNDList, hdpa);
    long long a2 = sws_milliseconds_now();
    for (unsigned int i = 0; i < DPA_GetPtrCount(hdpa); ++i)
    {
        sws_tshwnd* tshWnd = DPA_FastGetPtr(hdpa, i);
        int rv = DPA_Search(_this->htshwnds, tshWnd, 0, sws_tshwnd_CompareHWND, 0, 0);
        if (rv != -1)
        {
            sws_tshwnd* found_tshwnd = DPA_FastGetPtr(_this->htshwnds, rv);
            sws_tshwnd_ModifyTimestamp(tshWnd, found_tshwnd->ft);
        }
        else tshWnd->hWnd = INVALID_HANDLE_VALUE;
    }
    long long a3 = sws_milliseconds_now();
    DPA_Sort(hdpa, sws_tshwnd_CompareTimestamp, SWS_SORT_DESCENDING);
    long long a4 = sws_milliseconds_now();
    for (unsigned int i = 0; i < DPA_GetPtrCount(hdpa); ++i)
    {
        sws_tshwnd* tshWnd = DPA_FastGetPtr(hdpa, i);
        if (tshWnd->hWnd != INVALID_HANDLE_VALUE)
        {
            sws_window window;
            sws_window_Initialize(&window, tshWnd->hWnd);
            sws_vector_PushBack(&(_this->pHWNDList), &window);
        }
    }
    DPA_DestroyCallback(hdpa, _sws_WindowSwitcher_free_stub, 0);
    long long a5 = sws_milliseconds_now();
    printf("[sws] WindowSwitcher::Show %x [[ %lld + %lld + %lld + %lld = %lld ]]\n", GetShellWindow(), a2 - a1, a3 - a2, a4 - a3, a5 - a4, a5 - a1);
    _sws_WindowSwitcher_Calculate(_this, pOldHWNDs, cntOldHWNDSs, dwOldIndex);
    if (pOldHWNDs) free(pOldHWNDs);
    pWindowList = _this->layout.pWindowList.pList;
    if (_this->layout.pWindowList.cbSize == 0)
    {
        ShowWindow(_this->hWnd, SW_HIDE);
        return;
    }
    if (_this->layout.pWindowList.cbSize == 1 && _this->bSkipIfOneWindow && _this->mode == SWS_WINDOWSWITCHER_LAYOUTMODE_FULL)
    {
        ShowWindow(_this->hWnd, SW_HIDE);
        if (pWindowList[0].hWnd != GetShellWindow())
        {
            HWND hLastActivePopup = sws_WindowHelpers_GetLastActivePopup(pWindowList[_this->layout.iIndex].hWnd);
            SwitchToThisWindow(IsWindowVisible(hLastActivePopup) ? hLastActivePopup : pWindowList[_this->layout.iIndex].hWnd, TRUE);
        }
        return;
    }
    _this->layout.iFirstItemIndex = _this->layout.pWindowList.cbSize;
    int col = _this->settings.dwGridColumns;
    int row = _this->settings.dwGridRows;
    printf("[sws] cbSize=%d col=%d", _this->layout.pWindowList.cbSize, col);
    if (_this->initialDirection == SWS_WINDOWSWITCHERLAYOUT_COMPUTE_DIRECTION_BACKWARD)
    {
        _this->layout.iIndex = 0;
        if (_this->layout.pWindowList.cbSize > col * row)
        {
            _this->layout.iFirstItemIndex = col * (row / 2) + col / 2;
        }
    }
    _this->initialDirection = SWS_WINDOWSWITCHERLAYOUT_COMPUTE_DIRECTION_INITIAL;
    if (_this->hdcWindow)
    {
        EndBufferedPaint(_this->hBufferedPaint, FALSE);
        ReleaseDC(_this->hWnd, _this->hdcWindow);
        _this->hdcPaint = NULL;
    }
    _this->hdcWindow = GetDC(_this->hWnd);
    if (_this->hdcWindow)
    {
        BP_PAINTPARAMS params;
        ZeroMemory(&params, sizeof(BP_PAINTPARAMS));
        params.cbSize = sizeof(BP_PAINTPARAMS);
        params.dwFlags = BPPF_NOCLIP | BPPF_ERASE;
        RECT rc;
        SetRect(&rc, 0, 0, _this->layout.iWidth, _this->layout.iHeight);
        _this->hBufferedPaint = BeginBufferedPaint(_this->hdcWindow, &rc, BPBF_TOPDOWNDIB, &params, &(_this->hdcPaint));
    }
    sws_tshwnd* tshWnd = malloc(sizeof(sws_tshwnd));
    if (pWindowList && tshWnd)
    {
        for (int iCurrentWindow = _this->layout.pWindowList.cbSize - 1; iCurrentWindow >= 0; iCurrentWindow--)
        {
            sws_tshwnd_Initialize(tshWnd, pWindowList[iCurrentWindow].hWnd);
            int rv = DPA_Search(_this->htshwnds, tshWnd, 0, sws_tshwnd_CompareHWND, 0, 0);
            if (rv != -1)
            {
                pWindowList[iCurrentWindow].tshWnd = DPA_FastGetPtr(_this->htshwnds, rv);
            }
        }
    }
    if (tshWnd)
    {
        free(tshWnd);
    }
    sws_tshwnd* tshwnd2 = malloc(sizeof(sws_tshwnd));
    if (pHWNDList && tshwnd2)
    {
        for (unsigned int i = 0; i < _this->pHWNDList.cbSize; ++i)
        {
            sws_tshwnd_Initialize(tshwnd2, pHWNDList[i].hWnd);
            int rv = DPA_Search(_this->htshwnds, tshwnd2, 0, sws_tshwnd_CompareHWND, 0, 0);
            if (rv != -1)
            {
                pHWNDList[i].tshWnd = DPA_FastGetPtr(_this->htshwnds, rv);
            }
        }
    }
    if (tshwnd2)
    {
        free(tshwnd2);
    }
    if (!IsWindowVisible(_this->hWnd) && _this->dwShowDelay)
    {
        BOOL bCloak = TRUE;
        DwmSetWindowAttribute(_this->hWnd, DWMWA_CLOAK, &bCloak, sizeof(BOOL));
        SetEvent(_this->hShowSignal);
    }
    else
    {
        BOOL bCloak = FALSE;
        DwmSetWindowAttribute(_this->hWnd, DWMWA_CLOAK, &bCloak, sizeof(BOOL));
    }
    if (_this->bShouldStartFlashTimerWhenShowing)
    {
        _this->bShouldStartFlashTimerWhenShowing = FALSE;
        SetEvent(_this->hFlashAnimationSignal);
    }
    SetWindowPos(_this->hWnd, 0, _this->layout.iX, _this->layout.iY, _this->layout.iWidth, _this->layout.iHeight, SWP_NOZORDER);
    ShowWindow(_this->hWnd, SW_SHOW);
    SetForegroundWindow(_this->hWnd);
    _this->dwPaintFlags |= SWS_WINDOWSWITCHER_PAINTFLAGS_REDRAWENTIRE;
    InvalidateRect(_this->hWnd, NULL, TRUE);
    for (int iCurrentWindow = _this->layout.pWindowList.cbSize - 1; iCurrentWindow >= 0; iCurrentWindow--)
    {
        if (pWindowList[iCurrentWindow].hIcon == sws_DefAppIcon)
        {
            sws_IconPainter_CallbackParams* params = malloc(sizeof(sws_IconPainter_CallbackParams));
            if (params)
            {
                WCHAR wszRundll32Path[MAX_PATH];
                GetSystemDirectoryW(wszRundll32Path, MAX_PATH);
                wcscat_s(wszRundll32Path, MAX_PATH, L"\\rundll32.exe");
                params->bUseApplicationIcon = FALSE;
                if (!_this->settings.bAlwaysUseWindowTitleAndIcon &&
                    !(pWindowList[iCurrentWindow].wszPath && !_wcsicmp(pWindowList[iCurrentWindow].wszPath, wszRundll32Path)) &&
                    !(pWindowList[iCurrentWindow].dwWindowFlags & SWS_WINDOWSWITCHERLAYOUT_WINDOWFLAGS_ISUWP) &&
                    _this->settings.bSwitcherIsPerApplication &&
                    _this->mode == SWS_WINDOWSWITCHER_LAYOUTMODE_FULL &&
                    pWindowList[iCurrentWindow].dwCount > 1)
                {
                    params->bUseApplicationIcon = TRUE;
                }
                params->hWnd = _this->hWnd;
                params->index = iCurrentWindow;
                if (!_this->layout.timestamp)
                {
                    _this->layout.timestamp = sws_milliseconds_now();
                }
                params->timestamp = _this->layout.timestamp;
                params->bIsDesktop = (_this->layout.bIncludeWallpaper && pWindowList[iCurrentWindow].hWnd == GetShellWindow());
                if (!sws_IconPainter_ExtractAndDrawIconAsync(pWindowList[iCurrentWindow].hWnd, params))
                {
                    pWindowList[iCurrentWindow].hIcon = sws_LegacyDefAppIcon;
                    free(params);
                }
            }
        }
    }
    if (!_this->bWasControl)
    {
        SetTimer(_this->hWnd, SWS_WINDOWSWITCHER_TIMER_ASYNCKEYCHECK, SWS_WINDOWSWITCHER_TIMER_ASYNCKEYCHECK_DELAY, NULL);
    }
    _sws_WindowSwitcher_UpdateAccessibleText(_this);
}

static DWORD _sws_WindowSwitcher_EndTaskThreadProc(sws_WindowSwitcher_EndTaskThreadParams* params)
{
    SetThreadDesktop(params->hDesktop);
    if (IsWindow(params->hWnd) && IsHungAppWindow(params->hWnd))
    {
        EndTask(params->hWnd, FALSE, FALSE);
    }
    else
    {
        PostMessageW(params->hWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
    }
    free(params);
    return 0;
}

static LRESULT _sws_WindowsSwitcher_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    sws_WindowSwitcher* _this = NULL;
    if (uMsg == WM_CREATE)
    {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)(lParam);
        _this = (struct sws_WindowSwitcher*)(pCreate->lpCreateParams);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)_this);
    }
    else
    {
        LONG_PTR ptr = GetWindowLongPtr(hWnd, GWLP_USERDATA);
        _this = (struct sws_WindowSwitcher*)(ptr);
    }

    //printf("%d %d %d\n", uMsg, wParam, lParam);
    if (uMsg == WM_TIMER && wParam == SWS_WINDOWSWITCHER_TIMER_ASYNCKEYCHECK)
    {
        if (!_this->bWasControl && !(GetAsyncKeyState(VK_MENU) & 0x8000))
        {
            _sws_WindowSwitcher_SwitchToSelectedItemAndDismiss(_this);
            KillTimer(hWnd, SWS_WINDOWSWITCHER_TIMER_ASYNCKEYCHECK);
            return 0;
        }
    }
    else if (uMsg == WM_TIMER && wParam == SWS_WINDOWSWITCHER_TIMER_PAINT)
    {
        SendMessageW(_this->hWnd, SWS_WINDOWSWITCHER_PAINT_MSG, SWS_WINDOWSWITCHER_PAINTFLAGS_REDRAWENTIRE, 0);
        KillTimer(_this->hWnd, SWS_WINDOWSWITCHER_TIMER_PAINT);
    }
    else if (uMsg == SWS_WINDOWSWITCHER_PAINT_MSG)
    {
        _this->dwPaintFlags |= wParam;
        RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_INTERNALPAINT);
    }
    else if (uMsg == SWS_WINDOWSWITCHER_RELOAD_CONFIG_MSG)
    {
        sws_WindowSwitcher_LoadSettings(_this);
        sws_WindowSwitcher_UnregisterHotkeys(_this, TRUE);
        sws_WindowSwitcher_RegisterHotkeys(_this);
    }
    else if (uMsg == WM_ERASEBKGND)
    {
        return 0;
    }
    else if (_this && uMsg == _this->msgShellHook && lParam)
    {
        // Fallback for those who renamed InputSwitch.dll
        if (wParam == HSHELL_WINDOWACTIVATED || wParam == HSHELL_RUDEAPPACTIVATED)
        {
            SendMessageW(hWnd, WM_INPUTLANGCHANGE, 0, 0);
        }
        
        if (wParam == HSHELL_WINDOWCREATED || wParam == HSHELL_WINDOWACTIVATED || wParam == HSHELL_RUDEAPPACTIVATED || wParam == HSHELL_FLASH || wParam == HSHELL_REDRAW)
        {
            sws_tshwnd* tshWnd;
            for (unsigned int i = 0; i < 2; ++i)
            {
                HWND hWnd = lParam;
                if (!i)
                {
                    HWND hOwner = GetWindow(lParam, GW_OWNER);
                    if (hOwner)
                    {
                        hWnd = hOwner;
                    }
                    else
                    {
                        continue;
                    }
                }
                tshWnd = malloc(sizeof(sws_tshwnd));
                if (tshWnd)
                {
                    sws_tshwnd_Initialize(tshWnd, hWnd);
                    int rv = DPA_Search(_this->htshwnds, tshWnd, 0, sws_tshwnd_CompareHWND, 0, 0);
                    if (rv == -1)
                    {
                        // If this window is not in the window list and is not the foreground window, 
                        // make sure it will be last in the window list when the switcher will be presented
                        // https://github.com/valinet/ExplorerPatcher/issues/1084
                        if (hWnd != GetForegroundWindow())
                        {
                            sws_tshwnd_ModifyTimestamp(tshWnd, sws_WindowHelpers_GetAncientTime());
                        }
                        DPA_InsertPtr(_this->htshwnds, 0, tshWnd);
                        sws_WindowSwitcherLayoutWindow* pWindowList = _this->layout.pWindowList.pList;
                        if (pWindowList)
                        {
                            for (unsigned int i = 0; i < _this->layout.pWindowList.cbSize; ++i)
                            {
                                if (hWnd == pWindowList[i].hWnd)
                                {
                                    pWindowList[i].tshWnd = tshWnd;
                                }
                            }
                        }
                        sws_window* pHWNDList = _this->pHWNDList.pList;
                        if (pHWNDList)
                        {
                            for (unsigned int i = 0; i < _this->pHWNDList.cbSize; ++i)
                            {
                                if (hWnd == pHWNDList[i].hWnd)
                                {
                                    pHWNDList[i].tshWnd = tshWnd;
                                }
                            }
                        }
                    }
                    else
                    {
                        free(tshWnd);
                        // Update flash status and have the window pop at the front of the list only
                        // when the window is the foreground window; otherwise, the OS (probably) denied
                        // the foreground request from the app and the window might still be flashing
                        // and not actually in the foreground
                        // https://github.com/valinet/ExplorerPatcher/issues/1084
                        if ((wParam == HSHELL_WINDOWCREATED || wParam == HSHELL_WINDOWACTIVATED || wParam == HSHELL_RUDEAPPACTIVATED) && (hWnd == GetForegroundWindow() || sws_WindowHelpers_GetLastActivePopup(hWnd) == GetForegroundWindow()))
                        {
                            sws_tshwnd* found = DPA_FastGetPtr(_this->htshwnds, rv);
                            sws_tshwnd_UpdateTimestamp(found);
                            sws_tshwnd_SetFlashState(found, FALSE);
                            found->dwFlashAnimationState = 0;
                            found->cbFlashAnimationState = 0.0;
                        }
                    }
                }
            }
#if defined(DEBUG) | defined(_DEBUG)
            printf("[sws] tshwnd::insert: list count: %d\n", DPA_GetPtrCount(_this->htshwnds));
#endif
        }
        if (wParam == HSHELL_WINDOWDESTROYED)
        {
            sws_tshwnd* tshWnd = malloc(sizeof(sws_tshwnd));
            if (tshWnd)
            {
                sws_tshwnd_Initialize(tshWnd, lParam);
                int rv = DPA_Search(_this->htshwnds, tshWnd, 0, sws_tshwnd_CompareHWND, 0, 0);
                if (rv != -1)
                {
                    sws_WindowSwitcherLayoutWindow* pWindowList = _this->layout.pWindowList.pList;
                    if (pWindowList)
                    {
                        for (unsigned int i = 0; i < _this->layout.pWindowList.cbSize; ++i)
                        {
                            if (pWindowList[i].tshWnd == DPA_FastGetPtr(_this->htshwnds, rv))
                            {
                                pWindowList[i].tshWnd = NULL;
                            }
                            if (pWindowList[i].last_flashing_tshwnd == DPA_FastGetPtr(_this->htshwnds, rv))
                            {
                                pWindowList[i].last_flashing_tshwnd = NULL;
                            }
                        }
                    }
                    sws_window* pHWNDList = _this->pHWNDList.pList;
                    if (pHWNDList)
                    {
                        for (unsigned int i = 0; i < _this->pHWNDList.cbSize; ++i)
                        {
                            if (pHWNDList[i].tshWnd == DPA_FastGetPtr(_this->htshwnds, rv))
                            {
                                pHWNDList[i].tshWnd = NULL;
                            }
                        }
                    }

                    free(DPA_FastGetPtr(_this->htshwnds, rv));
                    DPA_DeletePtr(_this->htshwnds, rv);
                }
                free(tshWnd);
            }
            if (IsWindowVisible(_this->hWnd))
            {
                sws_window* pHWNDList = _this->pHWNDList.pList;
                int bContains = -1;
                for (int i = 0; i < _this->pHWNDList.cbSize; ++i)
                {
                    if (pHWNDList[i].hWnd == (HWND)lParam)
                    {
                        bContains = i;
                        break;
                    }
                }
                if (bContains != -1)
                {
                    _sws_WindowSwitcher_Show(_this);
                }
            }
#if defined(DEBUG) | defined(_DEBUG)
            printf("[sws] tshwnd::remove: list count: %d\n", DPA_GetPtrCount(_this->htshwnds));
#endif
        }
        if (wParam == HSHELL_FLASH)
        {
            sws_tshwnd* tshWnd = malloc(sizeof(sws_tshwnd));
            if (tshWnd)
            {
                sws_tshwnd_Initialize(tshWnd, lParam);
                int rv = DPA_Search(_this->htshwnds, tshWnd, 0, sws_tshwnd_CompareHWND, 0, 0);
                if (rv != -1)
                {
                    sws_tshwnd* found = DPA_FastGetPtr(_this->htshwnds, rv);
                    if (!sws_tshwnd_GetFlashState(found))
                    {
                        sws_tshwnd_SetFlashState(found, TRUE);
                        if (IsWindowVisible(_this->hWnd))
                        {
                            SetEvent(_this->hFlashAnimationSignal);
                        }
                        else
                        {
                            _this->bShouldStartFlashTimerWhenShowing = TRUE;
                        }
                    }
                }
                free(tshWnd);
            }
        }
        if (wParam == HSHELL_REDRAW)
        {
            for (unsigned int i = 0; i < 2; ++i)
            {
                HWND hWnd = lParam;
                if (!i)
                {
                    HWND hOwner = GetWindow(lParam, GW_OWNER);
                    if (hOwner)
                    {
                        hWnd = hOwner;
                    }
                    else
                    {
                        continue;
                    }
                }

                sws_tshwnd* tshWnd = malloc(sizeof(sws_tshwnd));
                if (tshWnd)
                {
                    sws_tshwnd_Initialize(tshWnd, hWnd);
                    int rv = DPA_Search(_this->htshwnds, tshWnd, 0, sws_tshwnd_CompareHWND, 0, 0);
                    if (rv != -1)
                    {
                        sws_tshwnd* found = DPA_FastGetPtr(_this->htshwnds, rv);
                        if (sws_tshwnd_GetFlashState(found))
                        {
                            sws_tshwnd_SetFlashState(found, FALSE);
                            if (IsWindowVisible(_this->hWnd))
                            {
                                SetEvent(_this->hFlashAnimationSignal);
                            }
                            else
                            {
                                _this->bShouldStartFlashTimerWhenShowing = TRUE;
                            }
                            found->dwFlashAnimationState = 0;
                            found->cbFlashAnimationState = 1.0;
                        }
                    }
                    free(tshWnd);
                }

                if (IsWindowVisible(_this->hWnd))
                {
                    sws_WindowSwitcherLayoutWindow* pWindowList = _this->layout.pWindowList.pList;
                    if (pWindowList)
                    {
                        for (unsigned int i = 0; i < _this->layout.pWindowList.cbSize; ++i)
                        {
                            if (hWnd == pWindowList[i].hWnd)
                            {
                                sws_IconPainter_CallbackParams* params = malloc(sizeof(sws_IconPainter_CallbackParams));
                                if (params)
                                {
                                    WCHAR wszRundll32Path[MAX_PATH];
                                    GetSystemDirectoryW(wszRundll32Path, MAX_PATH);
                                    wcscat_s(wszRundll32Path, MAX_PATH, L"\\rundll32.exe");
                                    params->bUseApplicationIcon = FALSE;
                                    if (!_this->settings.bAlwaysUseWindowTitleAndIcon &&
                                        !(pWindowList[i].wszPath && !_wcsicmp(pWindowList[i].wszPath, wszRundll32Path)) &&
                                        !(pWindowList[i].dwWindowFlags & SWS_WINDOWSWITCHERLAYOUT_WINDOWFLAGS_ISUWP) &&
                                        _this->settings.bSwitcherIsPerApplication &&
                                        _this->mode == SWS_WINDOWSWITCHER_LAYOUTMODE_FULL &&
                                        pWindowList[i].dwCount > 1)
                                    {
                                        params->bUseApplicationIcon = TRUE;
                                    }
                                    if (!params->bUseApplicationIcon)
                                    {
                                        params->hWnd = _this->hWnd;
                                        params->index = i;
                                        if (!_this->layout.timestamp)
                                        {
                                            _this->layout.timestamp = sws_milliseconds_now();
                                        }
                                        params->timestamp = _this->layout.timestamp;
                                        params->bIsDesktop = (_this->layout.bIncludeWallpaper && pWindowList[i].hWnd == GetShellWindow());
                                        if (!sws_IconPainter_ExtractAndDrawIconAsync(pWindowList[i].hWnd, params))
                                        {
                                            pWindowList[i].hIcon = sws_LegacyDefAppIcon;
                                            free(params);
                                            SendMessageW(_this->hWnd, SWS_WINDOWSWITCHER_PAINT_MSG, SWS_WINDOWSWITCHER_PAINTFLAGS_REDRAWENTIRE, 0);
                                        }
                                    }
                                    else
                                    {
                                        free(params);
                                        SendMessageW(_this->hWnd, SWS_WINDOWSWITCHER_PAINT_MSG, SWS_WINDOWSWITCHER_PAINTFLAGS_REDRAWENTIRE, 0);
                                    }
                                }
                            }
                        }
                    }
                }
            }
#if defined(DEBUG) | defined(_DEBUG)
            WCHAR wn[200];
            InternalGetWindowText(lParam, wn, 200);
            wprintf(L"[sws] Don't flash [[ %s ]]\n", wn);
#endif
        }

        if (wParam == HSHELL_WINDOWCREATED || wParam == HSHELL_WINDOWACTIVATED || wParam == HSHELL_RUDEAPPACTIVATED)
        {
            if (IsWindowVisible(_this->hWnd) && (HWND)lParam != _this->hWnd && sws_WindowHelpers_IsAltTabWindow((HWND)lParam))
            {
                HDPA hdpa = DPA_Create(SWS_VECTOR_CAPACITY);
                EnumWindows(sws_WindowHelpers_AddAltTabWindowsToTimeStampedHWNDList, hdpa);
                sws_WindowSwitcherLayoutWindow* pWindowList = _this->layout.pWindowList.pList;
                if (pWindowList)
                {
                    for (unsigned int i = 0; i < _this->layout.pWindowList.cbSize; ++i)
                    {
                        for (unsigned int j = 0; j < DPA_GetPtrCount(hdpa); ++j)
                        {
                            sws_tshwnd* tshWnd = DPA_FastGetPtr(hdpa, j);
                            if (tshWnd->hWnd == pWindowList[i].hWnd)
                            {
                                tshWnd->hWnd = NULL;
                            }
                        }
                    }
                    BOOL bShouldShow = FALSE;
                    for (unsigned j = 0; j < DPA_GetPtrCount(hdpa); ++j)
                    {
                        sws_tshwnd* tshWnd = DPA_FastGetPtr(hdpa, j);
                        if (tshWnd->hWnd)
                        {
                            bShouldShow = TRUE;
                        }
                        free(tshWnd);
                    }
                    if (bShouldShow)
                    {
                        _sws_WindowSwitcher_Show(_this);
                    }
                    DPA_Destroy(hdpa);
                }
            }
        }
    }
    else if (uMsg == WM_CLOSE)
    {
        DestroyWindow(hWnd);
        return 0;
    }
    else if (uMsg == WM_DESTROY)
    {
        PostQuitMessage(0);
        return 0;
    }
    else if (uMsg == WM_SHOWWINDOW)
    {
        if (wParam == FALSE)
        {
            KillTimer(hWnd, SWS_WINDOWSWITCHER_TIMER_ASYNCKEYCHECK);
            _this->lastMiniModehWnd = NULL;
            if (_sws_ForceFocusBasedMouseWheelRouting)
            {
                _sws_ForceFocusBasedMouseWheelRouting(FALSE);
            }
        }
        else
        {
            if (_sws_ForceFocusBasedMouseWheelRouting &&
                (_this->settings.dwScrollWheelBehavior == SWS_SCROLLWHEELBEHAVIOR_EVERYWHERE ||
                    _this->settings.dwScrollWheelBehavior == SWS_SCROLLWHEELBEHAVIOR_EVERYWHERE_IFCLIENTAREA_GRIDSCROLL ||
                    _this->settings.dwScrollWheelBehavior == SWS_SCROLLWHEELBEHAVIOR_EVERYWHERE_GRIDSCROLL ||
                    _this->settings.dwScrollWheelBehavior == SWS_SCROLLWHEELBEHAVIOR_EVERYWHERE_IFNOTCLIENTAREA_GRIDSCROLL)
                )
            {
                _sws_ForceFocusBasedMouseWheelRouting(TRUE);
            }
        }
        return 0;
    }
    else if (uMsg == WM_PAINT)
    {
        sws_WindowSwitcher_Paint(_this, _this->dwPaintFlags);
        _this->dwPaintFlags = SWS_WINDOWSWITCHER_PAINTFLAGS_NONE;
        return 0;
    }
    else if (uMsg == WM_MOUSEMOVE)
    {
        TRACKMOUSEEVENT tme;
        tme.cbSize = sizeof(TRACKMOUSEEVENT);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = hWnd;
        TrackMouseEvent(&tme);

        _this->bIsCursorOnSwitcher = TRUE;

        int x = GET_X_LPARAM(lParam);
        int y = GET_Y_LPARAM(lParam);
        sws_WindowSwitcherLayoutWindow* pWindowList = _this->layout.pWindowList.pList;
        if (pWindowList)
        {
            INT cwIndex = -1, cwMask = 0;
            for (unsigned int i = 0; i < _this->layout.pWindowList.cbSize; ++i)
            {
                RECT rc = pWindowList[i].rcWindow;
                if (x > rc.left && x < rc.right && y > rc.top && y < rc.bottom)
                {
                    cwMask |= SWS_WINDOWFLAG_IS_ON_WINDOW;
                    cwIndex = i;
                }
            }
            if (_this->cwMask != cwMask || _this->cwIndex != cwIndex)
            {
                _this->cwOldIndex = _this->cwIndex;
                _this->cwOldMask = _this->cwOldMask;
                _this->cwMask = cwMask;
                _this->cwIndex = cwIndex;
                _this->dwPaintFlags |= SWS_WINDOWSWITCHER_PAINTFLAGS_REDRAWENTIRE;
                RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_INTERNALPAINT);
            }
        }
        return 0;
    }
    else if (uMsg == WM_MOUSELEAVE)
    {
        _this->bIsCursorOnSwitcher = FALSE;
        if (_this->cwMask != 0)
        {
            _this->cwOldIndex = _this->cwIndex;
            _this->cwOldMask = _this->cwMask;
            _this->cwMask = 0;
            _this->cwIndex = -1;
            _this->dwPaintFlags |= SWS_WINDOWSWITCHER_PAINTFLAGS_REDRAWENTIRE;
            RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_INTERNALPAINT);
        }
    }
    else if (uMsg == WM_LBUTTONDOWN || uMsg == WM_MBUTTONDOWN)
    {
        _this->bIsMouseClicking = TRUE;
        return 0;
    }
    else if (uMsg == WM_LBUTTONUP || uMsg == WM_MBUTTONUP || ((uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN) && wParam == VK_DELETE))
    {
        if (uMsg == WM_LBUTTONUP || uMsg == WM_MBUTTONUP)
        {
            if (!_this->bIsMouseClicking) return 0;
            else _this->bIsMouseClicking = FALSE;
        }
        sws_WindowSwitcherLayoutWindow* pWindowList = _this->layout.pWindowList.pList;
        if (!pWindowList) return 0;
        unsigned int i = 0;
        BOOL bShouldClose = FALSE;
        if ((uMsg == WM_LBUTTONUP || uMsg == WM_MBUTTONUP) ?
            (
                i = _this->cwIndex,
                bShouldClose = uMsg == WM_MBUTTONUP,
                i >= 0 && i < _this->layout.pWindowList.cbSize &&
                GET_X_LPARAM(lParam) > pWindowList[i].rcWindow.left &&
                GET_X_LPARAM(lParam) < pWindowList[i].rcWindow.right &&
                GET_Y_LPARAM(lParam) > pWindowList[i].rcWindow.top &&
                GET_Y_LPARAM(lParam) < pWindowList[i].rcWindow.bottom
                ) : (
                    i = _this->layout.iIndex,
                    bShouldClose = TRUE,
                    i >= 0 && i < _this->layout.pWindowList.cbSize
                    ) &&
            (bShouldClose ? !(_this->layout.bIncludeWallpaper && pWindowList[i].hWnd == GetShellWindow()) : TRUE)
            )
        {
            if (!bShouldClose)
            {
                _this->layout.iIndex = i;
                _sws_WindowSwitcher_SwitchToSelectedItemAndDismiss(_this);
                return 0;
            }
            for (unsigned j = 0; j < DPA_GetPtrCount(pWindowList[i].dpaGroupedWnds/*_this->hLastClosedWnds*/); ++j)
            {
                HWND hWnd = DPA_FastGetPtr(pWindowList[i].dpaGroupedWnds/*_this->hLastClosedWnds*/, j);
                if (IsHungAppWindow(hWnd))
                {
                    ShowWindow(_this->hWnd, SW_HIDE);
                    sws_WindowSwitcher_EndTaskThreadParams* pEndTaskParams = malloc(sizeof(sws_WindowSwitcher_EndTaskThreadParams));
                    if (pEndTaskParams)
                    {
                        pEndTaskParams->hWnd = hWnd;
                        pEndTaskParams->sws = _this;
                        pEndTaskParams->hDesktop = GetThreadDesktop(GetCurrentThreadId());
                        if (!SHCreateThread(_sws_WindowSwitcher_EndTaskThreadProc, pEndTaskParams, CTF_NOADDREFLIB, NULL))
                        {
                            free(pEndTaskParams);
                            EndTask(hWnd, FALSE, FALSE);
                        }
                    }
                }
                else
                {
                    PostMessageW(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0);
                }
            }
        }
        return 0;
    }
    else if ((uMsg == WM_KEYUP && wParam == VK_ESCAPE) || uMsg == WM_KILLFOCUS || (uMsg == WM_ACTIVATE && wParam == WA_INACTIVE))
    {
        _this->bWasControl = FALSE;
        ShowWindow(_this->hWnd, SW_HIDE);
        return 0;
    }
    else if ((uMsg == WM_KEYUP && wParam == VK_MENU && !_this->bWasControl) ||
        ((uMsg == WM_KEYUP || uMsg == WM_SYSKEYUP) && wParam == VK_SPACE) ||
        ((uMsg == WM_KEYUP || uMsg == WM_SYSKEYUP) && wParam == VK_RETURN) ||
        (uMsg == WM_KEYUP && wParam == (_this->mode == SWS_WINDOWSWITCHER_LAYOUTMODE_MINI ? _this->vkTilde : VK_TAB) && !(GetKeyState(VK_MENU) & 0x8000) && !_this->bWasControl))
    {
        _sws_WindowSwitcher_SwitchToSelectedItemAndDismiss(_this);
        return 0;
    }
    else if (uMsg == WM_HOTKEY || uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN || (uMsg == WM_MOUSEWHEEL && _this && _this->settings.dwScrollWheelBehavior != SWS_SCROLLWHEELBEHAVIOR_DISABLED))
    {
        if (uMsg == WM_HOTKEY && (LOWORD(lParam) & MOD_CONTROL))
        {
            _this->bWasControl = TRUE;
        }
        if (((uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN) && wParam == _this->vkTilde && !_this->settings.bNoPerApplicationList) ||
            ((uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN) && wParam == VK_TAB) ||
            (uMsg == WM_HOTKEY && (LOWORD(lParam) & MOD_ALT)) ||
            ((uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN) && wParam == VK_LEFT) ||
            ((uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN) && wParam == VK_RIGHT) ||
            ((uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN) && wParam == VK_UP) ||
            ((uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN) && wParam == VK_DOWN) ||
            (uMsg == WM_MOUSEWHEEL)
            )
        {
            if (uMsg == WM_MOUSEWHEEL && !_this->bIsCursorOnSwitcher &&
                (_this->settings.dwScrollWheelBehavior == SWS_SCROLLWHEELBEHAVIOR_ONLYCLIENTAREA ||
                    _this->settings.dwScrollWheelBehavior == SWS_SCROLLWHEELBEHAVIOR_ONLYCLIENTAREA_GRIDSCROLL)
                )
            {
                return 0;
            }

            if (!IsWindowVisible(_this->hWnd))
            {
                if (uMsg == WM_HOTKEY && (int)wParam < 0)
                {
                    if (_this->settings.bNoPerApplicationList)
                    {
                        return 0;
                    }
                    _this->mode = SWS_WINDOWSWITCHER_LAYOUTMODE_MINI;
                }
                else
                {
                    _this->mode = SWS_WINDOWSWITCHER_LAYOUTMODE_FULL;
                }

                _this->initialDirection = SWS_WINDOWSWITCHERLAYOUT_COMPUTE_DIRECTION_INITIAL;
                if (uMsg == WM_HOTKEY && (LOWORD(lParam) & MOD_SHIFT))
                {
                    _this->initialDirection = SWS_WINDOWSWITCHERLAYOUT_COMPUTE_DIRECTION_BACKWARD;
                }

                _sws_WindowSwitcher_Show(_this);

                POINT ptCursor;
                GetCursorPos(&ptCursor);
                RECT rcSwitcher;
                GetWindowRect(_this->hWnd, &rcSwitcher);
                _this->bIsCursorOnSwitcher = PtInRect(&rcSwitcher, ptCursor);
                return 0;
            }
            else
            {
                _this->direction = SWS_WINDOWSWITCHERLAYOUT_COMPUTE_DIRECTION_INITIAL;
                _this->lastKey = (int)wParam;
                sws_WindowSwitcherLayoutWindow* pWindowList = _this->layout.pWindowList.pList;

                RECT rcPrev = pWindowList[_this->layout.iIndex].rcWindow;

                if ((((uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN) && wParam == _this->vkTilde) ||
                    (uMsg == WM_HOTKEY && ((int)wParam < 0))) &&
                    _this->mode == SWS_WINDOWSWITCHER_LAYOUTMODE_FULL &&
                    !_this->settings.bNoPerApplicationList &&
                    pWindowList[_this->layout.iIndex].hWnd != GetShellWindow())
                {
                    HWND hFw = pWindowList[_this->layout.iIndex].hWnd;
                    HWND hOwner = GetWindow(hFw, GW_OWNER);
                    _this->lastMiniModehWnd = (hOwner && IsWindowVisible(hOwner)) ? hOwner : hFw;
                    _this->mode = SWS_WINDOWSWITCHER_LAYOUTMODE_MINI;
                    _sws_WindowSwitcher_Show(_this);
                    return 0;
                }
                else if ((((uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN) && wParam == VK_TAB) ||
                    (uMsg == WM_HOTKEY && ((int)wParam > 0))) &&
                    _this->mode == SWS_WINDOWSWITCHER_LAYOUTMODE_MINI)
                {
                    _this->mode = SWS_WINDOWSWITCHER_LAYOUTMODE_FULL;
                    _sws_WindowSwitcher_Show(_this);
                    return 0;
                }

                UINT col = _this->settings.dwGridColumns;
                UINT row = _this->settings.dwGridRows;

                BOOL bIsGridScrolling = FALSE;
                if (uMsg == WM_MOUSEWHEEL && _this->layout.pWindowList.cbSize > col * row &&
                    (_this->settings.dwScrollWheelBehavior == SWS_SCROLLWHEELBEHAVIOR_ONLYCLIENTAREA_GRIDSCROLL ||
                        (_this->settings.dwScrollWheelBehavior == SWS_SCROLLWHEELBEHAVIOR_EVERYWHERE_IFCLIENTAREA_GRIDSCROLL && _this->bIsCursorOnSwitcher) ||
                        _this->settings.dwScrollWheelBehavior == SWS_SCROLLWHEELBEHAVIOR_EVERYWHERE_GRIDSCROLL ||
						(_this->settings.dwScrollWheelBehavior == SWS_SCROLLWHEELBEHAVIOR_EVERYWHERE_IFNOTCLIENTAREA_GRIDSCROLL && !_this->bIsCursorOnSwitcher))
                    )
                {
                    bIsGridScrolling = TRUE;
                }

                if (!bIsGridScrolling)
                {
                    if (
                        ((uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN) && (GetKeyState(VK_SHIFT) & 0x8000)) ||
                        (uMsg == WM_HOTKEY && (LOWORD(lParam) & MOD_SHIFT)) ||
                        ((uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN) && wParam == VK_LEFT) ||
                        ((uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN) && wParam == VK_UP) ||
                        (uMsg == WM_MOUSEWHEEL && (_this->settings.bScrollWheelInvert ? GET_WHEEL_DELTA_WPARAM(wParam) < 0 : GET_WHEEL_DELTA_WPARAM(wParam) > 0))
                        )
                    {
                        _this->direction = SWS_WINDOWSWITCHERLAYOUT_COMPUTE_DIRECTION_BACKWARD;

                        if (wParam != VK_UP || _this->layout.pWindowList.cbSize <= col)
                        {
                            if (_this->layout.iIndex == _this->layout.pWindowList.cbSize - 1)
                            {
                                _this->layout.iIndex = 0;
                            }
                            else
                            {
                                _this->layout.iIndex++;
                            }
                        }
                    }
                    else
                    {
                        _this->direction = SWS_WINDOWSWITCHERLAYOUT_COMPUTE_DIRECTION_FORWARD;

                        if (wParam != VK_DOWN || _this->layout.pWindowList.cbSize <= col)
                        {
                            if (_this->layout.iIndex == 0)
                            {
                                _this->layout.iIndex = _this->layout.pWindowList.cbSize - 1;
                            }
                            else
                            {
                                _this->layout.iIndex--;
                            }
                        }
                    }

                    if (_this->direction == SWS_WINDOWSWITCHERLAYOUT_COMPUTE_DIRECTION_FORWARD && _this->scrollDirection == _this->direction)
                    {
                        _this->layout.iFirstItemIndex -= col;
                        if (_this->layout.iFirstItemIndex < 0)
                        {
                            _this->layout.iFirstItemIndex += _this->layout.pWindowList.cbSize;
                        }
                        _this->cwMask = 0;
                        _this->cwIndex = -1;
                        printf("[sws] new first item index after scroll down: %d\n", _this->layout.iFirstItemIndex);
                    }
                    else if (_this->direction == SWS_WINDOWSWITCHERLAYOUT_COMPUTE_DIRECTION_BACKWARD && _this->scrollDirection == _this->direction)
                    {
                        _this->layout.iFirstItemIndex += col;
                        if (_this->layout.iFirstItemIndex >= (int)_this->layout.pWindowList.cbSize)
                        {
                            _this->layout.iFirstItemIndex -= _this->layout.pWindowList.cbSize;
                        }
                        _this->cwMask = 0;
                        _this->cwIndex = -1;
                        printf("[sws] new first item index after scroll up: %d\n", _this->layout.iFirstItemIndex);
                    }
                }
                else
                {
                    if (_this->settings.bScrollWheelInvert ? GET_WHEEL_DELTA_WPARAM(wParam) < 0 : GET_WHEEL_DELTA_WPARAM(wParam) > 0)
                    {
                        _this->lastKey = VK_UP;
                    }
                    else
                    {
                        _this->lastKey = VK_DOWN;
                    }
                }

                _this->dwPaintFlags |= SWS_WINDOWSWITCHER_PAINTFLAGS_REDRAWENTIRE;
                RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_INTERNALPAINT);
                SetForegroundWindow(_this->hWnd);
                _sws_WindowSwitcher_UpdateAccessibleText(_this);
            }
        }
        return 0;
    }
    else if (uMsg == WM_INPUTLANGCHANGE)
    {
        HKL hkl = (HKL)lParam;
        if (!hkl)
        {
            HWND foreground = GetForegroundWindow();
            if (foreground && foreground != _this->hWnd)
            {
                DWORD tid = GetWindowThreadProcessId(foreground, NULL);
                if (tid)
                {
                    hkl = GetKeyboardLayout(tid);
                }
            }
        }

        UINT vk;
        if (hkl)
        {
            vk = MapVirtualKeyExW(0x29, MAPVK_VSC_TO_VK_EX, hkl);
        }
        else
        {
            vk = MapVirtualKeyW(0x29, MAPVK_VSC_TO_VK_EX);
        }

        if (vk && vk != _this->vkTilde)
        {
            _this->vkTilde = vk;
            printf("Layout changed");
            sws_WindowSwitcher_UnregisterHotkeys(_this, TRUE);
            sws_WindowSwitcher_RegisterHotkeys(_this);
        }
        return 0;
    }

    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

static DWORD _sws_WindowSwitcher_FlashAnimationProcedure(sws_WindowSwitcher* _this)
{
    if (_this && _this->hFlashAnimationSignal)
    {
        while (WaitForSingleObject(_this->hFlashAnimationSignal, INFINITE) == WAIT_OBJECT_0)
        {
            if (!_this->hWnd)
            {
                break;
            }
            PostMessageW(_this->hWnd, SWS_WINDOWSWITCHER_PAINT_MSG, SWS_WINDOWSWITCHER_PAINTFLAGS_ISFLASHANIMATION, 0);
            sws_nanosleep((LONGLONG)SWS_WINDOWSWITCHER_ANIMATOR_FLASH_DELAY * (LONGLONG)10000);
        }
    }
    return 0;
}

static DWORD _sws_WindowSwitcher_ShowAsyncProcedure(sws_WindowSwitcher* _this)
{
    if (_this && _this->hShowSignal)
    {
        while (WaitForSingleObject(_this->hShowSignal, INFINITE) == WAIT_OBJECT_0)
        {
            if (!_this->hWnd)
            {
                break;
            }
            long long mulres = (LONGLONG)_this->dwShowDelay * (LONGLONG)10000;
            long long start = sws_milliseconds_now();
            sws_nanosleep(mulres);
            printf("[sws] Delayed showing by %lld ms due to: user configuration.\n", sws_milliseconds_now() - start);
            if (IsWindowVisible(_this->hWnd))
            {
                BOOL bCloak = FALSE;
                DwmSetWindowAttribute(_this->hWnd, DWMWA_CLOAK, &bCloak, sizeof(BOOL));
            }
        }
    }
    return 0;
}

static sws_error_t _sws_WindowSwitcher_RegisterWindowClass(sws_WindowSwitcher* _this)
{
    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = _sws_WindowsSwitcher_WndProc;
    wc.hbrBackground = _this->hBackgroundBrush;
    wc.hInstance = HINST_THISCOMPONENT;
    wc.lpszClassName = _T(SWS_WINDOWSWITCHER_CLASSNAME);
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    ATOM a = RegisterClassExW(&wc);
    if (!a)
    {
        return sws_error_Report(sws_error_GetFromWin32Error(GetLastError()), NULL);
    }
    return SWS_ERROR_SUCCESS;
}

__declspec(dllexport) sws_error_t sws_WindowSwitcher_RunMessageQueue(sws_WindowSwitcher* _this)
{
    if (!_this) return sws_error_GetFromInternalError(SWS_ERROR_INVALID_PARAMETER);

    MSG msg;
    BOOL bRet;

    while ((bRet = GetMessageW(&msg, NULL, 0, 0)) != 0)
    {
        if (bRet == -1)
        {
            return sws_error_GetFromWin32Error(GetLastError());
        }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return SWS_ERROR_SUCCESS;
}

void sws_WindowSwitcher_InitializeDefaultSettings(sws_WindowSwitcher* _this)
{
    _this->dwShowDelay = SWS_WINDOWSWITCHER_SHOWDELAY;
    _this->settings.bIncludeWallpaper = SWS_WINDOWSWITCHERLAYOUT_INCLUDE_WALLPAPER;
    _this->bPrimaryOnly = FALSE;
    _this->settings.bPerMonitor = FALSE;
    _this->settings.bNoPerApplicationList = FALSE;
    _this->settings.bSwitcherIsPerApplication = FALSE;
    _this->settings.bAlwaysUseWindowTitleAndIcon = FALSE;
    _this->settings.dwScrollWheelBehavior = SWS_SCROLLWHEELBEHAVIOR_DISABLED;
    _this->settings.bScrollWheelInvert = FALSE;
    _this->bSkipIfOneWindow = TRUE;
    _this->settings.dwGridColumns = SWS_WINDOWSWITCHERLAYOUT_DEFAULT_GRID_COLUMNS;
    _this->settings.dwGridRows = SWS_WINDOWSWITCHERLAYOUT_DEFAULT_GRID_ROWS;
}

__declspec(dllexport) void sws_WindowSwitcher_LoadSettings(sws_WindowSwitcher* _this)
{
    sws_WindowSwitcher_InitializeDefaultSettings(_this);
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, SWS_REGISTRY_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        DWORD dwType, dwData, dwSize = sizeof(dwData);
        if (RegQueryValueExW(hKey, L"ShowDelay", NULL, &dwType, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
        {
            if (dwData > 10000)
            {
                _this->dwShowDelay = 10000;
            }
            else
            {
                _this->dwShowDelay = dwData;
            }
        }
        if (RegQueryValueExW(hKey, L"IncludeWallpaper", NULL, &dwType, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
        {
            _this->settings.bIncludeWallpaper = dwData;
        }
        if (RegQueryValueExW(hKey, L"PrimaryMonitorOnly", NULL, &dwType, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
        {
            _this->bPrimaryOnly = dwData;
        }
        if (RegQueryValueExW(hKey, L"PerMonitor", NULL, &dwType, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
        {
            _this->settings.bPerMonitor = dwData;
        }
        if (RegQueryValueExW(hKey, L"NoPerApplicationList", NULL, &dwType, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
        {
            _this->settings.bNoPerApplicationList = dwData;
        }
        if (RegQueryValueExW(hKey, L"SwitcherIsPerApplication", NULL, &dwType, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
        {
            _this->settings.bSwitcherIsPerApplication = dwData;
        }
        if (RegQueryValueExW(hKey, L"AlwaysUseWindowTitleAndIcon", NULL, &dwType, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
        {
            _this->settings.bAlwaysUseWindowTitleAndIcon = dwData;
        }
        if (RegQueryValueExW(hKey, L"ScrollWheelBehavior", NULL, &dwType, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
        {
            if (dwData > SWS_SCROLLWHEELBEHAVIOR_EVERYWHERE_IFNOTCLIENTAREA_GRIDSCROLL)
            {
                _this->settings.dwScrollWheelBehavior = SWS_SCROLLWHEELBEHAVIOR_DISABLED;
            }
            else
            {
                _this->settings.dwScrollWheelBehavior = dwData;
            }
        }
        if (RegQueryValueExW(hKey, L"ScrollWheelInvert", NULL, &dwType, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
        {
            _this->settings.bScrollWheelInvert = dwData;
        }
        if (RegQueryValueExW(hKey, L"SkipIfOneWindow", NULL, &dwType, (LPBYTE)&dwData, &dwSize) == ERROR_SUCCESS)
        {
            _this->bSkipIfOneWindow = dwData;
        }
        RegCloseKey(hKey);
    }

    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Desktop", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        WCHAR wszValue[16];
        DWORD dwSize = sizeof(wszValue);
        if (RegQueryValueExW(hKey, L"CoolSwitchColumns", NULL, NULL, (LPBYTE)wszValue, &dwSize) == ERROR_SUCCESS)
        {
            _this->settings.dwGridColumns = (UINT)_wtoi(wszValue);
            if (_this->settings.dwGridColumns < 1)
            {
                _this->settings.dwGridColumns = SWS_WINDOWSWITCHERLAYOUT_DEFAULT_GRID_COLUMNS;
            }
        }
        dwSize = sizeof(wszValue);
        if (RegQueryValueExW(hKey, L"CoolSwitchRows", NULL, NULL, (LPBYTE)wszValue, &dwSize) == ERROR_SUCCESS)
        {
            _this->settings.dwGridRows = (UINT)_wtoi(wszValue);
            if (_this->settings.dwGridRows < 1)
            {
                _this->settings.dwGridRows = SWS_WINDOWSWITCHERLAYOUT_DEFAULT_GRID_ROWS;
            }
        }
        printf("[sws] Loaded grid layout: %d rows x %d columns\n", _this->settings.dwGridRows, _this->settings.dwGridColumns);
        RegCloseKey(hKey);
    }
}

__declspec(dllexport) void sws_WindowSwitcher_Clear(sws_WindowSwitcher* _this)
{
    if (_this)
    {
        if (_this->hWndAccessible)
        {
            if (_this->pAccPropServices != NULL)
            {
                MSAAPROPID props[] = { LiveSetting_Property_GUID };
                _this->pAccPropServices->lpVtbl->ClearHwndProps(
                    _this->pAccPropServices,
                    _this->hWndAccessible,
                    OBJID_CLIENT,
                    CHILDID_SELF,
                    props,
                    ARRAYSIZE(props));
                _this->pAccPropServices->lpVtbl->Release(_this->pAccPropServices);
                _this->pAccPropServices = NULL;
            }
            DestroyWindow(_this->hWndAccessible);
            _this->hWndAccessible = NULL;
        }
        if (_this->pInputSwitchControl)
        {
            _this->pInputSwitchControl->lpVtbl->Release(_this->pInputSwitchControl);
        }
        if (_this->hdcWindow)
        {
            EndBufferedPaint(_this->hBufferedPaint, FALSE);
            ReleaseDC(_this->hWnd, _this->hdcWindow);
            _this->hdcPaint = NULL;
        }
        sws_WindowSwitcherLayout_Clear(&(_this->layout));
        sws_window* pHWNDList = (sws_window*)_this->pHWNDList.pList;
        if (pHWNDList)
        {
            for (int i = 0; i < _this->pHWNDList.cbSize; ++i)
            {
                if (pHWNDList[i].wszAUMID)
                {
                    CoTaskMemFree(pHWNDList[i].wszAUMID);
                }
            }
        }
        sws_vector_Clear(&(_this->pHWNDList));
#if defined(DEBUG) | defined(_DEBUG)
        printf("[sws] tshwnd::destroy: list count: %d\n", DPA_GetPtrCount(_this->htshwnds));
#endif
        if (_this->htshwnds)
        {
            DPA_DestroyCallback(_this->htshwnds, _sws_WindowSwitcher_free_stub, 0);
        }
        sws_WindowSwitcher_UnregisterHotkeys(_this, FALSE);
        if (_this->hookForeground)
        {
            UnhookWinEvent(_this->hookForeground);
        }
        if (_this->hookCreateDestroy)
        {
            UnhookWinEvent(_this->hookCreateDestroy);
        }
        if (_this->hWnd)
        {
            DestroyWindow(_this->hWnd);
            _this->hWnd = NULL;
            BufferedPaintUnInit();
        }
        if (_this->hShowSignal)
        {
            if (_this->hShowThread)
            {
                SetEvent(_this->hShowSignal);
                WaitForSingleObject(_this->hShowThread, INFINITE);
                CloseHandle(_this->hShowThread);
            }
            CloseHandle(_this->hShowSignal);
        }
        if (_this->hFlashAnimationSignal)
        {
            if (_this->hFlashAnimationThread)
            {
                SetEvent(_this->hFlashAnimationSignal);
                WaitForSingleObject(_this->hFlashAnimationThread, INFINITE);
                CloseHandle(_this->hFlashAnimationThread);
            }
            CloseHandle(_this->hFlashAnimationSignal);
        }
        UnregisterClassW(SWS_WINDOWSWITCHER_CLASSNAME, HINST_THISCOMPONENT);
        sws_WindowHelpers_Clear();
        if (SUCCEEDED(_this->hrCo))
        {
            CoUninitialize();
        }
        if (_this->bIsDynamic)
        {
            free(_this);
        }
        else
        {
            memset(_this, 0, sizeof(sws_WindowSwitcher));
        }
#if defined(DEBUG) | defined(_DEBUG)
        _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
        _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
        _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);
        _CrtDumpMemoryLeaks();
#endif
    }
}

__declspec(dllexport) sws_error_t sws_WindowSwitcher_Initialize(sws_WindowSwitcher** __this, DWORD initFlags)
{
    sws_error_t rv = SWS_ERROR_SUCCESS;
    sws_WindowSwitcher* _this = NULL;
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);
    _CrtDumpMemoryLeaks();
#endif

    if (!rv)
    {
        if (!*__this)
        {
            *__this = calloc(1, sizeof(sws_WindowSwitcher));
            if (!*__this)
            {
                rv = sws_error_Report(sws_error_GetFromInternalError(SWS_ERROR_NO_MEMORY), NULL);
            }
            (*__this)->bIsDynamic = TRUE;
            (*__this)->dwInitFlags = initFlags;
        }
        else
        {
            (*__this)->bIsDynamic = FALSE;
            //memset((*__this), 0, sizeof(sws_WindowSwitcher));
        }
        _this = *__this;
    }
    if (!rv)
    {
        _this->hrCo = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        if (_this->hrCo != S_OK && _this->hrCo != S_FALSE)
        {
            rv = sws_error_Report(sws_error_GetFromHRESULT(_this->hrCo), NULL);
        }
    }
    if (!rv)
    {
        rv = sws_error_Report(sws_error_GetFromInternalError(sws_WindowHelpers_Initialize()), NULL);
    }
    if (!rv)
    {
        rv = sws_vector_Initialize(&(_this->pHWNDList), sizeof(sws_window));
    }
    if (!rv)
    {
        _this->hShowSignal = CreateEventW(NULL, FALSE, FALSE, NULL);
        if (!_this->hShowSignal)
        {
            rv = sws_error_Report(sws_error_GetFromWin32Error(GetLastError()), NULL);
        }
    }
    if (!rv)
    {
        _this->hShowThread = CreateThread(NULL, 0, _sws_WindowSwitcher_ShowAsyncProcedure, _this, 0, NULL);
        if (!_this->hShowThread)
        {
            rv = sws_error_Report(sws_error_GetFromWin32Error(GetLastError()), NULL);
        }
    }
    if (!rv)
    {
        _this->hFlashAnimationSignal = CreateEventW(NULL, TRUE, FALSE, NULL);
        if (!_this->hFlashAnimationSignal)
        {
            rv = sws_error_Report(sws_error_GetFromWin32Error(GetLastError()), NULL);
        }
    }
    if (!rv)
    {
        _this->hFlashAnimationThread = CreateThread(NULL, 0, _sws_WindowSwitcher_FlashAnimationProcedure, _this, 0, NULL);
        if (!_this->hFlashAnimationThread)
        {
            rv = sws_error_Report(sws_error_GetFromWin32Error(GetLastError()), NULL);
        }
    }
    if (!rv)
    {
        _this->htshwnds = DPA_Create(SWS_VECTOR_CAPACITY);
        if (!_this->htshwnds)
        {
            rv = sws_error_Report(sws_error_GetFromWin32Error(GetLastError()), NULL);
        }
        EnumWindows(sws_WindowHelpers_AddAltTabWindowsToTimeStampedHWNDList, _this->htshwnds);
    }
    if (!rv)
    {
        _this->hBackgroundBrush = (HBRUSH)CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
        _this->hFlashBrush = (HBRUSH)CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));
        _this->mode = SWS_WINDOWSWITCHER_LAYOUTMODE_FULL;
        _this->lastMiniModehWnd = NULL;
        _this->scrollDirection = SWS_WINDOWSWITCHERLAYOUT_COMPUTE_DIRECTION_INITIAL;
    }
    if (!rv)
    {
        _this->msgShellHook = RegisterWindowMessageW(L"SHELLHOOK");
        if (!_this->msgShellHook)
        {
            rv = sws_error_Report(sws_error_GetFromWin32Error(GetLastError()), NULL);
        }
    }
    if (!SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
    {
        SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);
    }
    if (!rv)
    {
        rv = sws_error_Report(sws_error_GetFromInternalError(_sws_WindowSwitcher_RegisterWindowClass(_this)), NULL);
    }
    if (!rv)
    {
        BufferedPaintInit();
        if (_sws_CreateWindowInBand)
        {
            _this->hWnd = _sws_CreateWindowInBand(
                WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
                _T(SWS_WINDOWSWITCHER_CLASSNAME),
                L"",
                WS_POPUP | WS_DLGFRAME,
                0, 0, 0, 0,
                NULL, NULL, HINST_THISCOMPONENT, _this,
                ZBID_UIACCESS // Only works in UIAccess process (or in privileged MSFT binary)
            );
        }
        if (!_this->hWnd)
        {
            // Try again without CWIB
            _this->hWnd = CreateWindowEx(
                WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
                _T(SWS_WINDOWSWITCHER_CLASSNAME),
                L"",
                WS_POPUP | WS_DLGFRAME,
                0, 0, 0, 0,
                NULL, NULL, HINST_THISCOMPONENT, _this
            );
        }
        if (!_this->hWnd)
        {
            rv = sws_error_Report(sws_error_GetFromWin32Error(GetLastError()), NULL);
        }
    }
    if (!rv)
    {
        _this->hookForeground = SetWinEventHook(
            EVENT_SYSTEM_FOREGROUND,
            EVENT_SYSTEM_FOREGROUND,
            NULL,
            _sws_WindowSwitcher_Wineventproc,
            0,
            0,
            WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS
        );
        if (!_this->hookForeground)
        {
            rv = sws_error_Report(HRESULT_FROM_WIN32(GetLastError()), NULL);
        }
    }
    if (!rv)
    {
        _this->hookCreateDestroy = SetWinEventHook(
            EVENT_OBJECT_CREATE,
            EVENT_OBJECT_DESTROY,
            NULL,
            _sws_WindowSwitcher_Wineventproc,
            0,
            0,
            WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS
        );
        if (!_this->hookCreateDestroy)
        {
            rv = sws_error_Report(HRESULT_FROM_WIN32(GetLastError()), NULL);
        }
    }
    if (!rv)
    {
        if (_this->bIsDynamic)
        {
            sws_WindowSwitcher_LoadSettings(_this);
        }
    }
    if (!rv)
    {
        _this->vkTilde = MapVirtualKeyW(0x29, MAPVK_VSC_TO_VK_EX);
        rv = sws_error_Report(sws_WindowSwitcher_RegisterHotkeys(_this), NULL);
    }
    if (!rv)
    {
        if (_this->hWnd && !RegisterShellHookWindow(_this->hWnd))
        {
            rv = sws_error_Report(sws_error_GetFromWin32Error(GetLastError()), NULL);
        }
    }
    if (!rv)
    {
        rv = sws_error_GetFromHRESULT(CoCreateInstance(&sws_CLSID_InputSwitchControl, NULL, CLSCTX_INPROC_SERVER, &sws_IID_InputSwitchControl, &(_this->pInputSwitchControl)));
        if (!rv)
        {
            rv = sws_error_GetFromHRESULT(_this->pInputSwitchControl->lpVtbl->Init(_this->pInputSwitchControl, 100));
        }
        if (!rv)
        {
            _this->InputSwitchCallback.lpVtbl = &_sws_WindowSwitcher_InputSwitchCallbackVtbl;
            rv = sws_error_GetFromHRESULT(_this->pInputSwitchControl->lpVtbl->SetCallback(_this->pInputSwitchControl, &(_this->InputSwitchCallback)));
        }
        if (rv)
        {
            // Make missing InputSwitch.dll not a critical error
            rv = 0;
        }
    }
    if (!rv)
    {
        _this->hWndAccessible = CreateWindowExW(
            0,
            L"Static",
            L"",
            WS_CHILD,
            0, 0, 0, 0,
            _this->hWnd,
            NULL,
            (HINSTANCE)GetWindowLongPtrW(_this->hWnd, GWLP_HINSTANCE),
            NULL
        );
        if (!_this->hWndAccessible)
        {
            rv = sws_error_Report(sws_error_GetFromWin32Error(GetLastError()), NULL);
        }
    }
    if (!rv)
    {
        rv = sws_error_GetFromHRESULT(CoCreateInstance(&CLSID_AccPropServices, NULL, CLSCTX_INPROC, &IID_IAccPropServices, &(_this->pAccPropServices)));
    }
    if (!rv)
    {
        VARIANT var;
        var.vt = VT_I4;
        var.lVal = 2; //Assertive;
        rv = sws_error_GetFromHRESULT(_this->pAccPropServices->lpVtbl->SetHwndProp(_this->pAccPropServices, _this->hWndAccessible, OBJID_CLIENT, CHILDID_SELF, LiveSetting_Property_GUID, var));
    }

    if (!rv)
    {
        _this->bIsInitialized = TRUE;
		wcscpy_s(_this->wszVersionString, 32, SWS_VER);
    }

    if (rv)
    {
        sws_WindowSwitcher_Clear(_this);
        *__this = NULL;
    }

    return rv;
}