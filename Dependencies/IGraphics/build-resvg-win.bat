echo off

if exist "%ProgramFiles(x86)%" (goto 64-Bit) else (goto 32-Bit)

:32-Bit
echo 32-Bit O/S detected
call "%ProgramFiles%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x86_x64
goto END

:64-Bit
echo 64-Bit Host O/S detected
call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x86_x64
goto END
:END


cd ..\Build\src\resvg
cargo build --release --features="cairo-backend"

cd capi
mkdir .cargo
echo [build] >> .cargo/config
echo rustflags = ["-C", "link-arg=/LIBPATH:%cd%\\..\\..\\..\\Build\\win\\x64\\Release", "-C", "link-arg=pixman.lib", "-C", "link-arg=freetype.lib", "-C", "link-arg=Msimg32.lib", "-C", "link-arg=gdi32.lib", "-C", "link-arg=User32.lib"] >> .cargo/config
cargo build --release --features="cairo-backend"
cd .. 

copy capi\include\resvg.h ..\..\win\include\resvg.h 
copy target\release\libresvg.rlib ..\..\win\x64\Release