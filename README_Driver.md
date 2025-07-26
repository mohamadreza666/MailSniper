# Windows Driver Unloader

A complete Windows kernel driver example that demonstrates proper driver loading, communication, and unloading functionality. This project includes both the kernel driver and user-mode utilities for managing the driver lifecycle.

## Features

- **Kernel Driver**: A WDM-based Windows kernel driver with proper DriverEntry and DriverUnload routines
- **User-Mode Application**: Command-line utility for driver management and communication
- **Service Management**: Complete service installation, starting, stopping, and removal
- **IOCTL Communication**: Bidirectional communication between user-mode and kernel-mode
- **Proper Cleanup**: Ensures all resources are properly released during driver unload
- **Multiple Interfaces**: PowerShell script, batch file, and direct executable usage

## Project Structure

```
├── UnloadDriver.c          # Kernel driver source code
├── UnloadDriver.h          # Header file with IOCTL definitions
├── UnloadApp.c             # User-mode application source
├── UnloadDriver.vcxproj    # Visual Studio project file
├── makefile                # Build automation
├── install_driver.ps1      # PowerShell management script
├── install_driver.bat      # Batch file for command prompt
└── README_Driver.md        # This documentation
```

## Prerequisites

### Development Requirements
- **Windows Driver Kit (WDK)**: Latest version recommended
- **Visual Studio**: 2019 or later with C++ development tools
- **Windows SDK**: Compatible with your target Windows version

### Runtime Requirements
- **Windows**: Windows 7 or later (x64 recommended)
- **Administrator Privileges**: Required for driver operations
- **Secure Boot**: May need to be disabled for unsigned drivers in development

## Building the Project

### Option 1: Using Visual Studio
1. Open `UnloadDriver.vcxproj` in Visual Studio
2. Select Release configuration and x64 platform
3. Build the solution (Ctrl+Shift+B)

### Option 2: Using Command Line (with WDK environment)
```cmd
# Build the driver
msbuild UnloadDriver.vcxproj /p:Configuration=Release /p:Platform=x64

# Build the user-mode application
cl /Fe:UnloadApp.exe UnloadApp.c /link advapi32.lib
```

### Option 3: Using the provided Makefile
```cmd
make all
```

## Usage

### PowerShell Script (Recommended)
```powershell
# Check status
.\install_driver.ps1 -Action status

# Install driver (automatically finds built driver)
.\install_driver.ps1 -Action install

# Or specify driver path explicitly
.\install_driver.ps1 -Action install -DriverPath "C:\path\to\UnloadDriver.sys"

# Start the driver
.\install_driver.ps1 -Action start

# Test communication
.\install_driver.ps1 -Action test

# Stop the driver
.\install_driver.ps1 -Action stop

# Completely unload (stop + delete service)
.\install_driver.ps1 -Action unload
```

### Batch File
```cmd
# Install driver
install_driver.bat install "C:\path\to\UnloadDriver.sys"

# Start driver
install_driver.bat start

# Test communication
install_driver.bat test

# Unload driver
install_driver.bat unload

# Show help
install_driver.bat help
```

### Direct Executable Usage
```cmd
# Install the driver as a service
UnloadApp.exe install "C:\path\to\UnloadDriver.sys"

# Start the driver
UnloadApp.exe start

# Communicate with the driver
UnloadApp.exe communicate

# Stop the driver
UnloadApp.exe stop

# Delete the service
UnloadApp.exe delete

# Complete unload (stop + delete)
UnloadApp.exe unload
```

## Driver Architecture

### Kernel Driver (`UnloadDriver.c`)
- **DriverEntry**: Initializes the driver, creates device object, and sets up dispatch routines
- **DriverUnload**: Properly cleans up all resources when driver is unloaded
- **Device Creation**: Creates a named device object with symbolic link for user-mode access
- **IRP Handling**: Processes Create, Close, and DeviceControl requests
- **IOCTL Support**: Handles custom I/O control codes for communication

### Key Components:
```c
// Device object creation
IoCreateDevice(..., &g_DeviceObject);
IoCreateSymbolicLink(&symbolicLinkName, &deviceName);

// Unload routine assignment (crucial for proper unloading)
DriverObject->DriverUnload = DriverUnload;

// Proper cleanup in DriverUnload
IoDeleteSymbolicLink(&deviceExtension->SymbolicLinkName);
IoDeleteDevice(g_DeviceObject);
```

### User-Mode Application (`UnloadApp.c`)
- **Service Management**: Uses Service Control Manager (SCM) APIs
- **Driver Communication**: Uses DeviceIoControl for IOCTL communication
- **Resource Management**: Proper handle cleanup and error handling
- **Multiple Commands**: Install, start, stop, delete, unload, and test operations

## IOCTL Communication

The driver supports custom IOCTL codes for communication:

```c
#define IOCTL_UNLOAD_DRIVER CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
```

### Communication Flow:
1. User-mode app opens handle to driver using `CreateFile`
2. Sends IOCTL using `DeviceIoControl`
3. Kernel driver processes request in `DispatchDeviceControl`
4. Driver responds with appropriate status code

## Security Considerations

### Driver Signing
- **Development**: Use test signing for development/testing
- **Production**: Drivers must be properly signed for release

### Enable Test Signing (Development Only):
```cmd
# Enable test signing (requires reboot)
bcdedit /set testsigning on

# Disable after testing
bcdedit /set testsigning off
```

### Permissions
- Driver installation requires administrator privileges
- Service management operations need elevated permissions
- File system access for driver files

## Troubleshooting

### Common Issues

1. **"Driver failed to load"**
   - Check if test signing is enabled
   - Verify driver is built for correct architecture (x64/x86)
   - Ensure no syntax errors in driver code

2. **"Access Denied" errors**
   - Run as Administrator
   - Check UAC settings
   - Verify user has necessary privileges

3. **"Device not found" when communicating**
   - Ensure driver is installed and running
   - Check device name and symbolic link creation
   - Verify IOCTL code matches between user and kernel mode

4. **Build failures**
   - Install Windows Driver Kit (WDK)
   - Set up proper build environment
   - Check Visual Studio integration

### Debugging

1. **Kernel Debugging**:
   ```cmd
   # View debug output
   DbgView.exe  # From Sysinternals
   
   # Or use WinDbg for advanced debugging
   ```

2. **Service Status**:
   ```cmd
   # Check service status
   sc query UnloadDriver
   
   # View service configuration
   sc qc UnloadDriver
   ```

3. **Event Logs**:
   - Check Windows Event Viewer
   - Look in System and Application logs
   - Driver events often appear in System log

## Advanced Usage

### Custom Driver Development
This project serves as a template for custom driver development:

1. Modify `UnloadDriver.c` for your specific functionality
2. Add custom IOCTL codes in `UnloadDriver.h`
3. Update the user-mode application accordingly
4. Implement proper error handling and logging

### Integration with Other Applications
The user-mode library functions can be integrated into other applications:

```c
#include "UnloadDriver.h"

// Example usage
HANDLE hDevice = OpenDriverHandle();
if (hDevice != INVALID_HANDLE_VALUE) {
    BOOL result = SendUnloadRequest(hDevice);
    CloseDriverHandle(hDevice);
}
```

## Best Practices

1. **Always implement DriverUnload**: Essential for proper driver unloading
2. **Clean up resources**: Delete device objects and symbolic links
3. **Handle errors gracefully**: Provide meaningful error messages
4. **Use proper synchronization**: For multi-threaded access
5. **Validate input**: Check all user-mode input in kernel driver
6. **Follow Windows driver guidelines**: Microsoft's WDK documentation

## Contributing

When contributing to this project:
1. Follow Windows driver development best practices
2. Test on multiple Windows versions
3. Ensure proper error handling
4. Update documentation for new features
5. Maintain backward compatibility when possible

## License

This project is provided as-is for educational and development purposes. Please ensure compliance with your organization's policies and applicable laws when using driver code.

## Additional Resources

- [Windows Driver Kit Documentation](https://docs.microsoft.com/en-us/windows-hardware/drivers/)
- [Kernel Driver Development Best Practices](https://docs.microsoft.com/en-us/windows-hardware/drivers/kernel/)
- [Service Control Manager](https://docs.microsoft.com/en-us/windows/win32/services/service-control-manager)
- [Driver Signing Requirements](https://docs.microsoft.com/en-us/windows-hardware/drivers/install/driver-signing)