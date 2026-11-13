@echo off
rem Compila reaper_reaglass-x64.dll (Windows). Use: build.bat [install]
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
cd /d "%~dp0"
if not exist build mkdir build
cl /nologo /std:c++17 /O2 /EHsc /MT /LD /utf-8 /DUNICODE /D_UNICODE /DWEBVIEW_STATIC /DWEBVIEW_EDGE ^
  /Ideps\reaper-sdk\sdk /Ideps\webview\core\include /Ideps\wv2\build\native\include ^
  src\main.cpp src\window_win32.cpp ^
  /Fobuild\ /Fe:build\reaper_reaglass-x64.dll ^
  /link user32.lib gdi32.lib ole32.lib shell32.lib shlwapi.lib version.lib advapi32.lib oleaut32.lib || exit /b 1
if /i "%1"=="install" copy /y build\reaper_reaglass-x64.dll "%APPDATA%\REAPER\UserPlugins\"
