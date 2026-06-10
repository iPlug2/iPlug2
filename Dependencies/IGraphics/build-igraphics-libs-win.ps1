# Build IGraphics libraries for Windows (x64 and ARM64EC)

$ErrorActionPreference = "Stop"

# Find vswhere
$vswherePath = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswherePath)) {
    Write-Error "vswhere not found. Please install Visual Studio."
    exit 1
}

# Find latest VS installation with C++ build tools
$vsPath = & $vswherePath -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsPath) {
    Write-Error "Could not find Visual Studio with C++ tools installed."
    exit 1
}

$vcvarsall = Join-Path $vsPath "VC\Auxiliary\Build\vcvarsall.bat"
if (-not (Test-Path $vcvarsall)) {
    Write-Error "vcvarsall.bat not found at: $vcvarsall"
    exit 1
}

# Check for ARM64EC tools
$vsPathArm64EC = & $vswherePath -latest -requires Microsoft.VisualStudio.Component.VC.Tools.ARM64EC -property installationPath
$hasArm64EC = $null -ne $vsPathArm64EC

$vsVersion = & $vswherePath -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property catalog_productDisplayVersion
Write-Host "Using Visual Studio $vsVersion at: $vsPath" -ForegroundColor Cyan
if (-not $hasArm64EC) {
    Write-Host "WARNING: ARM64EC tools not found. Install 'MSVC v143 - VS 2022 C++ ARM64EC build tools' in Visual Studio Installer to enable ARM64EC builds." -ForegroundColor Yellow
}

# Initialize VS environment and run msbuild
function Invoke-MsBuild {
    param(
        [string]$Platform,
        [string]$Configuration,
        [string]$VcvarsArch
    )

    Write-Host "Building $Configuration|$Platform..." -ForegroundColor Yellow

    $logFile = "build-win.log"
    $appendFlag = if ($script:firstBuild) { "" } else { ";append" }
    $script:firstBuild = $false

    cmd /c "`"$vcvarsall`" $VcvarsArch && msbuild IGraphicsLibraries.sln /p:configuration=$Configuration /p:platform=$Platform /nologo /verbosity:minimal /fileLogger /m /flp:logfile=$logFile;errorsonly$appendFlag"

    if ($LASTEXITCODE -ne 0) {
        Write-Error "Build failed for $Configuration|$Platform"
        exit $LASTEXITCODE
    }
}

$script:firstBuild = $true

# Build x64
Invoke-MsBuild -Platform "x64" -Configuration "Debug" -VcvarsArch "x64"
Invoke-MsBuild -Platform "x64" -Configuration "Release" -VcvarsArch "x64"

# Build ARM64EC (requires amd64_arm64 cross-compiler)
if ($hasArm64EC) {
    Invoke-MsBuild -Platform "ARM64EC" -Configuration "Debug" -VcvarsArch "amd64_arm64"
    Invoke-MsBuild -Platform "ARM64EC" -Configuration "Release" -VcvarsArch "amd64_arm64"
} else {
    Write-Host "Skipping ARM64EC builds (tools not installed)" -ForegroundColor Yellow
}

Write-Host "Build complete!" -ForegroundColor Green
