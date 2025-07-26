#include <ntddk.h>
#include <wdf.h>

// Function prototypes
DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;

// Device extension structure
typedef struct _DEVICE_EXTENSION {
    PDEVICE_OBJECT DeviceObject;
    UNICODE_STRING DeviceName;
    UNICODE_STRING SymbolicLinkName;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

// Forward declarations
NTSTATUS CreateDevice(PDRIVER_OBJECT DriverObject);
VOID DeleteDevice(PDRIVER_OBJECT DriverObject);
NTSTATUS DispatchCreate(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS DispatchClose(PDEVICE_OBJECT DeviceObject, PIRP Irp);
NTSTATUS DispatchDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);

// IOCTL codes for driver communication
#define IOCTL_UNLOAD_DRIVER CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

// Global variables
PDEVICE_OBJECT g_DeviceObject = NULL;

/**
 * DriverEntry - Driver initialization routine
 * 
 * @param DriverObject: Pointer to the driver object
 * @param RegistryPath: Registry path for this driver
 * 
 * @return: STATUS_SUCCESS if successful, error code otherwise
 */
NTSTATUS DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
)
{
    NTSTATUS status;
    
    UNREFERENCED_PARAMETER(RegistryPath);
    
    DbgPrint("UnloadDriver: DriverEntry called\n");
    
    // Set up the driver's dispatch routines
    DriverObject->MajorFunction[IRP_MJ_CREATE] = DispatchCreate;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = DispatchClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchDeviceControl;
    
    // Set the unload routine - this is crucial for proper driver unloading
    DriverObject->DriverUnload = DriverUnload;
    
    // Create the device
    status = CreateDevice(DriverObject);
    if (!NT_SUCCESS(status)) {
        DbgPrint("UnloadDriver: Failed to create device, status = 0x%08X\n", status);
        return status;
    }
    
    DbgPrint("UnloadDriver: Driver loaded successfully\n");
    return STATUS_SUCCESS;
}

/**
 * DriverUnload - Driver cleanup routine
 * 
 * @param DriverObject: Pointer to the driver object
 */
VOID DriverUnload(
    _In_ PDRIVER_OBJECT DriverObject
)
{
    DbgPrint("UnloadDriver: DriverUnload called\n");
    
    // Clean up resources
    DeleteDevice(DriverObject);
    
    DbgPrint("UnloadDriver: Driver unloaded successfully\n");
}

/**
 * CreateDevice - Creates the device object and symbolic link
 * 
 * @param DriverObject: Pointer to the driver object
 * 
 * @return: STATUS_SUCCESS if successful, error code otherwise
 */
NTSTATUS CreateDevice(PDRIVER_OBJECT DriverObject)
{
    NTSTATUS status;
    UNICODE_STRING deviceName;
    UNICODE_STRING symbolicLinkName;
    PDEVICE_EXTENSION deviceExtension;
    
    // Initialize device name
    RtlInitUnicodeString(&deviceName, L"\\Device\\UnloadDriver");
    
    // Create the device object
    status = IoCreateDevice(
        DriverObject,
        sizeof(DEVICE_EXTENSION),
        &deviceName,
        FILE_DEVICE_UNKNOWN,
        FILE_DEVICE_SECURE_OPEN,
        FALSE,
        &g_DeviceObject
    );
    
    if (!NT_SUCCESS(status)) {
        DbgPrint("UnloadDriver: IoCreateDevice failed, status = 0x%08X\n", status);
        return status;
    }
    
    // Initialize device extension
    deviceExtension = (PDEVICE_EXTENSION)g_DeviceObject->DeviceExtension;
    deviceExtension->DeviceObject = g_DeviceObject;
    RtlCopyUnicodeString(&deviceExtension->DeviceName, &deviceName);
    
    // Create symbolic link
    RtlInitUnicodeString(&symbolicLinkName, L"\\DosDevices\\UnloadDriver");
    RtlCopyUnicodeString(&deviceExtension->SymbolicLinkName, &symbolicLinkName);
    
    status = IoCreateSymbolicLink(&symbolicLinkName, &deviceName);
    if (!NT_SUCCESS(status)) {
        DbgPrint("UnloadDriver: IoCreateSymbolicLink failed, status = 0x%08X\n", status);
        IoDeleteDevice(g_DeviceObject);
        g_DeviceObject = NULL;
        return status;
    }
    
    // Set device flags
    g_DeviceObject->Flags |= DO_BUFFERED_IO;
    g_DeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    
    DbgPrint("UnloadDriver: Device created successfully\n");
    return STATUS_SUCCESS;
}

/**
 * DeleteDevice - Cleans up the device object and symbolic link
 * 
 * @param DriverObject: Pointer to the driver object
 */
VOID DeleteDevice(PDRIVER_OBJECT DriverObject)
{
    PDEVICE_EXTENSION deviceExtension;
    
    UNREFERENCED_PARAMETER(DriverObject);
    
    if (g_DeviceObject != NULL) {
        deviceExtension = (PDEVICE_EXTENSION)g_DeviceObject->DeviceExtension;
        
        // Delete symbolic link
        IoDeleteSymbolicLink(&deviceExtension->SymbolicLinkName);
        
        // Delete device object
        IoDeleteDevice(g_DeviceObject);
        g_DeviceObject = NULL;
        
        DbgPrint("UnloadDriver: Device deleted successfully\n");
    }
}

/**
 * DispatchCreate - Handles IRP_MJ_CREATE requests
 */
NTSTATUS DispatchCreate(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    
    DbgPrint("UnloadDriver: DispatchCreate called\n");
    
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    return STATUS_SUCCESS;
}

/**
 * DispatchClose - Handles IRP_MJ_CLOSE requests
 */
NTSTATUS DispatchClose(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    
    DbgPrint("UnloadDriver: DispatchClose called\n");
    
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    return STATUS_SUCCESS;
}

/**
 * DispatchDeviceControl - Handles IRP_MJ_DEVICE_CONTROL requests
 */
NTSTATUS DispatchDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp
)
{
    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION irpStack;
    ULONG controlCode;
    
    UNREFERENCED_PARAMETER(DeviceObject);
    
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    controlCode = irpStack->Parameters.DeviceIoControl.IoControlCode;
    
    DbgPrint("UnloadDriver: DispatchDeviceControl called with IOCTL: 0x%08X\n", controlCode);
    
    switch (controlCode) {
        case IOCTL_UNLOAD_DRIVER:
            DbgPrint("UnloadDriver: Received unload request\n");
            // Note: The actual unload will be handled by the user-mode application
            // calling the service control manager to stop/unload the driver
            status = STATUS_SUCCESS;
            break;
            
        default:
            DbgPrint("UnloadDriver: Unknown IOCTL code: 0x%08X\n", controlCode);
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }
    
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    
    return status;
}