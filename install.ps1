# ==============================================================================
# ilrmuzika — Windows PowerShell Automated Installer & Build Script
# ==============================================================================

$ErrorActionPreference = "Stop"

Write-Host "`n==================================================" -ForegroundColor Cyan
Write-Host "           ilrmuzika Windows Installer            " -ForegroundColor Cyan
Write-Host "==================================================`n" -ForegroundColor Cyan

function Check-Command($cmd) {
    return [bool](Get-Command $cmd -ErrorAction SilentlyContinue)
}

# 1. Dependency checks via winget
$missing = @()
if (-not (Check-Command "cmake")) { $missing += "Kitware.CMake" }
if (-not (Check-Command "ffmpeg")) { $missing += "Gyan.FFmpeg" }
if (-not (Check-Command "yt-dlp")) { $missing += "yt-dlp.yt-dlp" }
if (-not (Check-Command "python")) { $missing += "Python.Python.3.11" }

if ($missing.Count -gt 0) {
    Write-Host "Installing missing dependencies via winget..." -ForegroundColor Yellow
    foreach ($pkg in $missing) {
        Write-Host "  -> Installing $pkg"
        winget install --id $pkg -e --accept-source-agreements --accept-package-agreements
    }
    Write-Host "`nNote: Please restart PowerShell if PATH environment variables were updated.`n" -ForegroundColor Yellow
}

# 2. Python requests package for lyrics
try {
    python -m pip install requests --quiet
} catch {
    Write-Host "Warning: Failed to install python requests module. Lyrics will be disabled." -ForegroundColor Yellow
}

# 3. Build with CMake
Write-Host "`n--- Configuring and Compiling with CMake ---" -ForegroundColor Green
if (-not (Test-Path "build")) {
    New-Item -ItemType Directory -Path "build" | Out-Null
}

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

$binPath = "build\Release\ilrmuzika.exe"
if (-not (Test-Path $binPath)) {
    $binPath = "build\ilrmuzika.exe"
}

if (Test-Path $binPath) {
    Write-Host "`n==================================================" -ForegroundColor Green
    Write-Host "  Build successful! Binary at: $binPath" -ForegroundColor Green
    Write-Host "  Launch with: .\$binPath" -ForegroundColor Green
    Write-Host "==================================================" -ForegroundColor Green
} else {
    Write-Host "`nBuild failed: Could not locate compiled executable." -ForegroundColor Red
    exit 1
}
