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
  if %FORMAT% == ".aaxplugin" (
   REM -DEL /Q /F /S %AAX_BUNDLE%
   REM -CALL ..\..\AAX_SDK\Utilities\CreatePackage.bat %OUT_DIR% %ICON_SOURCE%
  )
)

if %PLATFORM% == "x64" (
  if exist "%ProgramFiles(x86)%" (
    if %FORMAT% == ".aaxplugin" (
      DEL /Q /F /S %AAX_BUNDLE%
      CALL ..\..\AAX_SDK\Utilities\CreatePackage.bat %OUT_DIR% %ICON_SOURCE%
    )
  )
)