@echo off
REM - CALL "$(ProjectDir)\scripts\postbuild.bat" "$(TargetExt)" "$(BINARY_NAME)" "$(Platform)" "$(COPY_VST2)" "$(TargetPath)" "$(VST2_32_PATH)" "$(VST2_64_PATH)" "$(VST3_32_PATH)" "$(VST3_64_PATH)" "$(AAX_32_PATH)" "$(AAX_64_PATH)"
set FORMAT=%1
set NAME=%2
set PLATFORM=%3
set COPY_VST2=%4
set BUILT_BINARY=%5
set VST2_32_PATH=%6
set VST2_64_PATH=%7 

echo POSTBUILD SCRIPT VARIABLES -----------------------------------------------------
echo FORMAT %FORMAT% 
echo NAME %NAME% 
echo PLATFORM %PLATFORM% 
echo COPY_VST2 %COPY_VST2% 
echo BUILT_BINARY %BUILT_BINARY% 
echo VST2_32_PATH %VST2_32_PATH% 
echo VST2_64_PATH %VST2_64_PATH% 
echo END POSTBUILD SCRIPT VARIABLES -----------------------------------------------------

if %PLATFORM% == "Win32" (
  if %FORMAT% == ".dll" (
    if %COPY_VST2% == "1" (
      echo copying 32bit binary to 32bit VST2 Plugins folder ... 
      copy /y %BUILT_BINARY% %VST2_32_PATH%
      GOTO :EOF
    ) else (
      echo not copying 32bit VST2 binary
      GOTO :EOF
    )
  )
)

if %PLATFORM% == "x64" (
  if exist "%ProgramFiles(x86)%" (
    if %FORMAT% == ".dll" (
      if %COPY_VST2% == "1" (
        echo copying 64bit binary to 64bit VST2 Plugins folder ... 
        copy /y %BUILT_BINARY% %VST2_64_PATH%
        GOTO :EOF
      ) else (
        echo not copying 64bit VST2 binary
        GOTO :EOF
      )
    )
  ) else (
    echo not copying 64bit, since machine is 32bit
    GOTO :EOF
  )
)

exit /b 0