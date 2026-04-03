@echo off
set "MSVC=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207"
set "SDK=C:\Program Files (x86)\Windows Kits\10"
set "SDK_VER=10.0.26100.0"

set "PATH=%MSVC%\bin\Hostx86\x86;%PATH%"
set "INCLUDE=%MSVC%\include;%SDK%\Include\%SDK_VER%\ucrt;%SDK%\Include\%SDK_VER%\um;%SDK%\Include\%SDK_VER%\shared"
set "LIB=%MSVC%\lib\x86;%SDK%\Lib\%SDK_VER%\ucrt\x86;%SDK%\Lib\%SDK_VER%\um\x86"

cd /d "%~dp0"

echo.
echo === HideParty Build ===
echo.

if not exist LuaCore.lib (
    echo Generating LuaCore.lib from LuaCore.def...
    lib.exe /nologo /def:LuaCore.def /out:LuaCore.lib /machine:x86
    echo.
)

echo Compiling hideparty_mem.dll...
cl.exe /nologo /O2 /MT /LD /I. hideparty_mem.c /link /out:hideparty_mem.dll kernel32.lib /LIBPATH:.

if exist hideparty_mem.dll (
    echo.
    echo BUILD SUCCESS
    copy /Y hideparty_mem.dll "..\addon\hideparty_mem.dll"
    echo Copied to addon folder.
) else (
    echo.
    echo BUILD FAILED
)

echo.
pause
