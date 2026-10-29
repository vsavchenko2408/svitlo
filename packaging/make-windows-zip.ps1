# Builds a portable Windows package (ZIP) from a Release build.
#
# Usage (PowerShell):
#   packaging\make-windows-zip.ps1 -Exe build\Release\Svitlo.exe -QtDir C:\Qt\6.8.3\msvc2022_64
#
# Result: dist\Svitlo-<version>-windows-x64.zip containing Svitlo.exe, the Qt
# DLLs and plugins (windeployqt), the Visual C++ runtime DLLs and licences.
# Users unzip it anywhere and run Svitlo.exe — no installation needed.

param(
    [Parameter(Mandatory = $true)][string]$Exe,
    [Parameter(Mandatory = $true)][string]$QtDir
)

$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
$Exe = (Resolve-Path $Exe).Path
$BuildDir = Split-Path -Parent (Split-Path -Parent $Exe)   # build\Release\Svitlo.exe -> build
if (-not (Test-Path (Join-Path $BuildDir "licenses"))) { $BuildDir = Split-Path -Parent $Exe }

# Version from CMakeLists.txt: project(Svitlo VERSION x.y.z ...)
$Version = "0.0.0"
$m = Select-String -Path (Join-Path $Root "CMakeLists.txt") -Pattern 'project\(Svitlo VERSION ([0-9.]+)'
if ($m) { $Version = $m.Matches[0].Groups[1].Value }

$Dist  = Join-Path (Get-Location) "dist"
$Stage = Join-Path $Dist "Svitlo"
$Zip   = Join-Path $Dist "Svitlo-$Version-windows-x64.zip"

Write-Host "==> Preparing $Stage"
if (Test-Path $Stage) { Remove-Item $Stage -Recurse -Force }
if (Test-Path $Zip)   { Remove-Item $Zip -Force }
New-Item -ItemType Directory -Path $Stage | Out-Null
Copy-Item $Exe $Stage

Write-Host "==> Bundling Qt (windeployqt)"
# Interface translations are compiled into Svitlo.exe, so Qt's are not needed.
& (Join-Path $QtDir "bin\windeployqt.exe") --release --no-translations (Join-Path $Stage "Svitlo.exe")
if ($LASTEXITCODE -ne 0) { throw "windeployqt failed" }

Write-Host "==> Copying the Visual C++ runtime"
$vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
$vsPath = & $vswhere -latest -products * -property installationPath
$crt = Get-ChildItem -Path (Join-Path $vsPath "VC\Redist\MSVC") -Recurse -Directory -Filter "Microsoft.VC14*.CRT" |
       Where-Object { $_.FullName -match '\\x64\\' -and $_.FullName -notmatch 'onecore|debug_nonredist' } |
       Select-Object -First 1
if ($crt) {
    Copy-Item (Join-Path $crt.FullName "*.dll") $Stage
} else {
    Write-Warning "Visual C++ runtime not found; users may need the VC++ Redistributable."
}

Write-Host "==> Copying licences"
$lic = Join-Path $BuildDir "licenses"
if (Test-Path $lic) { Copy-Item $lic (Join-Path $Stage "licenses") -Recurse }

Write-Host "==> Creating $Zip"
Compress-Archive -Path $Stage -DestinationPath $Zip
Write-Host "Done: $Zip"
