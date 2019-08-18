@echo off
REM - CALL "$(SolutionDir)\scripts\postbuild.bat" "$(BINARY_NAME)" "$(Platform)" "$(TargetPath)" "$(REAPER_32_PATH)" "$(REAPER_64_PATH)"
set NAME=%1
set PLATFORM=%2
set BUILT_BINARY=%3
set REAPER_32_PATH=%4
set REAPER_64_PATH=%4

echo BUILT_BINARY %BUILT_BINARY% 
echo PLATFORM %PLATFORM% 
echo REAPER_32_PATH %REAPER_32_PATH% 
echo REAPER_64_PATH %REAPER_64_PATH% 

if %PLATFORM% == "Win32" (
  copy /y %BUILT_BINARY% %REAPER_32_PATH%
)

if %PLATFORM% == "x64" (
  if exist "%ProgramFiles(x86)%" (
    copy /y %BUILT_BINARY% %REAPER_64_PATH%
  )
)