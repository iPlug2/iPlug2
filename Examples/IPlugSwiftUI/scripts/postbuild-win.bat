@echo off

REM - CALL "$(SolutionDir)scripts\postbuild-win.bat" "$(TargetExt)" "$(BINARY_NAME)" "$(Platform)" "$(COPY_VST2)" "$(TargetPath)" "$(VST2_ARM64EC_PATH)" "$(VST2_x86_64_PATH)" "$(VST3_ARM64EC_PATH)" "$(VST3_x86_64_PATH)" "$(AAX_ARM64EC_PATH)" "$(AAX_x86_64_PATH)" "$(CLAP_PATH)" "$(BUILD_DIR)" "$(VST_ICON)" "$(AAX_ICON)" "$(CREATE_BUNDLE_SCRIPT)" "$(ICUDAT_PATH)"

set FORMAT=%1
set NAME=%2
set PLATFORM=%3
set COPY_VST2=%4
set BUILT_BINARY=%5
set VST2_ARM64EC_PATH=%6
set VST2_x86_64_PATH=%7 
set VST3_ARM64EC_PATH=%8
set VST3_x86_64_PATH=%9
shift
shift 
shift
shift
shift 
shift
shift
set AAX_ARM64EC_PATH=%3
set AAX_x86_64_PATH=%4
set CLAP_PATH=%5
set BUILD_DIR=%6
set VST_ICON=%7
set AAX_ICON=%8
set CREATE_BUNDLE_SCRIPT=%9
shift
set ICUDAT_PATH=%9

echo POSTBUILD SCRIPT VARIABLES -----------------------------------------------------
echo FORMAT %FORMAT% 
echo NAME %NAME% 
echo PLATFORM %PLATFORM% 
echo COPY_VST2 %COPY_VST2% 
echo BUILT_BINARY %BUILT_BINARY% 
echo VST2_ARM64EC_PATH %VST2_ARM64EC_PATH% 
echo VST2_x86_64_PATH %VST2_x86_64_PATH% 
echo VST3_ARM64EC_PATH %VST3_ARM64EC_PATH% 
echo VST3_x86_64_PATH %VST3_x86_64_PATH% 
echo CLAP_PATH %CLAP_PATH% 
echo BUILD_DIR %BUILD_DIR%
echo VST_ICON %VST_ICON% 
echo AAX_ICON %AAX_ICON% 
echo CREATE_BUNDLE_SCRIPT %CREATE_BUNDLE_SCRIPT%
echo ICUDAT_PATH %ICUDAT_PATH%
echo END POSTBUILD SCRIPT VARIABLES -----------------------------------------------------

if %PLATFORM% == "ARM64EC" (
  if exist "%ICUDAT_PATH%" (
    echo copying icudtl.dat file next to built binary: %BUILT_BINARY%
    for %%F in (%BUILT_BINARY%) do (
      copy /y %ICUDAT_PATH% "%%~dpF"
    )
  ) else (
    echo icudtl.dat not found at %ICUDAT_PATH%, skipping...
  )

  if %FORMAT% == ".exe" (
    echo copying exe to build dir: %BUILD_DIR%\%NAME%_%PLATFORM%.exe
    copy /y %BUILT_BINARY% %BUILD_DIR%\%NAME%_%PLATFORM%.exe
    if exist "%ICUDAT_PATH%" (
      echo copying dat file to build dir: %BUILD_DIR%
      copy /y %ICUDAT_PATH% %BUILD_DIR%
    )
  )

  if %FORMAT% == ".dll" (
    copy /y %BUILT_BINARY% %BUILD_DIR%\%NAME%_%PLATFORM%.dll
    if exist "%ICUDAT_PATH%" (
      copy /y %ICUDAT_PATH% %BUILD_DIR%
    )
  )
  
  if %FORMAT% == ".dll" (
    if %COPY_VST2% == "1" (
      echo copying ARM64EC binary to ARM64EC VST2 Plugins folder ... 
      copy /y %BUILT_BINARY% %VST2_ARM64EC_PATH%
      if exist "%ICUDAT_PATH%" (
        copy /y %ICUDAT_PATH% %VST2_ARM64EC_PATH%
      )
    ) else (
      echo not copying ARM64EC VST2 binary
    )
  )
  
  if %FORMAT% == ".vst3" (
    echo copying ARM64EC binary to VST3 BUNDLE ..
    call %CREATE_BUNDLE_SCRIPT% %BUILD_DIR%\%NAME%.vst3 %VST_ICON% %FORMAT%
    copy /y %BUILT_BINARY% %BUILD_DIR%\%NAME%.vst3\Contents\arm64ec-win
    if exist "%ICUDAT_PATH%" (
      copy /y %ICUDAT_PATH% %BUILD_DIR%\%NAME%.vst3\Contents\arm64ec-win
    )
    if exist %VST3_ARM64EC_PATH% ( 
      echo copying VST3 bundle to ARM64EC VST3 Plugins folder ...
      call %CREATE_BUNDLE_SCRIPT% %VST3_ARM64EC_PATH%\%NAME%.vst3 %VST_ICON% %FORMAT%
      xcopy /E /H /Y %BUILD_DIR%\%NAME%.vst3\Contents\*  %VST3_ARM64EC_PATH%\%NAME%.vst3\Contents\
    )
  )
  
  if %FORMAT% == ".aaxplugin" (
    echo copying ARM64EC binary to AAX BUNDLE ..
    call %CREATE_BUNDLE_SCRIPT% %BUILD_DIR%\%NAME%.aaxplugin %AAX_ICON% %FORMAT%
    copy /y %BUILT_BINARY% %BUILD_DIR%\%NAME%.aaxplugin\Contents\Arm64ec
    if exist "%ICUDAT_PATH%" (
      copy /y %ICUDAT_PATH% %BUILD_DIR%\%NAME%.aaxplugin\Contents\Arm64ec
    )
    echo copying ARM64EC bundle to ARM64EC AAX Plugins folder ... 
    call %CREATE_BUNDLE_SCRIPT% %BUILD_DIR%\%NAME%.aaxplugin %AAX_ICON% %FORMAT%
    xcopy /E /H /Y %BUILD_DIR%\%NAME%.aaxplugin\Contents\* %AAX_ARM64EC_PATH%\%NAME%.aaxplugin\Contents\
  )
)

if %PLATFORM% == "x64" (
  
  if exist "%ICUDAT_PATH%" (
    echo copying icudtl.dat file next to built binary: %BUILT_BINARY%
    for %%F in (%BUILT_BINARY%) do (
      copy /y %ICUDAT_PATH% "%%~dpF"
    )
  ) else (
    echo icudtl.dat not found at %ICUDAT_PATH%, skipping...
  )

  if not exist "%ProgramFiles(x86)%" (
    echo "This batch script fails on 32 bit windows... edit accordingly"
  )

  if %FORMAT% == ".exe" (
    echo copying exe to build dir: %BUILD_DIR%\%NAME%_%PLATFORM%.exe
    copy /y %BUILT_BINARY% %BUILD_DIR%\%NAME%_%PLATFORM%.exe
    if exist "%ICUDAT_PATH%" (
      echo copying dat file to build dir: %BUILD_DIR%
      copy /y %ICUDAT_PATH% %BUILD_DIR%
    )
  )

  if %FORMAT% == ".dll" (
    copy /y %BUILT_BINARY% %BUILD_DIR%\%NAME%_%PLATFORM%.dll
    if exist "%ICUDAT_PATH%" (
      copy /y %ICUDAT_PATH% %BUILD_DIR%
    )
  )
  
  if %FORMAT% == ".dll" (
    if %COPY_VST2% == "1" (
      echo copying 64bit binary to 64bit VST2 Plugins folder ... 
      copy /y %BUILT_BINARY% %VST2_x86_64_PATH%
      if exist "%ICUDAT_PATH%" (
        copy /y %ICUDAT_PATH% %VST2_x86_64_PATH%
      )
    ) else (
      echo not copying 64bit VST2 binary
    )
  )
  
  if %FORMAT% == ".vst3" (
    echo copying 64bit binary to VST3 BUNDLE ...
    call %CREATE_BUNDLE_SCRIPT% %BUILD_DIR%\%NAME%.vst3 %VST_ICON% %FORMAT%
    copy /y %BUILT_BINARY% %BUILD_DIR%\%NAME%.vst3\Contents\x86_64-win
    if exist "%ICUDAT_PATH%" (
      copy /y %ICUDAT_PATH% %BUILD_DIR%\%NAME%.vst3\Contents\x86_64-win
    )
    if exist %VST3_x86_64_PATH% (
      echo copying VST3 bundle to 64bit VST3 Plugins folder ...
      call %CREATE_BUNDLE_SCRIPT% %VST3_x86_64_PATH%\%NAME%.vst3 %VST_ICON% %FORMAT%
      xcopy /E /H /Y %BUILD_DIR%\%NAME%.vst3\Contents\*  %VST3_x86_64_PATH%\%NAME%.vst3\Contents\
    )
  )
  
  if %FORMAT% == ".aaxplugin" (
    echo copying 64bit binary to AAX BUNDLE ...
    call %CREATE_BUNDLE_SCRIPT% %BUILD_DIR%\%NAME%.aaxplugin %AAX_ICON% %FORMAT%
    copy /y %BUILT_BINARY% %BUILD_DIR%\%NAME%.aaxplugin\Contents\x64
    if exist "%ICUDAT_PATH%" (
      copy /y %ICUDAT_PATH% %BUILD_DIR%\%NAME%.aaxplugin\Contents\x64
    )
    echo copying 64bit bundle to 64bit AAX Plugins folder ... 
    call %CREATE_BUNDLE_SCRIPT% %BUILD_DIR%\%NAME%.aaxplugin %AAX_ICON% %FORMAT%
    xcopy /E /H /Y %BUILD_DIR%\%NAME%.aaxplugin\Contents\* %AAX_x86_64_PATH%\%NAME%.aaxplugin\Contents\
  )
  
  if %FORMAT% == ".clap" (
    echo copying binary to CLAP Plugins folder ... 
    copy /y %BUILT_BINARY% %CLAP_PATH%
    if exist "%ICUDAT_PATH%" (
      copy /y %ICUDAT_PATH% %CLAP_PATH%
    )
  )
)