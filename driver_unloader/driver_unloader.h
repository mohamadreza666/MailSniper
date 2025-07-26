#pragma once

#include <ntddk.h>
#include <wdf.h>

// IOCTL codes for driver operations
#define IOCTL_UNLOAD_DRIVER CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_GET_LOADED_DRIVERS CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Structure for driver unload request
typedef struct _DRIVER_UNLOAD_REQUEST {
    WCHAR DriverName[256];
    ULONG Flags;
} DRIVER_UNLOAD_REQUEST, *PDRIVER_UNLOAD_REQUEST;

// Structure for driver information
typedef struct _DRIVER_INFO {
    WCHAR DriverName[256];
    WCHAR DriverPath[512];
    ULONG LoadCount;
    ULONG State;
} DRIVER_INFO, *PDRIVER_INFO;

// Function declarations
NTSTATUS UnloadDriver(_In_ PDRIVER_UNLOAD_REQUEST UnloadRequest);
NTSTATUS GetLoadedDrivers(_Out_ PDRIVER_INFO DriverInfo, _In_ size_t BufferLength, _Out_ size_t* BytesReturned);