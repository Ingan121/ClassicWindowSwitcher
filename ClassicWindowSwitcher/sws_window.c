#include "sws_window.h"

void sws_window_Clear(sws_window* _this)
{
	if (_this->hWnd)
	{
		_this->hWnd = NULL;
		_this->dwProcessId = 0;
		ZeroMemory(_this->wszPath, MAX_PATH);
		_this->bIsApplicationFrameHost = FALSE;
		_this->pNextWindow = NULL;
	}
	if (_this->wszAUMID)
	{
		CoTaskMemFree(_this->wszAUMID);
		_this->wszAUMID = NULL;
	}
}

sws_error_t sws_window_Initialize(sws_window* _this, HWND hWnd)
{
	sws_error_t rv = SWS_ERROR_SUCCESS;

	if (!rv)
	{
		ZeroMemory(_this->wszPath, MAX_PATH);
		_this->hWnd = hWnd;
	}
	if (!rv)
	{
		HWND hWndOfInterest = hWnd;
		if (_sws_HungWindowFromGhostWindow)
		{
			HWND hWndGhost = _sws_HungWindowFromGhostWindow(hWnd);
			if (hWndGhost)
			{
				hWndOfInterest = hWndGhost;
			}
		}
		if (!GetWindowThreadProcessId(hWndOfInterest, &(_this->dwProcessId)))
		{
			rv = HRESULT_FROM_WIN32(GetLastError());
		}
	}
	if (!rv)
	{
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, _this->dwProcessId);
		if (hProcess)
		{
			GetModuleFileNameExW(hProcess, NULL, _this->wszPath, MAX_PATH);
			CloseHandle(hProcess);
		}
	}
	if (!rv)
	{
		_this->bIsApplicationFrameHost = sws_WindowHelpers_IsWindowUWP(hWnd);
		_this->tshWnd = NULL;
		_this->pNextWindow = NULL;
	}
	if (!rv)
	{
		_this->wszAUMID = sws_WindowHelpers_GetAUMIDForHWND(_this->hWnd);
	}
	return rv;
}
