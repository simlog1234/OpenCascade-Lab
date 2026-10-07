<#
.SYNOPSIS
  Builds OpenCascade-Lab.sln (x64) with the MSBuild that ships with Visual Studio.

.DESCRIPTION
  `dotnet build` cannot compile the C++/CLI project (OcctProxy.vcxproj), so this script
  locates MSBuild.exe through vswhere and builds the whole solution with it.

.PARAMETER Configuration
  Debug or Release (default). Debug requires the "with-debug" OCCT package (bind/ + libd/).

.EXAMPLE
  .\scripts\build.ps1
  .\scripts\build.ps1 -Configuration Debug
#>
[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',
    [switch]$Clean
)

$ErrorActionPreference = 'Stop'
$RepoRoot = Split-Path -Parent $PSScriptRoot
$Solution = Join-Path $RepoRoot 'OpenCascade-Lab.sln'

if (-not (Test-Path (Join-Path $RepoRoot 'third_party\occt\inc\Standard_Version.hxx'))) {
    throw 'OCCT not found in third_party/occt. Run scripts\setup-occt.ps1 first.'
}

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $vswhere)) { throw 'vswhere.exe not found - is Visual Studio installed?' }

# Need: MSBuild + C++ toolset + C++/CLI support
$msbuild = & $vswhere -latest -products * `
    -requires Microsoft.Component.MSBuild Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
if (-not $msbuild) { throw 'MSBuild with the C++ toolset was not found. Install the "Desktop development with C++" workload and "C++/CLI support".' }

Write-Host "MSBuild : $msbuild"
Write-Host "Config  : $Configuration | x64"

$target = if ($Clean) { 'Rebuild' } else { 'Build' }
& $msbuild $Solution "/t:$target" "/p:Configuration=$Configuration" '/p:Platform=x64' '/restore' '/m' '/nologo' '/v:minimal'
if ($LASTEXITCODE -ne 0) { throw "Build failed (exit code $LASTEXITCODE)" }

Write-Host ''
Write-Host 'Build succeeded.' -ForegroundColor Green
Write-Host "  CLI   : src\OcctLab.Cli\bin\x64\$Configuration\net8.0\OcctLab.Cli.exe"
Write-Host "  Viewer: src\OcctLab.Viewer\bin\x64\$Configuration\net8.0-windows\OcctLab.Viewer.exe"
