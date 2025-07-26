#
# Makefile for UnloadDriver
# Requires Windows Driver Kit (WDK) and Visual Studio Build Tools
#

# Default target
all: driver app

# Build the kernel driver
driver:
	@echo Building kernel driver...
	msbuild UnloadDriver.vcxproj /p:Configuration=Release /p:Platform=x64

# Build the user-mode application
app:
	@echo Building user-mode application...
	cl /Fe:UnloadApp.exe UnloadApp.c /link advapi32.lib

# Clean build artifacts
clean:
	@echo Cleaning build artifacts...
	del /Q *.obj *.exe *.sys *.pdb x64\Release\* 2>nul || echo No files to clean
	rmdir /S /Q x64 2>nul || echo No directories to clean

# Install the driver (requires administrator privileges)
install: driver
	@echo Installing driver...
	UnloadApp.exe install "$(PWD)\x64\Release\UnloadDriver.sys"

# Start the driver
start:
	@echo Starting driver...
	UnloadApp.exe start

# Stop the driver
stop:
	@echo Stopping driver...
	UnloadApp.exe stop

# Unload the driver completely
unload:
	@echo Unloading driver...
	UnloadApp.exe unload

# Test communication with driver
test:
	@echo Testing driver communication...
	UnloadApp.exe communicate

.PHONY: all driver app clean install start stop unload test