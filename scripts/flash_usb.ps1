# Flashes a firmware image over USB (CH340 on COM7) without touching the settings:
#   .\scripts\flash_usb.ps1                      # the idf build (C:\pio\iradio-idf\...\firmware.bin)
#   .\scripts\flash_usb.ps1 -Bin .pio\build\radio\firmware.bin
# Writes the app to app0 (0x10000), then erases otadata (0xe000, 0x2000) so the bootloader starts
# app0, then resets the board. NVS (settings, pairings), the bootloader and the partition table
# stay as they are. A serial monitor holding the port (pio device monitor) is stopped first.
# Used whenever the radio is connected to the PC (user's instruction 2026-10-11); OTA otherwise.
param(
    [string]$Bin = "C:/pio/iradio-idf/.pio/build/idf/firmware.bin",
    [string]$Port = "COM7"
)
$ErrorActionPreference = "Continue"

if (-not (Test-Path $Bin)) { Write-Error "firmware not found: $Bin"; exit 1 }

# Free the port: a hidden pio device monitor keeps it open otherwise ("Access is denied").
Get-CimInstance Win32_Process |
    Where-Object { $_.CommandLine -match "device monitor" -and $_.CommandLine -match $Port } |
    ForEach-Object { Stop-Process -Id $_.ProcessId -Force -Confirm:$false -ErrorAction SilentlyContinue }
Start-Sleep -Milliseconds 500

# Write first, then switch the boot slot: a failed write leaves the old firmware bootable.
python -m esptool --chip esp32 --port $Port --baud 921600 --after no-reset write-flash 0x10000 $Bin
if ($LASTEXITCODE -ne 0) { Write-Error "write failed; nothing switched"; exit 1 }
python -m esptool --chip esp32 --port $Port --after hard-reset erase-region 0xe000 0x2000
if ($LASTEXITCODE -ne 0) { Write-Error "otadata erase failed"; exit 1 }
Write-Output "Flashed $Bin to app0 over $Port and reset the board."
