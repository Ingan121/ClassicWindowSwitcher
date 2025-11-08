// ==WindhawkMod==
// @id              cws-loader
// @name            ClassicWindowSwitcher Loader
// @description     Load ClassicWindowSwitcher
// @version         0.1
// @author          Ingan121
// @github          https://github.com/Ingan121
// @twitter         https://twitter.com/Ingan121
// @homepage        https://www.ingan121.com/
// @include         explorer.exe
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# ClassicWindowSwitcher Loader
* Build [ClassicWindowSwitcher](https://github.com/Ingan121/ClassicWindowSwitcher/), and put the DLL path in the mod settings.
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
/*
- dllpath: ClassicWindowSwitcher.dll
  $name: DLL Path
*/
// ==/WindhawkModSettings==

#include <windhawk_utils.h>

#define CWS_WINDOWSWITCHER_CLASSNAME L"ClassicWindowSwitcher_{BEA057BB-66C7-4758-A610-FAE6013E9F98}"

HANDLE g_hThread = NULL;
void* g_unloadFn = nullptr;

// The mod is being initialized, load settings, hook functions, and do other
// initialization stuff if required.
BOOL Wh_ModInit() {
#ifdef _WIN64
    Wh_Log(L"Init - x86_64");
#else
    Wh_Log(L"Init - x86");
#endif

    if (FindWindowW(CWS_WINDOWSWITCHER_CLASSNAME, NULL)) {
        Wh_Log(L"ClassicWindowSwitcher is already running!");
        return FALSE;
	}

	PCWSTR dllPath = Wh_GetStringSetting(L"dllpath");
    HMODULE hModule = LoadLibraryW(dllPath);
	Wh_FreeStringSetting(dllPath);
    if (!hModule) {
        Wh_Log(L"ClassicWindowSwitcher.dll not found!");
        return FALSE;
    }
    void* main = (void*)GetProcAddress(hModule, "sws_main");
    if (!main) {
        Wh_Log(L"sws_main function not found!");
        return false;
    }
    g_unloadFn = (void*)GetProcAddress(hModule, "sws_unload");
    g_hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)main, (LPVOID)0, 0, NULL);
    return TRUE;
}

BOOL Wh_ModSettingsChanged(BOOL* bReload) {
	*bReload = TRUE;
    return TRUE;
}

// The mod is being unloaded, free all allocated resources.
void Wh_ModUninit() {
    Wh_Log(L"Uninit");
    if (g_unloadFn) {
        ((void(*)())g_unloadFn)();
    }
    if (g_hThread) {
        WaitForSingleObject(g_hThread, INFINITE);
        CloseHandle(g_hThread);
        g_hThread = NULL;
    }
    FreeLibrary(GetModuleHandleW(L"ClassicWindowSwitcher.dll"));
    Wh_Log(L"Uninit complete");
}