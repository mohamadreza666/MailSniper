#include <ntddk.h>
#include <ntstrsafe.h>

// Device interface GUID for our driver unloader
DEFINE_GUID(GUID_DEVICE_INTERFACE,
    0x12345678, 0x1234, 0x1234, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12);

// Device name and symbolic link
#define DEVICE_NAME L"\\Device\\DriverUnloader"
#define SYMBOLIC_LINK_NAME L"\\DosDevices\\DriverUnloader"

// IOCTL codes
#define IOCTL_UNLOAD_DRIVER CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Function prototypes
DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;
NTSTATUS DeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS CreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);

// Global variables
PDEVICE_OBJECT g_DeviceObject = NULL;

// Driver entry point
NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
    UNICODE_STRING deviceName;
    UNICODE_STRING symbolicLinkName;
    NTSTATUS status;

    UNREFERENCED_PARAMETER(RegistryPath);

    DbgPrint("DriverUnloader: Driver loaded successfully\n");

    // Initialize device name
    RtlInitUnicodeString(&deviceName, DEVICE_NAME);
    RtlInitUnicodeString(&symbolicLinkName, SYMBOLIC_LINK_NAME);

    // Create device object
    status = IoCreateDevice(
        DriverObject,
        0,
        &deviceName,
        FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &g_DeviceObject
    );

    if (!NT_SUCCESS(status)) {
        DbgPrint("DriverUnloader: Failed to create device object, status: 0x%X\n", status);
        return status;
    }

    // Set up dispatch routines
    DriverObject->MajorFunction[IRP_MJ_CREATE] = CreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = CreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceControl;
    DriverObject->DriverUnload = DriverUnload;

    // Create symbolic link
    status = IoCreateSymbolicLink(&symbolicLinkName, &deviceName);
    if (!NT_SUCCESS(status)) {
        DbgPrint("DriverUnloader: Failed to create symbolic link, status: 0x%X\n", status);
        IoDeleteDevice(g_DeviceObject);
        return status;
    }

    // Set device flags
    g_DeviceObject->Flags |= DO_BUFFERED_IO;
    g_DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;

    DbgPrint("DriverUnloader: Device created successfully\n");
    return STATUS_SUCCESS;
}

// Driver unload routine
VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
    UNICODE_STRING symbolicLinkName;

    DbgPrint("DriverUnloader: Driver unloading\n");

    // Delete symbolic link
    RtlInitUnicodeString(&symbolicLinkName, SYMBOLIC_LINK_NAME);
    IoDeleteSymbolicLink(&symbolicLinkName);

    // Delete device object
    if (g_DeviceObject) {
        IoDeleteDevice(g_DeviceObject);
    }

    DbgPrint("DriverUnloader: Driver unloaded successfully\n");
}

// Create/Close dispatch routine
NTSTATUS CreateClose(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;
}

// Device control dispatch routine
NTSTATUS DeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PIO_STACK_LOCATION stack;
    NTSTATUS status = STATUS_SUCCESS;
    ULONG bytesIO = 0;

    UNREFERENCED_PARAMETER(DeviceObject);

    stack = IoGetCurrentIrpStackLocation(Irp);

    switch (stack->Parameters.DeviceIoControl.IoControlCode) {
        case IOCTL_UNLOAD_DRIVER: {
            PUNICODE_STRING driverName = (PUNICODE_STRING)Irp->AssociatedIrp.SystemBuffer;
            
            if (stack->Parameters.DeviceIoControl.InputBufferLength < sizeof(UNICODE_STRING)) {
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            DbgPrint("DriverUnloader: Attempting to unload driver: %wZ\n", driverName);

            // Attempt to unload the driver
            status = ZwUnloadDriver(driverName);
            
            if (NT_SUCCESS(status)) {
                DbgPrint("DriverUnloader: Successfully unloaded driver: %wZ\n", driverName);
                bytesIO = sizeof(UNICODE_STRING);
            } else {
                DbgPrint("DriverUnloader: Failed to unload driver: %wZ, status: 0x%X\n", driverName, status);
            }
            break;
        }
        default:
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = bytesIO;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return status;
}