echo off
setlocal EnableDelayedExpansion

echo Usage:    makedist-win.bat ^<demo version^> ^<include AAX^> ^<include VST2^> ^<include VST3^> ^<sign the installer^>
echo           ^<demo version^>: Whether to build as demo. Possible values: 0 (no), 1 (yes). Default: 0.
echo           ^<include AAX^>: Whether to include AAX format. Possible values: 0 (no), 1 (yes). Default: 1.
echo           ^<include VST2^>: Whether to include VST2 format. Possible values: 0 (no), 1 (yes). Default: 1.
echo           ^<include VST3^>: Whether to include VST3 format. Possible values: 0 (no), 1 (yes). Default: 1.
echo           ^<sign the installer^>: Whether to sign the installer with a certificate. Possible values: 0 (no), 1 (yes). Default: 1.
echo           Example (no demo, does not include AAX and does not sign the installer, but includes VST2 and VST3): makedist-win.bat 0 0 1 1 0

REM - batch file to build MSVS project and zip the resulting binaries (or make installer)
REM - updating version numbers requires python and python path added to %PATH% env variable 
REM - zipping requires 7zip in %ProgramFiles%\7-Zip\7z.exe
REM - building installer requires innotsetup in "%ProgramFiles(x86)%\Inno Setup 5\iscc"
REM - AAX codesigning requires wraptool tool added to %PATH% env variable and aax.key/.crt in .\..\..\..\Certificates\

if "%1"=="1" (set "BUILD_DEMO=1") else (set "BUILD_DEMO=0")
if "%2"=="0" (set "INCLUDE_AAX=0") else (set "INCLUDE_AAX=1") 
if "%3"=="0" (set "INCLUDE_VST2=0") else (set "INCLUDE_VST2=1") 
if "%4"=="0" (set "INCLUDE_VST3=0") else (set "INCLUDE_VST3=1") 
if "%5"=="0" (set "SIGN_INSTALLER=0") else (set "SIGN_INSTALLER=1") 
echo Configuration based on parameter values: BUILD_DEMO = %BUILD_DEMO%, INCLUDE_AAX = %INCLUDE_AAX%, INCLUDE_VST2 = %INCLUDE_VST2%, INCLUDE_VST3 = %INCLUDE_VST3%, SIGN_INSTALLER = %SIGN_INSTALLER%


if BUILD_DEMO (echo Making IPlugEffect Windows DEMO VERSION distribution ...) else (echo Making IPlugEffect Windows FULL VERSION distribution ...)

echo "touching source"
cd ..\
copy /b *.cpp+,,

echo ------------------------------------------------------------------
echo Updating version numbers ...

call python scripts\prepare_resources-win.py %1
call python scripts\update_installer_version.py %1

echo ------------------------------------------------------------------
echo Building ...

if exist "%ProgramFiles(x86)%" (goto 64-Bit) else (goto 32-Bit)

if not defined DevEnvDir (
:32-Bit
echo 32-Bit O/S detected
call "%ProgramFiles%\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x86_x64
goto END

:64-Bit
echo 64-Bit Host O/S detected
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x86_x64
goto END
:END
)


REM - set preprocessor macros like this, for instance to enable demo build:
if %1 == 1 (
set CMDLINE_DEFINES="DEMO_VERSION=1"
REM -copy ".\resources\img\AboutBox_Demo.png" ".\resources\img\AboutBox.png"
) else (
set CMDLINE_DEFINES="DEMO_VERSION=0"
REM -copy ".\resources\img\AboutBox_Registered.png" ".\resources\img\AboutBox.png"
)

REM - Could build individual targets like this:
REM - msbuild IPlugEffect-app.vcxproj /p:configuration=release /p:platform=win32

echo Building 32 bit binaries...
msbuild IPlugEffect.sln /p:configuration=release /p:platform=win32 /nologo /verbosity:minimal /fileLogger /m /flp:logfile=build-win.log;errorsonly 

echo Building 64 bit binaries...
msbuild IPlugEffect.sln /p:configuration=release /p:platform=x64 /nologo /verbosity:minimal /fileLogger /m /flp:logfile=build-win.log;errorsonly;append

REM --echo Copying AAX Presets

REM --echo ------------------------------------------------------------------
REM --echo Code sign AAX binary...
REM --info at pace central, login via iLok license manager https://www.paceap.com/pace-central.html
REM --wraptool sign --verbose --account XXXXX --wcguid XXXXX --keyfile XXXXX.p12 --keypassword XXXXX --in .\build-win\aax\bin\IPlugEffect.aaxplugin\Contents\Win32\IPlugEffect.aaxplugin --out .\build-win\aax\bin\IPlugEffect.aaxplugin\Contents\Win32\IPlugEffect.aaxplugin
REM --wraptool sign --verbose --account XXXXX --wcguid XXXXX --keyfile XXXXX.p12 --keypassword XXXXX --in .\build-win\aax\bin\IPlugEffect.aaxplugin\Contents\x64\IPlugEffect.aaxplugin --out .\build-win\aax\bin\IPlugEffect.aaxplugin\Contents\x64\IPlugEffect.aaxplugin

REM - Make Installer (InnoSetup)

echo ------------------------------------------------------------------
echo Making Installer ...

if exist "%ProgramFiles(x86)%" (goto 64-Bit-is) else (goto 32-Bit-is)

:32-Bit-is
"%ProgramFiles%\Inno Setup 5\iscc" /Q /cc ".\installer\IPlugEffect.iss"
goto END-is

:64-Bit-is
"%ProgramFiles(x86)%\Inno Setup 5\iscc" /Q /cc ".\installer\IPlugEffect.iss"
goto END-is

:END-is

REM - Codesign Installer for Windows 8+
REM -"C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Bin\signtool.exe" sign /f "XXXXX.p12" /p XXXXX /d "IPlugEffect Installer" ".\installer\IPlugEffect Installer.exe"

REM -if %1 == 1 (
REM -copy ".\installer\IPlugEffect Installer.exe" ".\installer\IPlugEffect Demo Installer.exe"
REM -del ".\installer\IPlugEffect Installer.exe"
REM -)

REM - ZIP
echo ------------------------------------------------------------------
echo Making Zip File ...

call python scripts\make_zip.py %1

echo ------------------------------------------------------------------
echo Printing log file to console...

type build-win.log
