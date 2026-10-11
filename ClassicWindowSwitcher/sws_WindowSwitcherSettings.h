#ifndef _H_SWS_WINDOWSWITCHERSETTINGS_H_
#define _H_SWS_WINDOWSWITCHERSETTINGS_H_

#include <Windows.h>

typedef struct _sws_WindowSwitcherSettings
{
    DWORD bIncludeWallpaper;
    DWORD bPerMonitor;
    DWORD bNoPerApplicationList;
    DWORD bSwitcherIsPerApplication;
    DWORD bAlwaysUseWindowTitleAndIcon;
    DWORD dwScrollWheelBehavior;
    DWORD bScrollWheelInvert;
    DWORD dwGridColumns;
    DWORD dwGridRows;
} sws_WindowSwitcherSettings;

#endif