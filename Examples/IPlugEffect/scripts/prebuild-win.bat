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

if %FORMAT% == ".vst3" (
 REM DEL /Q /F /S %VST_BUNDLE%
  CALL ..\..\..\Scripts\CreateBundle.bat %VST_BUNDLE% %VST_ICON% %FORMAT%
)

if %FORMAT% == ".aaxplugin" (
 REM DEL /Q /F /S %AAX_BUNDLE%
  CALL ..\..\..\Scripts\CreateBundle.bat %AAX_BUNDLE% %AAX_ICON% %FORMAT%
)
