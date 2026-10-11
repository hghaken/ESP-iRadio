# Builds the experimental env `idf` (Arduino 2.0.17 as an ESP-IDF 4.4.7 component, own
# sdkconfig; see docs/framework-build.md).
#
# ESP-IDF 4.4 cannot build in a path with a space, and this project lives in
# "D:\_AI Projects\...". A subst drive or junction does not help: PlatformIO resolves it back
# to the real path while CMake keeps the alias, and the two disagree. So this script mirrors
# the files the build needs into a folder without spaces and builds there.
#
# Usage (from the project root, PowerShell):   .\scripts\build_idf.ps1
# Result: C:\pio\iradio-idf\.pio\build\idf\firmware.bin; the resolved sdkconfig is copied
# back to the project as sdkconfig.idf so it can be reviewed and committed.
# Nothing is uploaded: env `idf` has upload disabled.

# Not "Stop": PowerShell 5.1 turns normal stderr output of native tools (git's "Cloning
# into ...") into errors. Exit codes are checked explicitly instead.
$ErrorActionPreference = "Continue"
$project = Split-Path -Parent $PSScriptRoot
$mirror = "C:\pio\iradio-idf"

New-Item -ItemType Directory -Force $mirror | Out-Null
# /MIR keeps the mirror identical; the excluded folders are neither copied nor purged, so the
# mirror keeps its own .pio (libraries, build cache) between runs.
robocopy $project $mirror /MIR /NFL /NDL /NJH /NJS /NP `
    /XD .pio .git .claude .vscode backup docs public `
    /XF *.png *.url sdkconfig.idf | Out-Null
if ($LASTEXITCODE -ge 8) { throw "robocopy failed with code $LASTEXITCODE" }

# ESP-IDF keeps the values of an existing sdkconfig and only takes sdkconfig.defaults for
# options it does not have yet. So a changed default is ignored unless the generated file
# goes: drop it whenever sdkconfig.defaults is newer.
$generated = "$mirror\sdkconfig.idf"
if ((Test-Path $generated) -and
    ((Get-Item "$mirror\sdkconfig.defaults").LastWriteTime -gt (Get-Item $generated).LastWriteTime)) {
    Remove-Item $generated
    Write-Host "sdkconfig.defaults changed: regenerating sdkconfig.idf"
}

Push-Location $mirror
try {
    pio run -e idf
    $result = $LASTEXITCODE
} finally {
    Pop-Location
}

if (Test-Path "$mirror\sdkconfig.idf") {
    Copy-Item "$mirror\sdkconfig.idf" "$project\sdkconfig.idf" -Force
}
if ($result -ne 0) { throw "pio run -e idf failed ($result)" }
Write-Host "firmware: $mirror\.pio\build\idf\firmware.bin"
