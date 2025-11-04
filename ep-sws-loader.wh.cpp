// ==WindhawkMod==
// @id              ep-sws-loader
// @name            EP SWS Loader
// @description     Load ExplorerPatcher SimpleWindowSwitcher
// @version         0.1
// @author          Ingan121
// @github          https://github.com/Ingan121
// @twitter         https://twitter.com/Ingan121
// @homepage        https://www.ingan121.com/
// @include         explorer.exe
// ==/WindhawkMod==

// ==WindhawkModReadme==
/*
# EP SWS loader
*/
// ==/WindhawkModReadme==

// ==WindhawkModSettings==
// ==/WindhawkModSettings==

#include <windhawk_utils.h>
#include <thread>

// The mod is being initialized, load settings, hook functions, and do other
// initialization stuff if required.
BOOL Wh_ModInit() {
#ifdef _WIN64
    Wh_Log(L"Init - x86_64");
#else
    Wh_Log(L"Init - x86");
#endif

    HMODULE hModule = LoadLibraryW(L"C:\\Users\\user\\source\\repos\\sws\\x64\\Release\\SimpleWindowSwitcher.dll");
    if (!hModule) {
        Wh_Log(L"SWS DLL NOT FOUND");
        return FALSE;
    }
    void* main = (void*)GetProcAddress(hModule, "main");
    if (!main) {
        Wh_Log(L"MAIN FUNCTION NOT FOUND");
        return false;
    }
    //((void(*)(DWORD))main)(0);
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)main, (LPVOID)0, 0, NULL);
    return TRUE;
}

// The mod is being unloaded, free all allocated resources.
void Wh_ModUninit() {
    Wh_Log(L"Uninit");
}
