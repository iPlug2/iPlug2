#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Build script for Windows platforms
.DESCRIPTION
    Builds plugin formats (VST2, VST3, AAX, CLAP, APP) for x64 and/or ARM64EC
.PARAMETER Platform
    Target platform(s). Comma-separated list: x64,ARM64EC
    Default: x64
.PARAMETER Format
    Build only specific format(s). Comma-separated list: vst2,vst3,aax,clap,app
    Default: all formats
.PARAMETER Quick
    Build only IPlugEffect as a quick test
.PARAMETER StopOnError
    Stop on first build error
.EXAMPLE
    .\buildall-win.ps1
    Build all formats for x64
.EXAMPLE
    .\buildall-win.ps1 -Platform "x64,ARM64EC"
    Build all formats for both platforms
.EXAMPLE
    .\buildall-win.ps1 -Platform ARM64EC -Format "vst3,clap" -Quick
    Quick test of VST3 and CLAP for ARM64EC
#>

param(
    [string]$Platform = "x64",
    [string]$Format = "",
    [switch]$Quick,
    [switch]$StopOnError,
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Continue"

# Get repo root (parent of Examples folder)
$ScriptDir = $PSScriptRoot
if (-not $ScriptDir) {
    $ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
}
$RepoRoot = Split-Path -Parent $ScriptDir

# Find MSBuild
$VSWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$MSBuildPath = $null
if (Test-Path $VSWhere) {
    $MSBuildPath = & $VSWhere -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe | Select-Object -First 1
}
if (-not $MSBuildPath -or -not (Test-Path $MSBuildPath)) {
    Write-Error "MSBuild not found. Please install Visual Studio with C++ workload."
    exit 1
}

# Parse platform filter
$AllPlatforms = @("x64", "ARM64EC")
$BuildPlatforms = $Platform -split "," | ForEach-Object { $_.Trim() }
foreach ($p in $BuildPlatforms) {
    if ($p -notin $AllPlatforms) {
        Write-Error "Unknown platform: $p. Valid platforms: $($AllPlatforms -join ', ')"
        exit 1
    }
}

# Parse format filter
$AllFormats = @("vst2", "vst3", "aax", "clap", "app")
if ($Format -eq "") {
    $BuildFormats = $AllFormats
} else {
    $BuildFormats = $Format.ToLower() -split "," | ForEach-Object { $_.Trim() }
    foreach ($f in $BuildFormats) {
        if ($f -notin $AllFormats) {
            Write-Error "Unknown format: $f. Valid formats: $($AllFormats -join ', ')"
            exit 1
        }
    }
}

Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "iPlug2 Build Script" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "MSBuild: $MSBuildPath"
Write-Host "Repo Root: $RepoRoot"
Write-Host "Configuration: $Configuration"
Write-Host "Platforms: $($BuildPlatforms -join ', ')"
Write-Host "Formats: $($BuildFormats -join ', ')"
Write-Host "Quick Test: $Quick"
Write-Host "============================================================"
Write-Host ""

# All Example projects - excludes macOS-only projects (IPlugCocoaUI, IPlugSwiftUI)
$AllProjects = @(
    @{ Name = "IPlugChunks"; Path = "Examples\IPlugChunks" },
    @{ Name = "IPlugControls"; Path = "Examples\IPlugControls" },
    @{ Name = "IPlugConvoEngine"; Path = "Examples\IPlugConvoEngine" },
    @{ Name = "IPlugDrumSynth"; Path = "Examples\IPlugDrumSynth" },
    @{ Name = "IPlugEffect"; Path = "Examples\IPlugEffect" },
    @{ Name = "IPlugInstrument"; Path = "Examples\IPlugInstrument" },
    @{ Name = "IPlugMidiEffect"; Path = "Examples\IPlugMidiEffect" },
    @{ Name = "IPlugOSCEditor"; Path = "Examples\IPlugOSCEditor" },
    @{ Name = "IPlugP5js"; Path = "Examples\IPlugP5js" },
    @{ Name = "IPlugReaperPlugin"; Path = "Examples\IPlugReaperPlugin" },
    @{ Name = "IPlugResponsiveUI"; Path = "Examples\IPlugResponsiveUI" },
    @{ Name = "IPlugSideChain"; Path = "Examples\IPlugSideChain" },
    @{ Name = "IPlugSurroundEffect"; Path = "Examples\IPlugSurroundEffect" },
    @{ Name = "IPlugSvelteUI"; Path = "Examples\IPlugSvelteUI" },
    @{ Name = "IPlugVisualizer"; Path = "Examples\IPlugVisualizer" },
    @{ Name = "IPlugWebUI"; Path = "Examples\IPlugWebUI" }
)

$PassCount = 0
$FailCount = 0
$SkipCount = 0

function Build-Target {
    param(
        [string]$ProjectName,
        [string]$ProjectPath,
        [string]$Format,
        [string]$TargetPlatform
    )

    $SolutionPath = Join-Path $RepoRoot "$ProjectPath\$ProjectName.sln"
    $Target = "$ProjectName-$Format"

    if (-not (Test-Path $SolutionPath)) {
        Write-Host "[SKIP] $Target ($TargetPlatform) - solution not found" -ForegroundColor Yellow
        return @{ Success = $null; Message = "Solution not found" }
    }

    Write-Host ""
    Write-Host "------------------------------------------------------------" -ForegroundColor DarkGray
    Write-Host "Building $Target ($TargetPlatform)" -ForegroundColor White
    Write-Host "------------------------------------------------------------" -ForegroundColor DarkGray

    # Run MSBuild and show output in real-time (merge stderr into stdout)
    & $MSBuildPath $SolutionPath /t:$Target /p:Configuration=$Configuration /p:Platform=$TargetPlatform /m /v:minimal 2>&1 | ForEach-Object {
        if ($_ -is [System.Management.Automation.ErrorRecord]) {
            Write-Host $_.ToString() -ForegroundColor Red
        } else {
            Write-Host $_
        }
    }

    $exitCode = $LASTEXITCODE

    if ($exitCode -eq 0) {
        Write-Host "[$Target] " -NoNewline -ForegroundColor White
        Write-Host "PASS" -ForegroundColor Green
        return @{ Success = $true; Message = "Success" }
    } else {
        Write-Host "[$Target] " -NoNewline -ForegroundColor White
        Write-Host "FAIL (exit code: $exitCode)" -ForegroundColor Red
        return @{ Success = $false; Message = "Build failed" }
    }
}

function Build-Format {
    param(
        [string]$FormatName,
        [array]$Projects,
        [string]$TargetPlatform
    )

    Write-Host ""
    Write-Host "============================================================" -ForegroundColor Cyan
    Write-Host "Building $($FormatName.ToUpper()) targets ($TargetPlatform)..." -ForegroundColor Cyan
    Write-Host "============================================================" -ForegroundColor Cyan

    foreach ($proj in $Projects) {
        $result = Build-Target -ProjectName $proj.Name -ProjectPath $proj.Path -Format $FormatName -TargetPlatform $TargetPlatform
        if ($result.Success -eq $true) { $script:PassCount++ }
        elseif ($result.Success -eq $false) {
            $script:FailCount++
            if ($StopOnError) { return $false }
        }
        else { $script:SkipCount++ }
    }
    return $true
}

# Main build loop
foreach ($plat in $BuildPlatforms) {
    if ($StopOnError -and $FailCount -gt 0) { break }

    Write-Host ""
    Write-Host "############################################################" -ForegroundColor Magenta
    Write-Host "# Platform: $plat" -ForegroundColor Magenta
    Write-Host "############################################################" -ForegroundColor Magenta

    if ($Quick) {
        # Quick mode - just IPlugEffect
        foreach ($fmt in $BuildFormats) {
            if ($StopOnError -and $FailCount -gt 0) { break }
            $result = Build-Target -ProjectName "IPlugEffect" -ProjectPath "Examples\IPlugEffect" -Format $fmt -TargetPlatform $plat
            if ($result.Success -eq $true) { $PassCount++ }
            elseif ($result.Success -eq $false) { $FailCount++ }
            else { $SkipCount++ }
        }
    }
    else {
        foreach ($fmt in $BuildFormats) {
            if ($StopOnError -and $FailCount -gt 0) { break }

            $continue = Build-Format -FormatName $fmt -Projects $AllProjects -TargetPlatform $plat
            if (-not $continue) { break }
        }
    }
}

# Summary
Write-Host ""
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "Build Summary" -ForegroundColor Cyan
Write-Host "============================================================" -ForegroundColor Cyan
Write-Host "Passed:  $PassCount" -ForegroundColor Green
Write-Host "Failed:  $FailCount" -ForegroundColor $(if ($FailCount -gt 0) { "Red" } else { "Green" })
Write-Host "Skipped: $SkipCount" -ForegroundColor Yellow
Write-Host "============================================================"

if ($FailCount -gt 0) {
    Write-Host ""
    Write-Host "Some builds failed!" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "All builds completed successfully!" -ForegroundColor Green
exit 0
