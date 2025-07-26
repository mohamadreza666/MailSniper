#include <ntddk.h>
#include <wdf.h>

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD DriverUnloaderEvtDeviceAdd;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL DriverUnloaderEvtIoDeviceControl;

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

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    NTSTATUS status;
    WDF_DRIVER_CONFIG config;

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "DriverUnloader: DriverEntry called\n"));

    WDF_DRIVER_CONFIG_INIT(&config, DriverUnloaderEvtDeviceAdd);

    status = WdfDriverCreate(
        DriverObject,
        RegistryPath,
        WDF_NO_OBJECT_ATTRIBUTES,
        &config,
        WDF_NO_HANDLE
    );

    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "DriverUnloader: WdfDriverCreate failed: 0x%x\n", status));
    }

    return status;
}

NTSTATUS
DriverUnloaderEvtDeviceAdd(
    _In_    WDFDRIVER       Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
)
{
    NTSTATUS status;
    WDFDEVICE device;
    WDF_IO_QUEUE_CONFIG queueConfig;
    WDF_OBJECT_ATTRIBUTES deviceAttributes;

    UNREFERENCED_PARAMETER(Driver);

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "DriverUnloader: DriverUnloaderEvtDeviceAdd called\n"));

    // Set device type
    WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_UNKNOWN);

    // Set IO type
    WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoBuffered);

    // Create device
    WDF_OBJECT_ATTRIBUTES_INIT(&deviceAttributes);
    status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device);

    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "DriverUnloader: WdfDeviceCreate failed: 0x%x\n", status));
        return status;
    }

    // Create symbolic link
    UNICODE_STRING symLink = RTL_CONSTANT_STRING(L"\\DosDevices\\DriverUnloader");
    status = WdfDeviceCreateSymbolicLink(device, &symLink);

    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "DriverUnloader: WdfDeviceCreateSymbolicLink failed: 0x%x\n", status));
    }

    // Configure I/O queue
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchSequential);
    queueConfig.EvtIoDeviceControl = DriverUnloaderEvtIoDeviceControl;

    status = WdfIoQueueCreate(
        device,
        &queueConfig,
        WDF_NO_OBJECT_ATTRIBUTES,
        WDF_NO_HANDLE
    );

    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "DriverUnloader: WdfIoQueueCreate failed: 0x%x\n", status));
    }

    return status;
}

VOID
DriverUnloaderEvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
)
{
    NTSTATUS status = STATUS_SUCCESS;
    size_t bytesReturned = 0;
    PDRIVER_UNLOAD_REQUEST unloadRequest;
    PDRIVER_INFO driverInfo;

    UNREFERENCED_PARAMETER(Queue);

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "DriverUnloader: IOCTL received: 0x%x\n", IoControlCode));

    switch (IoControlCode) {
    case IOCTL_UNLOAD_DRIVER:
        if (InputBufferLength < sizeof(DRIVER_UNLOAD_REQUEST)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        status = WdfRequestRetrieveInputBuffer(
            Request,
            sizeof(DRIVER_UNLOAD_REQUEST),
            &unloadRequest,
            NULL
        );

        if (!NT_SUCCESS(status)) {
            break;
        }

        status = UnloadDriver(unloadRequest);
        break;

    case IOCTL_GET_LOADED_DRIVERS:
        if (OutputBufferLength < sizeof(DRIVER_INFO)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        status = WdfRequestRetrieveOutputBuffer(
            Request,
            sizeof(DRIVER_INFO),
            &driverInfo,
            NULL
        );

        if (!NT_SUCCESS(status)) {
            break;
        }

        status = GetLoadedDrivers(driverInfo, OutputBufferLength, &bytesReturned);
        break;

    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    WdfRequestCompleteWithInformation(Request, status, bytesReturned);
}

NTSTATUS
UnloadDriver(
    _In_ PDRIVER_UNLOAD_REQUEST UnloadRequest
)
{
    NTSTATUS status;
    UNICODE_STRING driverName;
    PDRIVER_OBJECT driverObject;

    KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "DriverUnloader: Attempting to unload driver: %ws\n", UnloadRequest->DriverName));

    RtlInitUnicodeString(&driverName, UnloadRequest->DriverName);

    // Get driver object
    status = ObReferenceObjectByName(
        &driverName,
        OBJ_CASE_INSENSITIVE,
        NULL,
        0,
        *IoDriverObjectType,
        KernelMode,
        NULL,
        &driverObject
    );

    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "DriverUnloader: ObReferenceObjectByName failed: 0x%x\n", status));
        return status;
    }

    // Check if driver can be unloaded
    if (driverObject->DeviceObject != NULL) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_WARNING_LEVEL, "DriverUnloader: Driver has active devices, cannot unload\n"));
        ObDereferenceObject(driverObject);
        return STATUS_DRIVER_BLOCKED;
    }

    // Attempt to unload the driver
    status = IoUnloadDriver(driverObject);

    if (!NT_SUCCESS(status)) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "DriverUnloader: IoUnloadDriver failed: 0x%x\n", status));
    } else {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, "DriverUnloader: Driver unloaded successfully\n"));
    }

    ObDereferenceObject(driverObject);
    return status;
}

NTSTATUS
GetLoadedDrivers(
    _Out_ PDRIVER_INFO DriverInfo,
    _In_ size_t BufferLength,
    _Out_ size_t* BytesReturned
)
{
    NTSTATUS status;
    ULONG driverCount = 0;
    PDRIVER_OBJECT* driverObjects;
    ULONG size;

    // Get size needed for driver objects
    status = IoEnumerateRegisteredFiltersList(
        NULL,
        0,
        &size
    );

    if (status != STATUS_BUFFER_TOO_SMALL) {
        return status;
    }

    driverObjects = ExAllocatePool2(POOL_FLAG_NON_PAGED, size, 'DrvL');
    if (driverObjects == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = IoEnumerateRegisteredFiltersList(
        driverObjects,
        size,
        &driverCount
    );

    if (NT_SUCCESS(status) && driverCount > 0) {
        // Copy first driver info
        RtlZeroMemory(DriverInfo, sizeof(DRIVER_INFO));
        
        if (driverObjects[0]->DriverName.Buffer != NULL) {
            RtlCopyMemory(
                DriverInfo->DriverName,
                driverObjects[0]->DriverName.Buffer,
                min(sizeof(DriverInfo->DriverName) - sizeof(WCHAR), driverObjects[0]->DriverName.Length)
            );
        }
        
        DriverInfo->LoadCount = 1;
        DriverInfo->State = driverObjects[0]->Flags;
        
        *BytesReturned = sizeof(DRIVER_INFO);
    } else {
        *BytesReturned = 0;
    }

    ExFreePool(driverObjects);
    return status;
}