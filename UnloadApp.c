#include <windows.h>
#include <stdio.h>
#include <winsvc.h>
#include "UnloadDriver.h"

/**
 * Opens a handle to the driver
 */
HANDLE OpenDriverHandle(void)
{
    HANDLE hDevice = CreateFileW(
        DEVICE_NAME,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    
    if (hDevice == INVALID_HANDLE_VALUE) {
        wprintf(L"Failed to open driver handle. Error: %lu\n", GetLastError());
    }
    
    return hDevice;
}

/**
 * Closes the driver handle
 */
void CloseDriverHandle(HANDLE hDevice)
{
    if (hDevice != INVALID_HANDLE_VALUE) {
        CloseHandle(hDevice);
    }
}

/**
 * Sends an unload request to the driver
 */
BOOL SendUnloadRequest(HANDLE hDevice)
{
    DWORD bytesReturned;
    BOOL result;
    
    if (hDevice == INVALID_HANDLE_VALUE) {
        wprintf(L"Invalid device handle\n");
        return FALSE;
    }
    
    result = DeviceIoControl(
        hDevice,
        IOCTL_UNLOAD_DRIVER,
        NULL,
        0,
        NULL,
        0,
        &bytesReturned,
        NULL
    );
    
    if (!result) {
        wprintf(L"DeviceIoControl failed. Error: %lu\n", GetLastError());
        return FALSE;
    }
    
    wprintf(L"Unload request sent successfully to driver\n");
    return TRUE;
}

/**
 * Unloads the driver using Service Control Manager
 */
BOOL UnloadDriverService(LPCWSTR serviceName)
{
    SC_HANDLE hSCManager = NULL;
    SC_HANDLE hService = NULL;
    SERVICE_STATUS serviceStatus;
    BOOL result = FALSE;
    
    // Open Service Control Manager
    hSCManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCManager == NULL) {
        wprintf(L"Failed to open Service Control Manager. Error: %lu\n", GetLastError());
        return FALSE;
    }
    
    // Open the service
    hService = OpenServiceW(hSCManager, serviceName, SERVICE_ALL_ACCESS);
    if (hService == NULL) {
        wprintf(L"Failed to open service '%s'. Error: %lu\n", serviceName, GetLastError());
        CloseServiceHandle(hSCManager);
        return FALSE;
    }
    
    // Stop the service
    if (!ControlService(hService, SERVICE_CONTROL_STOP, &serviceStatus)) {
        DWORD error = GetLastError();
        if (error == ERROR_SERVICE_NOT_ACTIVE) {
            wprintf(L"Service '%s' is not running\n", serviceName);
            result = TRUE;
        } else {
            wprintf(L"Failed to stop service '%s'. Error: %lu\n", serviceName, error);
        }
    } else {
        wprintf(L"Service '%s' stopped successfully\n", serviceName);
        result = TRUE;
    }
    
    // Clean up
    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
    
    return result;
}

/**
 * Installs the driver as a service
 */
BOOL InstallDriverService(LPCWSTR serviceName, LPCWSTR displayName, LPCWSTR driverPath)
{
    SC_HANDLE hSCManager = NULL;
    SC_HANDLE hService = NULL;
    BOOL result = FALSE;
    
    // Open Service Control Manager
    hSCManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCManager == NULL) {
        wprintf(L"Failed to open Service Control Manager. Error: %lu\n", GetLastError());
        return FALSE;
    }
    
    // Create the service
    hService = CreateServiceW(
        hSCManager,
        serviceName,
        displayName,
        SERVICE_ALL_ACCESS,
        SERVICE_KERNEL_DRIVER,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_NORMAL,
        driverPath,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    );
    
    if (hService == NULL) {
        DWORD error = GetLastError();
        if (error == ERROR_SERVICE_EXISTS) {
            wprintf(L"Service '%s' already exists\n", serviceName);
            result = TRUE;
        } else {
            wprintf(L"Failed to create service '%s'. Error: %lu\n", serviceName, error);
        }
    } else {
        wprintf(L"Service '%s' created successfully\n", serviceName);
        result = TRUE;
        CloseServiceHandle(hService);
    }
    
    CloseServiceHandle(hSCManager);
    return result;
}

/**
 * Starts the driver service
 */
BOOL StartDriverService(LPCWSTR serviceName)
{
    SC_HANDLE hSCManager = NULL;
    SC_HANDLE hService = NULL;
    BOOL result = FALSE;
    
    // Open Service Control Manager
    hSCManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCManager == NULL) {
        wprintf(L"Failed to open Service Control Manager. Error: %lu\n", GetLastError());
        return FALSE;
    }
    
    // Open the service
    hService = OpenServiceW(hSCManager, serviceName, SERVICE_ALL_ACCESS);
    if (hService == NULL) {
        wprintf(L"Failed to open service '%s'. Error: %lu\n", serviceName, GetLastError());
        CloseServiceHandle(hSCManager);
        return FALSE;
    }
    
    // Start the service
    if (!StartServiceW(hService, 0, NULL)) {
        DWORD error = GetLastError();
        if (error == ERROR_SERVICE_ALREADY_RUNNING) {
            wprintf(L"Service '%s' is already running\n", serviceName);
            result = TRUE;
        } else {
            wprintf(L"Failed to start service '%s'. Error: %lu\n", serviceName, error);
        }
    } else {
        wprintf(L"Service '%s' started successfully\n", serviceName);
        result = TRUE;
    }
    
    // Clean up
    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
    
    return result;
}

/**
 * Deletes the driver service
 */
BOOL DeleteDriverService(LPCWSTR serviceName)
{
    SC_HANDLE hSCManager = NULL;
    SC_HANDLE hService = NULL;
    BOOL result = FALSE;
    
    // Open Service Control Manager
    hSCManager = OpenSCManagerW(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (hSCManager == NULL) {
        wprintf(L"Failed to open Service Control Manager. Error: %lu\n", GetLastError());
        return FALSE;
    }
    
    // Open the service
    hService = OpenServiceW(hSCManager, serviceName, SERVICE_ALL_ACCESS);
    if (hService == NULL) {
        wprintf(L"Failed to open service '%s'. Error: %lu\n", serviceName, GetLastError());
        CloseServiceHandle(hSCManager);
        return FALSE;
    }
    
    // Delete the service
    if (!DeleteService(hService)) {
        wprintf(L"Failed to delete service '%s'. Error: %lu\n", serviceName, GetLastError());
    } else {
        wprintf(L"Service '%s' deleted successfully\n", serviceName);
        result = TRUE;
    }
    
    // Clean up
    CloseServiceHandle(hService);
    CloseServiceHandle(hSCManager);
    
    return result;
}

/**
 * Prints usage information
 */
void PrintUsage(LPCWSTR programName)
{
    wprintf(L"Usage: %s <command> [options]\n\n", programName);
    wprintf(L"Commands:\n");
    wprintf(L"  install <driver_path>   - Install the driver as a service\n");
    wprintf(L"  start                   - Start the driver service\n");
    wprintf(L"  stop                    - Stop the driver service\n");
    wprintf(L"  unload                  - Unload the driver (stop + delete service)\n");
    wprintf(L"  communicate             - Send unload request to driver\n");
    wprintf(L"  delete                  - Delete the driver service\n");
    wprintf(L"\nExample:\n");
    wprintf(L"  %s install C:\\\\Drivers\\\\UnloadDriver.sys\n", programName);
    wprintf(L"  %s start\n", programName);
    wprintf(L"  %s unload\n", programName);
}

/**
 * Main function
 */
int wmain(int argc, wchar_t* argv[])
{
    const wchar_t* serviceName = L"UnloadDriver";
    const wchar_t* displayName = L"Unload Driver Example";
    
    wprintf(L"Windows Driver Unloader Utility\n");
    wprintf(L"================================\n\n");
    
    if (argc < 2) {
        PrintUsage(argv[0]);
        return 1;
    }
    
    if (_wcsicmp(argv[1], L"install") == 0) {
        if (argc < 3) {
            wprintf(L"Error: Driver path required for install command\n");
            PrintUsage(argv[0]);
            return 1;
        }
        
        if (InstallDriverService(serviceName, displayName, argv[2])) {
            wprintf(L"Driver installation completed successfully\n");
            return 0;
        } else {
            wprintf(L"Driver installation failed\n");
            return 1;
        }
    }
    else if (_wcsicmp(argv[1], L"start") == 0) {
        if (StartDriverService(serviceName)) {
            wprintf(L"Driver started successfully\n");
            return 0;
        } else {
            wprintf(L"Failed to start driver\n");
            return 1;
        }
    }
    else if (_wcsicmp(argv[1], L"stop") == 0) {
        if (UnloadDriverService(serviceName)) {
            wprintf(L"Driver stopped successfully\n");
            return 0;
        } else {
            wprintf(L"Failed to stop driver\n");
            return 1;
        }
    }
    else if (_wcsicmp(argv[1], L"delete") == 0) {
        if (DeleteDriverService(serviceName)) {
            wprintf(L"Driver service deleted successfully\n");
            return 0;
        } else {
            wprintf(L"Failed to delete driver service\n");
            return 1;
        }
    }
    else if (_wcsicmp(argv[1], L"unload") == 0) {
        wprintf(L"Unloading driver...\n");
        
        // First stop the service
        if (UnloadDriverService(serviceName)) {
            wprintf(L"Driver stopped successfully\n");
            
            // Then delete the service
            if (DeleteDriverService(serviceName)) {
                wprintf(L"Driver unloaded and service deleted successfully\n");
                return 0;
            } else {
                wprintf(L"Driver stopped but failed to delete service\n");
                return 1;
            }
        } else {
            wprintf(L"Failed to stop driver\n");
            return 1;
        }
    }
    else if (_wcsicmp(argv[1], L"communicate") == 0) {
        HANDLE hDevice = OpenDriverHandle();
        if (hDevice != INVALID_HANDLE_VALUE) {
            if (SendUnloadRequest(hDevice)) {
                wprintf(L"Communication with driver successful\n");
                CloseDriverHandle(hDevice);
                return 0;
            } else {
                wprintf(L"Failed to communicate with driver\n");
                CloseDriverHandle(hDevice);
                return 1;
            }
        } else {
            wprintf(L"Failed to open driver handle\n");
            return 1;
        }
    }
    else {
        wprintf(L"Error: Unknown command '%s'\n\n", argv[1]);
        PrintUsage(argv[0]);
        return 1;
    }
}