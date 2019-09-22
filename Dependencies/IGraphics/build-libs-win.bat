echo off

echo 64-Bit Host O/S detected
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x86_x64

echo Building 32 bit binaries...
REM - msbuild IGraphicsLibraries.sln /p:configuration=Debug /p:platform=win32 /nologo /verbosity:minimal /fileLogger /m /flp:logfile=build-win.log;errorsonly 
REM - msbuild IGraphicsLibraries.sln /p:configuration=Release /p:platform=win32 /nologo /verbosity:minimal /fileLogger /m /flp:logfile=build-win.log;errorsonly 

echo Building 64 bit binaries...
msbuild IGraphicsLibraries.sln /p:configuration=Debug /p:platform=x64 /nologo /verbosity:minimal /fileLogger /m /flp:logfile=build-win.log;errorsonly;append
msbuild IGraphicsLibraries.sln /p:configuration=Release /p:platform=x64 /nologo /verbosity:minimal /fileLogger /m /flp:logfile=build-win.log;errorsonly;append

