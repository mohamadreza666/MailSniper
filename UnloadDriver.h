#ifndef UNLOAD_DRIVER_H
#define UNLOAD_DRIVER_H

#include <windows.h>
#include <winioctl.h>

// Device name for user-mode applications
#define DEVICE_NAME L"\\\\.\\UnloadDriver"

// IOCTL codes
#define IOCTL_UNLOAD_DRIVER CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Function prototypes for user-mode applications
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Opens a handle to the driver
 * @return: Valid handle on success, INVALID_HANDLE_VALUE on failure
 */
HANDLE OpenDriverHandle(void);

/**
 * Closes the driver handle
 * @param hDevice: Handle to close
 */
void CloseDriverHandle(HANDLE hDevice);

/**
 * Sends an unload request to the driver
 * @param hDevice: Handle to the driver
 * @return: TRUE on success, FALSE on failure
 */
BOOL SendUnloadRequest(HANDLE hDevice);

/**
 * Unloads the driver using Service Control Manager
 * @param serviceName: Name of the service to unload
 * @return: TRUE on success, FALSE on failure
 */
BOOL UnloadDriverService(LPCWSTR serviceName);

#ifdef __cplusplus
}
#endif

#endif // UNLOAD_DRIVER_H