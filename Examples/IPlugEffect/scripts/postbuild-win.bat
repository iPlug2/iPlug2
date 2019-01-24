@echo off
REM - CALL "$(SolutionDir)\scripts\postbuild.bat" "$(TargetExt)" "$(BINARY_NAME)" "$(Platform)" "$(COPY_VST2)" "$(TargetPath)" "$(VST2_32_PATH)" "$(VST2_64_PATH)" "$(VST3_32_PATH)" "$(VST3_64_PATH)" "$(VST_BUNDLE)" "$(AAX_32_PATH)" "$(AAX_64_PATH)" "$(AAX_BUNDLE)"
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
shift
shift
set VST_BUNDLE=%5
set AAX_32_PATH=%6
set AAX_64_PATH=%7
set AAX_BUNDLE=%8
set PDB_FILE=%9

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
echo VST_BUNDLE %VST_BUNDLE%
echo AAX_BUNDLE %AAX_BUNDLE%
echo PDB_FILE %PDB_FILE%
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
    echo copying 32bit binary to VST3 BUNDLE .. 
    copy /y %BUILT_BINARY% %VST_BUNDLE%\Contents\x86-win
    copy /y %PDB_FILE% %VST_BUNDLE%\Contents\x86-win
    echo copying VST3 bundle to 32bit VST3 Plugins folder ... 
    xcopy /E /H /Y %VST_BUNDLE%\* %VST3_32_PATH%\%NAME%.vst3\
  )
  if %FORMAT% == ".aaxplugin" (
    echo copying 32bit binary to AAX BUNDLE .. 
    copy /y %BUILT_BINARY% %AAX_BUNDLE%\Contents\Win32
    copy /y %PDB_FILE% %AAX_BUNDLE%\Contents\Win32
    echo copying 32bit binary to 32bit AAX Plugins folder ... 
    xcopy /E /H /Y %AAX_BUNDLE%\* %AAX_32_PATH%\%NAME%.aaxplugin\
  )
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
      echo copying 64bit binary to VST3 BUNDLE .. 
      copy /y %BUILT_BINARY% %VST_BUNDLE%\Contents\x86_64-win
      copy /y %PDB_FILE% %VST_BUNDLE%\Contents\x86_64-win
      echo copying bundle to 64bit VST3 Plugins folder ... 
      xcopy /E /H /Y %VST_BUNDLE%\* %VST3_64_PATH%\%NAME%.vst3\
    )
    if %FORMAT% == ".aaxplugin" (
      echo copying 64bit binary to AAX BUNDLE .. 
      copy /y %BUILT_BINARY% %AAX_BUNDLE%\Contents\x64
      copy /y %PDB_FILE% %AAX_BUNDLE%\Contents\x64
      echo copying 64bit binary to 64bit AAX Plugins folder ... 
      xcopy /E /H /Y %AAX_BUNDLE%\* %AAX_64_PATH%\%NAME%.aaxplugin\
    )
  ) else (
    echo not copying 64bit, since machine is 32bit
  )
)