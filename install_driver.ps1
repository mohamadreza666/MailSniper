# PowerShell script to install and manage the UnloadDriver
# Requires Administrator privileges

param(
    [Parameter(Mandatory=$false)]
    [ValidateSet("install", "start", "stop", "unload", "status", "test")]
    [string]$Action = "status",
    
    [Parameter(Mandatory=$false)]
    [string]$DriverPath = ""
)

# Check if running as Administrator
function Test-Administrator {
    $currentUser = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($currentUser)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

# Get the service status
function Get-DriverStatus {
    try {
        $service = Get-Service -Name "UnloadDriver" -ErrorAction SilentlyContinue
        if ($service) {
            Write-Host "Service Status: $($service.Status)" -ForegroundColor Green
            return $service.Status
        } else {
            Write-Host "Service not found" -ForegroundColor Yellow
            return "NotFound"
        }
    } catch {
        Write-Host "Error checking service status: $($_.Exception.Message)" -ForegroundColor Red
        return "Error"
    }
}

# Main script
Write-Host "==================================" -ForegroundColor Cyan
Write-Host "Windows Driver Unloader Utility" -ForegroundColor Cyan
Write-Host "==================================" -ForegroundColor Cyan
Write-Host ""

# Check administrator privileges for actions that require them
if ($Action -in @("install", "start", "stop", "unload") -and -not (Test-Administrator)) {
    Write-Host "Error: This action requires Administrator privileges!" -ForegroundColor Red
    Write-Host "Please run PowerShell as Administrator and try again." -ForegroundColor Yellow
    exit 1
}

# Build the application if it doesn't exist
if (-not (Test-Path "UnloadApp.exe")) {
    Write-Host "Building UnloadApp.exe..." -ForegroundColor Yellow
    try {
        & cl /Fe:UnloadApp.exe UnloadApp.c /link advapi32.lib
        if ($LASTEXITCODE -ne 0) {
            throw "Build failed with exit code $LASTEXITCODE"
        }
        Write-Host "Build completed successfully" -ForegroundColor Green
    } catch {
        Write-Host "Error building application: $($_.Exception.Message)" -ForegroundColor Red
        Write-Host "Make sure Visual Studio Build Tools are installed and cl.exe is in PATH" -ForegroundColor Yellow
        exit 1
    }
}

# Execute the requested action
switch ($Action.ToLower()) {
    "install" {
        if ([string]::IsNullOrEmpty($DriverPath)) {
            # Try to find the driver in common build locations
            $possiblePaths = @(
                "x64\Release\UnloadDriver.sys",
                "Release\UnloadDriver.sys",
                "UnloadDriver.sys"
            )
            
            foreach ($path in $possiblePaths) {
                if (Test-Path $path) {
                    $DriverPath = (Resolve-Path $path).Path
                    break
                }
            }
            
            if ([string]::IsNullOrEmpty($DriverPath)) {
                Write-Host "Error: Driver file not found!" -ForegroundColor Red
                Write-Host "Please specify the driver path or build the driver first." -ForegroundColor Yellow
                Write-Host "Usage: .\install_driver.ps1 -Action install -DriverPath 'C:\path\to\UnloadDriver.sys'" -ForegroundColor Yellow
                exit 1
            }
        }
        
        if (-not (Test-Path $DriverPath)) {
            Write-Host "Error: Driver file not found at: $DriverPath" -ForegroundColor Red
            exit 1
        }
        
        Write-Host "Installing driver from: $DriverPath" -ForegroundColor Yellow
        & .\UnloadApp.exe install $DriverPath
        if ($LASTEXITCODE -eq 0) {
            Write-Host "Driver installed successfully" -ForegroundColor Green
        } else {
            Write-Host "Driver installation failed" -ForegroundColor Red
            exit 1
        }
    }
    
    "start" {
        Write-Host "Starting driver..." -ForegroundColor Yellow
        & .\UnloadApp.exe start
        if ($LASTEXITCODE -eq 0) {
            Write-Host "Driver started successfully" -ForegroundColor Green
        } else {
            Write-Host "Failed to start driver" -ForegroundColor Red
            exit 1
        }
    }
    
    "stop" {
        Write-Host "Stopping driver..." -ForegroundColor Yellow
        & .\UnloadApp.exe stop
        if ($LASTEXITCODE -eq 0) {
            Write-Host "Driver stopped successfully" -ForegroundColor Green
        } else {
            Write-Host "Failed to stop driver" -ForegroundColor Red
            exit 1
        }
    }
    
    "unload" {
        Write-Host "Unloading driver (stop + delete)..." -ForegroundColor Yellow
        & .\UnloadApp.exe unload
        if ($LASTEXITCODE -eq 0) {
            Write-Host "Driver unloaded successfully" -ForegroundColor Green
        } else {
            Write-Host "Failed to unload driver" -ForegroundColor Red
            exit 1
        }
    }
    
    "test" {
        $status = Get-DriverStatus
        if ($status -eq "Running") {
            Write-Host "Testing communication with driver..." -ForegroundColor Yellow
            & .\UnloadApp.exe communicate
            if ($LASTEXITCODE -eq 0) {
                Write-Host "Communication test successful" -ForegroundColor Green
            } else {
                Write-Host "Communication test failed" -ForegroundColor Red
                exit 1
            }
        } else {
            Write-Host "Cannot test communication - driver is not running" -ForegroundColor Yellow
            Write-Host "Use 'start' action to start the driver first" -ForegroundColor Yellow
        }
    }
    
    "status" {
        Get-DriverStatus
    }
    
    default {
        Write-Host "Unknown action: $Action" -ForegroundColor Red
        Write-Host "Valid actions: install, start, stop, unload, status, test" -ForegroundColor Yellow
    }
}

Write-Host ""
Write-Host "Operation completed." -ForegroundColor Cyan