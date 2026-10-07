<#
.SYNOPSIS
  Downloads the OCCT V8.0.1 Windows (vc14, x64) binary package from GitHub Releases
  and extracts it to third_party/occt (+ third_party/3rdparty).

.DESCRIPTION
  Release page: https://github.com/Open-Cascade-SAS/OCCT/releases/tag/V8.0.1

  Packages (each is a zip that contains one inner zip):
    occt-combined-release-no-pch.zip      ~245 MB  OCCT Release + all 3rd-party libs
    occt-combined-with-debug-no-pch.zip   ~443 MB  OCCT Release + Debug (bind/libd) + 3rd-party

  Resulting layout (what Directory.Build.props expects):
    third_party/occt/inc                 headers
    third_party/occt/win64/vc14/bin|lib  Release DLLs / import libs
    third_party/occt/win64/vc14/bind|libd Debug DLLs / import libs (with-debug package only)
    third_party/3rdparty/<lib>/bin       freetype, tbb, freeimage, ...

.PARAMETER WithDebug
  Download the package that also contains Debug binaries (needed for Debug|x64 builds).

.PARAMETER Force
  Re-download / re-extract even if third_party/occt already exists.

.EXAMPLE
  .\scripts\setup-occt.ps1 -WithDebug
#>
[CmdletBinding()]
param(
    [switch]$WithDebug,
    [switch]$Force
)

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'   # Invoke-WebRequest is much faster without the progress bar

$Version    = 'V8.0.1'
$Asset      = if ($WithDebug) { 'occt-combined-with-debug-no-pch.zip' } else { 'occt-combined-release-no-pch.zip' }
$Url        = "https://github.com/Open-Cascade-SAS/OCCT/releases/download/$Version/$Asset"

$RepoRoot   = Split-Path -Parent $PSScriptRoot
$ThirdParty = Join-Path $RepoRoot 'third_party'
$Downloads  = Join-Path $ThirdParty '_downloads'
$OcctDir    = Join-Path $ThirdParty 'occt'
$LibsDir    = Join-Path $ThirdParty '3rdparty'
$ZipPath    = Join-Path $Downloads $Asset

if ((Test-Path (Join-Path $OcctDir 'win64\vc14\bin\TKernel.dll')) -and -not $Force) {
    Write-Host "OCCT already present at $OcctDir (use -Force to re-install)." -ForegroundColor Green
    exit 0
}

New-Item -ItemType Directory -Force $Downloads | Out-Null

if (-not (Test-Path $ZipPath) -or $Force) {
    Write-Host "Downloading $Url ..." -ForegroundColor Cyan
    Invoke-WebRequest -Uri $Url -OutFile $ZipPath -UseBasicParsing
}
Write-Host ("Package: {0} ({1:N0} MB)" -f $ZipPath, ((Get-Item $ZipPath).Length / 1MB))

# Outer zip -> inner zip (opencascade-8.0.1-vc14-64-*.zip)
Write-Host 'Extracting outer archive ...' -ForegroundColor Cyan
Expand-Archive -Path $ZipPath -DestinationPath $Downloads -Force
$Inner = Get-ChildItem $Downloads -Filter 'opencascade-8.0.1-vc14-64*.zip' | Select-Object -First 1
if ($null -eq $Inner) { throw "Inner archive not found in $Downloads" }

$Extract = Join-Path $ThirdParty '_extract'
if (Test-Path $Extract) { Remove-Item -Recurse -Force $Extract }
Write-Host "Extracting $($Inner.Name) (this takes a few minutes, ~1.8 GB) ..." -ForegroundColor Cyan
Expand-Archive -Path $Inner.FullName -DestinationPath $Extract -Force

foreach ($pair in @(@{ From = 'opencascade-8.0.1-vc14-64'; To = $OcctDir }, @{ From = '3rdparty-vc14-64'; To = $LibsDir })) {
    $src = Join-Path $Extract $pair.From
    if (-not (Test-Path $src)) { throw "Expected folder missing in archive: $($pair.From)" }
    if (Test-Path $pair.To) { Remove-Item -Recurse -Force $pair.To }
    Move-Item -Path $src -Destination $pair.To
}
Remove-Item -Recurse -Force $Extract

Write-Host ''
Write-Host "OCCT $Version installed:" -ForegroundColor Green
Write-Host "  headers : $OcctDir\inc"
Write-Host "  release : $OcctDir\win64\vc14\bin"
if (Test-Path (Join-Path $OcctDir 'win64\vc14\bind')) { Write-Host "  debug   : $OcctDir\win64\vc14\bind" }
Write-Host "  3rdparty: $LibsDir"
Write-Host ''
Write-Host 'Next: .\scripts\build.ps1   (or open OpenCascade-Lab.sln in Visual Studio)'
