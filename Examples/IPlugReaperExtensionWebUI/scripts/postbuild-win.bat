@echo off

REM - CALL "$(SolutionDir)scripts\postbuild-win.bat" "$(BINARY_NAME)" "$(Platform)" "$(TargetPath)" "$(REAPER_EXT_PATH)" "$(SolutionDir)"

set BINARY_NAME=%~1
set PLATFORM=%~2
set BUILT_BINARY=%~3
set REAPER_EXT_PATH=%~4
set PROJECT_DIR=%~5

REM Must match SHARED_RESOURCES_SUBPATH in config.h (where the extension loads its web UI from at runtime).
set WEB_SUBPATH=IPlugReaperExtensionWebUI

echo POSTBUILD SCRIPT VARIABLES -----------------------------------------------------
echo BINARY_NAME %BINARY_NAME%
echo PLATFORM %PLATFORM%
echo BUILT_BINARY %BUILT_BINARY%
echo REAPER_EXT_PATH %REAPER_EXT_PATH%
echo PROJECT_DIR %PROJECT_DIR%
echo END POSTBUILD SCRIPT VARIABLES -----------------------------------------------------

if not exist "%REAPER_EXT_PATH%" (
  echo creating REAPER UserPlugins directory: %REAPER_EXT_PATH%
  mkdir "%REAPER_EXT_PATH%"
)

echo copying REAPER extension to: %REAPER_EXT_PATH%
copy /y "%BUILT_BINARY%" "%REAPER_EXT_PATH%"

REM Deploy the WebView UI so Release builds (which can't load from the source tree) find index.html.
echo deploying web resources to: %REAPER_EXT_PATH%\%WEB_SUBPATH%
xcopy /y /i /e "%PROJECT_DIR%resources\web" "%REAPER_EXT_PATH%\%WEB_SUBPATH%"
