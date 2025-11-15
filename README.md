# Classic Window Switcher
* Classic Window Switcher (cws) is a reimplementation of the classic CSRSS Windows Alt+Tab window switcher, designed to work on modern versions of Windows, including Windows 11 24H2 and later.
* Also, it features more modern features, such as UWP icon support, richer navigation with mouse and arrow keys, better HiDPI support, and more.
* Based on the Simple Window Switcher (sws) from ExplorerPatcher by Valinet.

![Build status](https://github.com/valinet/sws/actions/workflows/build.yml/badge.svg)

## Example usage
* Build `ClassicWindowSwitcher` and `cws-loader` projects, and run `ClassicWindowSwitcher.exe` from command line.
* Loading CWS inside Explorer.exe is also possible by using the `cws-loader.wh.cpp` Windhawk mod from this repository. You must change the DLL path in the mod settings to point to your path of `ClassicWindowSwitcher.dll`.
* You can also put the CWS DLL as `C:\Windows\dxgi.dll` to have it loaded by Explorer.exe automatically on every launch, replacing the default Windows Alt+Tab switcher.
* Note that you must disable the default Windows Alt+Tab switcher by setting `HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\AltTabSettings` to `0` (DWORD) for the standalone CWS executable to work properly.

## Compiling

If you need more information, you can also consult the GitHub Actions automated build file [here](https://github.com/valinet/sws/actions/workflows/msbuild.yml).

The following prerequisites are necessary in order to compile this project:

* Microsoft C/C++ Optimizing Compiler - this can be obtained by installing either of these packages:

  * Visual Studio - this is a fully featured IDE; you'll need to check "C/C++ application development role" when installing. If you do not require the full suite, use the package bellow.
  * Build Tools for Visual Studio - this just installs the compiler, which you'll be able to use from the command line, or from other applications like CMake

  Download either of those [here](http://go.microsoft.com/fwlink/p/?LinkId=840931). The guide assumes you have installed either Visual Studio 2019, either Build Tools for Visual Studio 2019.

* A recent version of the [Windows SDK](https://developer.microsoft.com/en-us/windows/downloads/windows-10-sdk/) - for development, version 10.0.19041.0 was used, available [here](https://go.microsoft.com/fwlink/p/?linkid=2120843) (this may also be offered as an option when installing Visual Studio)

* Git - you can use [Git for Windows](https://git-scm.com/download/win), or git command via the Windows Subsystem for Linux.

Steps:

1. Clone git repo

   ```
   git clone https://github.com/valinet/sws
   ```

   If "git" is not found as a command, type its full path, or have its folder added to PATH, or open Git command window in the respective folder if using Git for Windows.

2. Compile sws

   * Double click the `SimpleWindowSwitcher.sln` file to open the solution in Visual Studio. Choose Release and your processor architecture in the toolbar. Press `[Ctrl]`+`[Shift]`+`[B]` or choose "Build" - "Build solution" to compile.

   * Open an "x86 Native Tools Command Prompt for VS 2019" (for x86), or "x64 Native Tools Command Prompt for VS 2019" (for x64) (search that in Start), go to folder containing solution file and type:

     * For x86:

       ```
       msbuild ExplorerPatcher.sln /property:Configuration=Release /property:Platform=x86
       ```

     * For x64:

       ```
       msbuild ExplorerPatcher.sln /property:Configuration=Release /property:Platform=x64
       ```

   The resulting libraries will be in the "Release" (for x86) or "x64\Release" (for x64) folder in the directory containing the solution file.

That's it.

## Registry Keys
* `HKEY_CURRENT_USER\SOFTWARE\Ingan121\ClassicWindowSwitcher`
* Everything here is `REG_DWORD`

|Name|Description|Default Behavior|
|----|-----------|-------|
|`ShowDelay`|Set to the number of milliseconds to wait before showing the switcher.<br>Set to `0` to show the switcher immediately.|100 ms|
|`IncludeWallpaper`|Set to `1` to include the 'show desktop' item at the end of the list.|Hidden (`0`)|
|`PrimaryMonitorOnly`|Set to `1` to show the switcher only on the primary monitor.|Shown on the monitor where the mouse cursor is located (`0`)|
|`PerMonitor`|Not implemented yet. Only show windows on the switcher's monitor?|Disabled (`0`)|
|`NoPerApplicationList`|Set to `1` to disable the per-application window list, which is shown when `Alt+Tilde` is pressed.|Enabled (`0`)|
|`SwitcherIsPerApplication`|Set to `1` to make the switcher only show a single entry per application.|Disabled (`0`)|
|`AlwaysUseWindowTitleAndIcon`|Set to `1` to always use the window title and icon.|Disabled (`0`)|
|`ScrollWheelBehavior`|Sets the behavior of the mouse scroll wheel when the switcher is open.<br>`0`: Disabled<br>`1`: Move selection item by item, only when the cursor is over the switcher.<br>`2`: Move selection item by item, regardless of cursor position.<br>`3`: Scroll the grid list, only when the cursor is over the switcher.<br>`4`: Scroll the grid list if the cursor is over the switcher, otherwise move selection item by item.<br>`5`: Scroll the grid list, regardless of cursor position.<br>`6`: Move selection item by item if the cursor is over the switcher, otherwise scroll the grid list.<br>If there are not enough items to scroll, the behavior falls back to item by item movement.|Disabled (`0`)|
|`ScrollWheelInvert`|Set to `1` to invert the scroll wheel behavior.|Not inverted (`0`)|
|`SkipIfOneWindow`|Set to `1` to skip showing the switcher and immediately switch to the only window if there is just one window to switch to.|Enabled (`1`)|

* `HKEY_CURRENT_USER\Contol Panel\Desktop`

|Name|Type|Description|Default Behavior|
|----|----|-----------|-------|
|`CoolSwitchColumns`|REG_SZ|Set to the number of columns to use in the switcher grid.|7 columns|
|`CoolSwitchRows`|REG_SZ|Set to the number of rows to use in the switcher grid.|3 rows|