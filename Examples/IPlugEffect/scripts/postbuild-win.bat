@echo off
REM - CALL "$(SolutionDir)\scripts\postbuild.bat" "$(TargetExt)" "$(BINARY_NAME)" "$(Platform)" "$(COPY_VST2)" "$(TargetPath)" "$(VST2_32_PATH)" "$(VST2_64_PATH)" "$(VST3_32_PATH)" "$(VST3_64_PATH)" "$(AAX_32_PATH)" "$(AAX_64_PATH)"
set FORMAT=%1
set NAME=%2
set PLATFORM=%3
set COPY_VST2=%4
set BUILT_BINARY=%5
set VST2_32_PATH=%6
set VST2_64_PATH=%7 
set VST3_32_PATH=%8
set VST3_64_PATH=%9
shift
shift 
shift
set AAX_32_PATH=%7
set AAX_64_PATH=%8
set AAX_BUNDLE=%9

echo POSTBUILD SCRIPT VARIABLES -----------------------------------------------------
echo FORMAT %FORMAT% 
echo NAME %NAME% 
echo PLATFORM %PLATFORM% 
echo COPY_VST2 %COPY_VST2% 
echo BUILT_BINARY %BUILT_BINARY% 
echo VST2_32_PATH %VST2_32_PATH% 
echo VST2_64_PATH %VST2_64_PATH% 
echo VST3_32_PATH %VST3_32_PATH% 
echo VST3_64_PATH %VST3_64_PATH% 
echo END POSTBUILD SCRIPT VARIABLES -----------------------------------------------------

if %PLATFORM% == "Win32" (
  if %FORMAT% == ".dll" (
    if %COPY_VST2% == "1" (
      echo copying 32bit binary to 32bit VST2 Plugins folder ... 
      copy /y %BUILT_BINARY% %VST2_32_PATH%
    ) else (
      echo not copying 32bit VST2 binary
    )
  )
  if %FORMAT% == ".vst3" (
    echo copying 32bit binary to 32bit VST3 Plugins folder ... 
    copy /y %BUILT_BINARY% %VST3_32_PATH%
  )
REM -  if %FORMAT% == ".aaxplugin" (
REM -    echo copying 32bit binary to 32bit AAX Plugins folder ... 
REM -    echo %AAX_BUNDLE% %AAX_32_PATH%
REM -    xcopy /E /H /Y %AAX_BUNDLE%\* %AAX_32_PATH%\%NAME%.aaxplugin\
REM -  )
)

if %PLATFORM% == "x64" (
  if exist "%ProgramFiles(x86)%" (
    if %FORMAT% == ".dll" (
      if %COPY_VST2% == "1" (
        echo copying 64bit binary to 64bit VST2 Plugins folder ... 
        copy /y %BUILT_BINARY% %VST2_64_PATH%
      ) else (
        echo not copying 64bit VST2 binary
      )
    )
    if %FORMAT% == ".vst3" (
      echo copying 64bit binary to 64bit VST3 Plugins folder ... 
      copy /y %BUILT_BINARY% %VST3_64_PATH%
    )
    if %FORMAT% == ".aaxplugin" (
      echo copying 64bit binary to 64bit AAX Plugins folder ... 
    echo %AAX_BUNDLE% %AAX_64_PATH%
    xcopy /E /H /Y %AAX_BUNDLE%\* %AAX_64_PATH%\%NAME%.aaxplugin\
    )
  ) else (
    echo not copying 64bit, since machine is 32bit
  )
)