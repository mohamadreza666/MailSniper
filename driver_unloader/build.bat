@echo off
echo Building Driver Unloader...

REM Set up Visual Studio environment
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat"

REM Set up WDK environment
call "C:\Program Files (x86)\Windows Kits\10\bin\10.0.22000.0\x64\setenv.bat"

REM Build the driver
cl /I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.22000.0\km" ^
   /I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.22000.0\shared" ^
   /I"C:\Program Files (x86)\Windows Kits\10\Include\wdf\kmdf\1.15" ^
   /D_AMD64_=1 /DWIN32=1 /D_WIN32=1 /DWIN64=1 /D_WIN64=1 ^
   /DWINVER=0x0A00 /D_WIN32_WINNT=0x0A00 ^
   /DWIN32_LEAN_AND_MEAN=1 /DNTDDI_VERSION=NTDDI_WIN10 ^
   /DWDF_VERSION_MAJOR=1 /DWDF_VERSION_MINOR=15 ^
   /W4 /WX /GS /Zi /Gy /Od /Oi /EHsc ^
   /Fe:driver_unloader.sys ^
   /link /DEBUG /DRIVER /SUBSYSTEM:NATIVE /ENTRY:DriverEntry ^
   /LIBPATH:"C:\Program Files (x86)\Windows Kits\10\Lib\10.0.22000.0\km\x64" ^
   /LIBPATH:"C:\Program Files (x86)\Windows Kits\10\Lib\wdf\kmdf\x64\1.15" ^
   ntoskrnl.lib hal.lib wdf01000.lib ^
   driver_unloader.c

if %ERRORLEVEL% EQU 0 (
    echo Build successful!
    echo Driver file: driver_unloader.sys
) else (
    echo Build failed!
)

pause