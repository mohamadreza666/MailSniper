@echo off
REM Windows Driver Unloader Utility - Batch Script
REM Requires Administrator privileges for driver operations

setlocal enabledelayedexpansion

echo ==================================
echo Windows Driver Unloader Utility
echo ==================================
echo.

REM Check for command line arguments
if "%1"=="" goto :show_usage
if "%1"=="help" goto :show_usage
if "%1"=="/?" goto :show_usage

REM Check if UnloadApp.exe exists, build if necessary
if not exist "UnloadApp.exe" (
    echo Building UnloadApp.exe...
    cl /Fe:UnloadApp.exe UnloadApp.c /link advapi32.lib
    if errorlevel 1 (
        echo Error: Failed to build UnloadApp.exe
        echo Make sure Visual Studio Build Tools are installed and cl.exe is in PATH
        goto :error_exit
    )
    echo Build completed successfully
    echo.
)

REM Execute the requested action
if /i "%1"=="install" goto :install
if /i "%1"=="start" goto :start
if /i "%1"=="stop" goto :stop
if /i "%1"=="unload" goto :unload
if /i "%1"=="delete" goto :delete
if /i "%1"=="test" goto :test
if /i "%1"=="status" goto :status

echo Error: Unknown command "%1"
goto :show_usage

:install
if "%2"=="" (
    echo Error: Driver path required for install command
    echo Usage: %0 install ^<driver_path^>
    goto :error_exit
)
echo Installing driver from: %2
UnloadApp.exe install "%2"
if errorlevel 1 (
    echo Driver installation failed
    goto :error_exit
) else (
    echo Driver installed successfully
)
goto :success_exit

:start
echo Starting driver...
UnloadApp.exe start
if errorlevel 1 (
    echo Failed to start driver
    goto :error_exit
) else (
    echo Driver started successfully
)
goto :success_exit

:stop
echo Stopping driver...
UnloadApp.exe stop
if errorlevel 1 (
    echo Failed to stop driver
    goto :error_exit
) else (
    echo Driver stopped successfully
)
goto :success_exit

:delete
echo Deleting driver service...
UnloadApp.exe delete
if errorlevel 1 (
    echo Failed to delete driver service
    goto :error_exit
) else (
    echo Driver service deleted successfully
)
goto :success_exit

:unload
echo Unloading driver (stop + delete)...
UnloadApp.exe unload
if errorlevel 1 (
    echo Failed to unload driver
    goto :error_exit
) else (
    echo Driver unloaded successfully
)
goto :success_exit

:test
echo Testing communication with driver...
UnloadApp.exe communicate
if errorlevel 1 (
    echo Communication test failed
    echo Make sure the driver is installed and running
    goto :error_exit
) else (
    echo Communication test successful
)
goto :success_exit

:status
echo Checking driver status...
sc query UnloadDriver >nul 2>&1
if errorlevel 1 (
    echo Service not found
) else (
    sc query UnloadDriver
)
goto :success_exit

:show_usage
echo Usage: %0 ^<command^> [options]
echo.
echo Commands:
echo   install ^<driver_path^>   - Install the driver as a service
echo   start                   - Start the driver service
echo   stop                    - Stop the driver service
echo   unload                  - Unload the driver (stop + delete service)
echo   delete                  - Delete the driver service
echo   test                    - Test communication with driver
echo   status                  - Show driver service status
echo   help                    - Show this help message
echo.
echo Examples:
echo   %0 install C:\Drivers\UnloadDriver.sys
echo   %0 start
echo   %0 test
echo   %0 unload
echo.
echo Note: Driver operations require Administrator privileges
goto :normal_exit

:error_exit
echo.
echo Operation failed!
exit /b 1

:success_exit
echo.
echo Operation completed successfully!
exit /b 0

:normal_exit
exit /b 0