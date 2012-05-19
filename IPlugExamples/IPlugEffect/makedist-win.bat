REM - batch file to build 32&64 bit VS2010 VST/APP project and VS2005 RTAS project and zip the resulting binaries
REM - zipping requires 7zip in %ProgramFiles%\7-Zip\7z.exe
REM - building installer requires innotsetup in "%ProgramFiles(x86)%\Inno Setup 5\iscc"

echo "making IPlugEffect win distribution..."

echo "updating version numbers"
call python update_version.py

REM - START VST2/VST3/APP VS2010

if exist "%ProgramFiles(x86)%" (goto 64-Bit) else (goto 32-Bit)

:32-Bit
echo 32-Bit O/S detected
call "%ProgramFiles%\Microsoft Visual Studio 10.0\VC\vcvarsall.bat"
goto END

:64-Bit
echo 64-Bit Host O/S detected
call "%ProgramFiles(x86)%\Microsoft Visual Studio 10.0\VC\vcvarsall.bat"
goto END
:END

REM - set preprocessor macros like this, for instance to enable demo build:
REM - SET CMDLINE_DEFINES="DEMO_VERSION"

REM - Could build individual projects like this:
REM - msbuild IPlugEffect.vcxproj /p:configuration=release /p:platform=win32
REM - msbuild IPlugEffect-app.vcxproj /p:configuration=release /p:platform=win32
REM - msbuild IPlugEffect.vcxproj /p:configuration=release /p:platform=x64
REM - msbuild IPlugEffect-app.vcxproj /p:configuration=release /p:platform=x64

msbuild IPlugEffect.sln /p:configuration=release /p:platform=win32
msbuild IPlugEffect.sln /p:configuration=release /p:platform=x64

REM - START RTAS VS2005

REM - this is bit clumsy, oh well
if exist "%ProgramFiles(x86)%" (goto 64-Bit-rtas) else (goto 32-Bit-rtas)

:32-Bit-rtas
echo 32-Bit O/S detected
call "%ProgramFiles%\Microsoft Visual Studio 8\VC\vcvarsall.bat"
goto END-rtas

:64-Bit-rtas
echo 64-Bit Host O/S detected
call "%ProgramFiles(x86)%\Microsoft Visual Studio 8\VC\vcvarsall.bat"
goto END-rtas

:END-rtas

msbuild IPlugEffect-rtas.sln /p:configuration=release

REM - Make Installer (InnoSetup)

if exist "%ProgramFiles(x86)%" (goto 64-Bit-is) else (goto 32-Bit-is)

:32-Bit-is
REM - "%ProgramFiles%\Inno Setup 5\iscc" /cc ".\installer\IPlugEffect.iss"
goto END-is

:64-Bit-is
REM - "%ProgramFiles(x86)%\Inno Setup 5\iscc" /cc ".\installer\IPlugEffect.iss"
goto END-is

:END-is

REM - ZIP
"%ProgramFiles%\7-Zip\7z.exe" a .\installer\IPlugEffect-win-32bit.zip .\build-win-app\win32\bin\IPlugEffect.exe .\build-win-vst3\win32\bin\IPlugEffect.vst3 .\build-win-vst2\win32\bin\IPlugEffect.dll .\build-win-rtas\bin\IPlugEffect.dpm .\build-win-rtas\bin\IPlugEffect.dpm.rsr .\installer\license.rtf .\installer\readmewin.rtf
REM - "%ProgramFiles%\7-Zip\7z.exe" a .\installer\IPlugEffect-win-64bit.zip .\build-win-app\x64\bin\IPlugEffect.exe .\build-win-vst3\x64\bin\IPlugEffect.vst3 .\build-win-vst2\x64\bin\IPlugEffect.dll .\installer\license.rtf .\installer\readmewin.rtf

echo off
pause