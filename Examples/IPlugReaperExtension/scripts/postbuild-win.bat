@echo off

REM - CALL "$(SolutionDir)scripts\postbuild-win.bat" "$(BINARY_NAME)" "$(Platform)" "$(TargetPath)" "$(REAPER_EXT_PATH)"

set BINARY_NAME=%~1
set PLATFORM=%~2
set BUILT_BINARY=%~3
set REAPER_EXT_PATH=%~4

echo POSTBUILD SCRIPT VARIABLES -----------------------------------------------------
echo BINARY_NAME %BINARY_NAME%
echo PLATFORM %PLATFORM%
echo BUILT_BINARY %BUILT_BINARY%
echo REAPER_EXT_PATH %REAPER_EXT_PATH%
echo END POSTBUILD SCRIPT VARIABLES -----------------------------------------------------

if not exist "%REAPER_EXT_PATH%" (
  echo creating REAPER UserPlugins directory: %REAPER_EXT_PATH%
  mkdir "%REAPER_EXT_PATH%"
)

echo copying REAPER extension to: %REAPER_EXT_PATH%
copy /y "%BUILT_BINARY%" "%REAPER_EXT_PATH%"
