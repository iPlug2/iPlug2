REM ICON SETTER Adapted for VST3 and AAX based on CreatePackage.bat Copyright 2017 by Avid Technology, Inc.
SET BundleDir="%~fn1"
SET IconSource="%~fn2"
SET Format=%3

echo Create AAX/VST3 Package Directories

if %Format% == ".vst3" (
SET X86=x86-win
SET X86_64=x86_64-win
) else (
SET X86=Win32
SET X86_64=x64
)

IF EXIST %BundleDir% GOTO BUNDLEDIR_EXISTS
mkdir %BundleDir%
:BUNDLEDIR_EXISTS

IF EXIST %BundleDir%\Contents\%X86% GOTO Win32_EXISTS
mkdir %BundleDir%\Contents\%X86%
:Win32_EXISTS

IF EXIST %BundleDir%\Contents\%X86_64% GOTO X64_EXISTS
mkdir %BundleDir%\Contents\%X86_64%
:X64_EXISTS

IF EXIST %BundleDir%\Contents\Resources GOTO RESOURCES_EXISTS
mkdir %BundleDir%\Contents\Resources
:RESOURCES_EXISTS

echo Set Folder Icon

IF EXIST %BundleDir%\PlugIn.ico GOTO ICON_EXISTS
copy /Y %IconSource% %BundleDir%\PlugIn.ico > NUL
:ICON_EXISTS

IF EXIST %BundleDir%\desktop.ini GOTO DESKTOP_INI_EXISTS
echo "" >%BundleDir%\desktop.ini
:DESKTOP_INI_EXISTS

attrib /D -r %BundleDir%
attrib -h -r -s %BundleDir%\desktop.ini
echo [.ShellClassInfo] > %BundleDir%\desktop.ini 
echo IconResource=PlugIn.ico,0 >> %BundleDir%\desktop.ini 
echo ;For compatibility with Windows XP >> %BundleDir%\desktop.ini 
echo IconFile=PlugIn.ico >> %BundleDir%\desktop.ini 
echo IconIndex=0 >> %BundleDir%\desktop.ini 
attrib +h +r +s %BundleDir%\PlugIn.ico
attrib +h +r +s %BundleDir%\desktop.ini
attrib +r %BundleDir%
