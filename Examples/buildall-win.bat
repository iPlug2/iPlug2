if exist "%ProgramFiles(Arm)%" (goto ARM64) else (goto x86_64)

:ARM64
echo ARM64 O/S detected
call "%ProgramFiles%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" arm64 8.1
goto END

:x86_64
echo x86_64 Host O/S detected
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x86_x64 8.1
goto END
:END

if exist build_errors.log del build_errors.log

for /r %%X in (/*.sln) do (
echo Building %%X
msbuild "%%X" /p:configuration=release /p:platform=ARM64EC /nologo /noconsolelogger /fileLogger /v:quiet /flp:logfile=build_errors.log;errorsonly;append
msbuild "%%X" /p:configuration=release /p:platform=x64 /nologo /noconsolelogger /fileLogger /v:quiet /flp:logfile=build_errors.log;errorsonly;append
)

echo ------------------------------------------------------------------
echo Printing log file to console...

type build_errors.log

pause