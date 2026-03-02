// ==WindhawkMod==
// @id              cws-loader
// @name            ClassicWindowSwitcher Loader
// @description     Load ClassicWindowSwitcher
// @version         0.7
// @author          Ingan121 (patched)
// @github          https://github.com/Ingan121
// @twitter         https://twitter.com/Ingan121
// @homepage        https://www.ingan121.com/
// @include         explorer.exe
// @architecture    x86-64
// ==/WindhawkMod==

// ==WindhawkModSettings==
/*...*/
// ==/WindhawkModSettings==

#include <windows.h>
#include <string>
#include <windhawk_utils.h>

#define CWS_WINDOWSWITCHER_CLASSNAME L"ClassicWindowSwitcher_{BEA057BB-66C7-4758-A610-FAE6013E9F98}"

static HANDLE  g_hThread  = NULL;
static HMODULE g_hModule  = NULL;
static void*   g_unloadFn = nullptr;

static std::wstring g_lastDllPath;

using SwsMainFn   = DWORD(WINAPI*)(LPVOID);
using SwsUnloadFn = void(*)();

static void CleanupLoaded() {
    if (g_unloadFn) {
        reinterpret_cast<SwsUnloadFn>(g_unloadFn)();
        g_unloadFn = nullptr;
    } else {
        Wh_Log(L"sws_unload not found, skipping");
    }

    Sleep(200);

    if (g_hThread) {
        DWORD wait = WaitForSingleObject(g_hThread, 5000);
        if (wait == WAIT_TIMEOUT) {
            Wh_Log(L"Warning: sws_main thread did not exit within 5s.");
        }
        CloseHandle(g_hThread);
        g_hThread = NULL;
    }

    if (g_hModule) {
        FreeLibrary(g_hModule);
        g_hModule = NULL;
    }
}

static BOOL LoadFromPath(PCWSTR dllPath) {
    if (!dllPath || !*dllPath) {
        Wh_Log(L"Empty DLL path");
        return FALSE;
    }

    if (FindWindowW(CWS_WINDOWSWITCHER_CLASSNAME, NULL)) {
        Wh_Log(L"ClassicWindowSwitcher is already running!");
        return FALSE;
    }

    g_hModule = LoadLibraryW(dllPath);
    if (!g_hModule) {
        Wh_Log(L"Failed to load DLL: %ls. Error=%d", dllPath, (int)GetLastError());
        return FALSE;
    }

    auto mainFn = reinterpret_cast<SwsMainFn>(GetProcAddress(g_hModule, "sws_main"));
    if (!mainFn) {
        Wh_Log(L"sws_main function not found!");
        FreeLibrary(g_hModule);
        g_hModule = NULL;
        return FALSE;
    }

    g_unloadFn = reinterpret_cast<void*>(GetProcAddress(g_hModule, "sws_unload"));
    if (!g_unloadFn) {
        Wh_Log(L"sws_unload not found (will not be able to unload cleanly)");
    }

    g_hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)mainFn, (LPVOID)0, 0, NULL);
    if (!g_hThread) {
        Wh_Log(L"CreateThread failed! Error=%d", (int)GetLastError());
        g_unloadFn = nullptr;
        FreeLibrary(g_hModule);
        g_hModule = NULL;
        return FALSE;
    }

    return TRUE;
}

BOOL Wh_ModInit() {
#ifdef _WIN64
    Wh_Log(L"Init - x86_64");
#else
    Wh_Log(L"Init - x86");
#endif

    auto dllPathSetting = WindhawkUtils::StringSetting::make(L"dllpath");
    PCWSTR dllPath = dllPathSetting.get();

    BOOL ok = LoadFromPath(dllPath);
    if (ok && dllPath) {
        g_lastDllPath.assign(dllPath);
    }
    return ok;
}

BOOL Wh_ModSettingsChanged(BOOL* bReload) {
    *bReload = FALSE;

    auto dllPathSetting = WindhawkUtils::StringSetting::make(L"dllpath");
    PCWSTR newPath = dllPathSetting.get();
    std::wstring newPathStr = newPath ? newPath : L"";

    if (newPathStr.empty()) {
        Wh_Log(L"Empty new path, skipping reload");
        return TRUE;
    }

    if (newPathStr == g_lastDllPath) {
        return TRUE;
    }

    Wh_Log(L"dllpath changed. Reloading ClassicWindowSwitcher...");

    CleanupLoaded();

    if (LoadFromPath(newPathStr.c_str())) {
        g_lastDllPath = newPathStr;
    } else {
        Wh_Log(L"Reload failed. CWS is not running.");
        g_lastDllPath = newPathStr;
    }

    return TRUE;
}

void Wh_ModUninit() {
    Wh_Log(L"Uninit");
    CleanupLoaded();
    Wh_Log(L"Uninit complete");
}
