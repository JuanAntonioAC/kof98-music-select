@echo off
setlocal
set "BIN=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207\bin\Hostx64\x64"
set "ROOT=%~dp0"
set "ROOT=%ROOT:~0,-1%"
set "SRC=%ROOT%\src"
set "OUT=%ROOT%\build"
set "DIST=%ROOT%\dist"
if not exist "%OUT%" mkdir "%OUT%"
if not exist "%DIST%" mkdir "%DIST%"

"%BIN%\lib.exe" /nologo /machine:x64 /def:"%SRC%\kernel32.def" /out:"%OUT%\kernel32.lib"
if errorlevel 1 exit /b 1
"%BIN%\lib.exe" /nologo /machine:x64 /def:"%SRC%\user32.def" /out:"%OUT%\user32.lib"
if errorlevel 1 exit /b 1
"%BIN%\lib.exe" /nologo /machine:x64 /def:"%SRC%\gdi32.def" /out:"%OUT%\gdi32.lib"
if errorlevel 1 exit /b 1

"%BIN%\cl.exe" /nologo /c /TC /O2 /GS- /Gs999999 /Zl /W3 /I"%SRC%" /Fo"%OUT%\crtstub.obj" "%SRC%\crtstub.c"
if errorlevel 1 exit /b 1
"%BIN%\cl.exe" /nologo /c /TC /O2 /GS- /Gs999999 /Zl /W3 /I"%SRC%" /Fo"%OUT%\str.obj" "%SRC%\str.c"
if errorlevel 1 exit /b 1
"%BIN%\cl.exe" /nologo /c /TC /O2 /GS- /Gs999999 /Zl /W3 /I"%SRC%" /Fo"%OUT%\bgm.obj" "%SRC%\bgm.c"
if errorlevel 1 exit /b 1
"%BIN%\cl.exe" /nologo /c /TC /O2 /GS- /Gs999999 /Zl /W3 /I"%SRC%" /Fo"%OUT%\library.obj" "%SRC%\library.c"
if errorlevel 1 exit /b 1
"%BIN%\cl.exe" /nologo /c /TC /O2 /GS- /Gs999999 /Zl /W3 /I"%SRC%" /Fo"%OUT%\menu.obj" "%SRC%\menu.c"
if errorlevel 1 exit /b 1
"%BIN%\cl.exe" /nologo /c /TC /O2 /GS- /Gs999999 /Zl /W3 /I"%SRC%" /Fo"%OUT%\test_bgm.obj" "%SRC%\test_bgm.c"
if errorlevel 1 exit /b 1
"%BIN%\cl.exe" /nologo /c /TC /O2 /GS- /Gs999999 /Zl /W3 /I"%SRC%" /Fo"%OUT%\dllmain.obj" "%SRC%\dllmain.c"
if errorlevel 1 exit /b 1

"%BIN%\link.exe" /nologo /NODEFAULTLIB /SUBSYSTEM:CONSOLE /ENTRY:main /OUT:"%DIST%\BgmSelectTest.exe" "%OUT%\test_bgm.obj" "%OUT%\library.obj" "%OUT%\menu.obj" "%OUT%\bgm.obj" "%OUT%\str.obj" "%OUT%\crtstub.obj" "%OUT%\kernel32.lib"
if errorlevel 1 exit /b 1
"%DIST%\BgmSelectTest.exe"
if errorlevel 1 exit /b 1

"%BIN%\link.exe" /nologo /DLL /NODEFAULTLIB /SUBSYSTEM:WINDOWS /ENTRY:DllMain /OUT:"%DIST%\Kof98Bgm.dll" "%OUT%\dllmain.obj" "%OUT%\library.obj" "%OUT%\menu.obj" "%OUT%\bgm.obj" "%OUT%\str.obj" "%OUT%\crtstub.obj" "%OUT%\kernel32.lib" "%OUT%\user32.lib" "%OUT%\gdi32.lib"
if errorlevel 1 exit /b 1

dotnet publish "%SRC%\Launcher\Kof98BgmLauncher.csproj" -c Release -o "%DIST%" --nologo -v q
if errorlevel 1 exit /b 1
exit /b 0
