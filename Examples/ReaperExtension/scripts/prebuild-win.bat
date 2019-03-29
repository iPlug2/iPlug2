@echo off
REM - CALL "$(ProjectDir)\scripts\prebuild.bat" "$(TargetExt)" "$(BINARY_NAME)" "$(Platform)" "$(TargetPath)" "$(OutDir)"
set FORMAT=%1
set NAME=%2
set PLATFORM=%3
set BUILT_BINARY=%4
set OUT_DIR=%5
set OUT_DIR=%OUT_DIR:"=%
set ICON_SOURCE=%6
set AAX_BUNDLE=%7

if %PLATFORM% == "Win32" (
)

if %PLATFORM% == "x64" (
)