#include <stdio.h>
#include <Windows.h>

#pragma comment(linker,"\"/manifestdependency:type='win32' \
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' \
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

int main(int argc, char** argv)
{
	HMODULE hModule = LoadLibraryW(L"ClassicWindowSwitcher.dll");
	if (!hModule)
	{
		printf("Failed to load ClassicWindowSwitcher.dll\n");
		return 1;
	}
	void* main = GetProcAddress(hModule, "sws_main");
	if (main)
	{
		((void(*)(DWORD))main)(0);
	}
	else
	{
		printf("Failed to find main function in ClassicWindowSwitcher.dll\n");
		FreeLibrary(hModule);
		return 1;
	}
	FreeLibrary(hModule);
	return 0;
}