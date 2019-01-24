@echo off
REM - CALL "$(SolutionDir)scripts\prebuild-win.bat" "$(TargetExt)" "$(BINARY_NAME)" "$(Platform)" "$(TargetPath)" "$(OutDir)" "$(VST_ICON)" "$(VST_BUNDLE)" "$(AAX_ICON)" "$(AAX_BUNDLE)"
set FORMAT=%1
set NAME=%2
set PLATFORM=%3
set BUILT_BINARY=%4
set OUT_DIR=%5
set OUT_DIR=%OUT_DIR:"=%
set VST_ICON=%6
set VST_BUNDLE=%7
set AAX_ICON=%8
set AAX_BUNDLE=%9

echo PREBUILD SCRIPT VARIABLES -----------------------------------------------------
echo FORMAT %FORMAT% 
echo NAME %NAME% 
echo PLATFORM %PLATFORM% 
echo BUILT_BINARY %BUILT_BINARY% 
echo VST_ICON %VST_ICON%
echo VST_BUNDLE %VST_BUNDLE%
echo AAX_ICON %AAX_ICON%
echo AAX_BUNDLE %AAX_BUNDLE%
echo END PREBUILD SCRIPT VARIABLES -----------------------------------------------------

REM - create bundle on whichever arch gets compiled first!
REM if %PLATFORM% == "x64" (
  REM if exist "%ProgramFiles(x86)%" (
    if %FORMAT% == ".vst3" (
      DEL /Q /F /S %VST_BUNDLE%
      CALL ..\..\..\Scripts\CreateBundle.bat "%OUT_DIR%" %VST_ICON% %FORMAT%
    ) else (
      if %FORMAT% == ".aaxplugin" (
        DEL /Q /F /S %AAX_BUNDLE%
        CALL ..\..\..\Scripts\CreateBundle.bat "%OUT_DIR%" %AAX_ICON% %FORMAT%
      )
    )
  REM )
REM )