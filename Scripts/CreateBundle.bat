REM ICON SETTER Adapted for VST3 and AAX based on CreatePackage.bat Copyright 2017 by Avid Technology, Inc.
SET OutDir="%~fn1"
SET IconSource="%~fn2"
SET Format=%3

echo Create AAX/VST3 Package Directories

echo %IconSource%

if %Format% == ".vst3" (
SET X86=x86-win
SET X86_64=x86_64-win
) else (
SET X86=Win32
SET X86_64=x64
)

IF EXIST %OutDir% GOTO OUTDIR_EXISTS
mkdir %OutDir%
:OUTDIR_EXISTS

IF EXIST %OutDir%\..\%X86% GOTO Win32_EXISTS
mkdir %OutDir%\..\%X86%
:Win32_EXISTS

IF EXIST %OutDir%\..\%X86_64% GOTO X64_EXISTS
mkdir %OutDir%\..\%X86_64%
:X64_EXISTS

IF EXIST %OutDir%\..\Resources GOTO RESOURCES_EXISTS
mkdir %OutDir%\..\Resources
:RESOURCES_EXISTS

echo Set Folder Icon

IF EXIST %OutDir%\..\..\PlugIn.ico GOTO ICON_EXISTS
copy /Y %IconSource% %OutDir%\..\..\PlugIn.ico > NUL
:ICON_EXISTS

attrib -r %OutDir%\..\..
attrib -h -r -s %OutDir%\..\..\desktop.ini
echo [.ShellClassInfo] > %OutDir%\..\..\desktop.ini 
echo IconResource=PlugIn.ico,0 >> %OutDir%\..\..\desktop.ini 
echo ;For compatibility with Windows XP >> %OutDir%\..\..\desktop.ini 
echo IconFile=PlugIn.ico >> %OutDir%\..\..\desktop.ini 
echo IconIndex=0 >> %OutDir%\..\..\desktop.ini 
attrib +h +r +s %OutDir%\..\..\PlugIn.ico
attrib +h +r +s %OutDir%\..\..\desktop.ini
attrib +r %OutDir%\..\..


