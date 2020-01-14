echo off

call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x86_x64

set BACKENDS="skia-backend"

REM set LIBPATH=%CD:\=\\%\\..\\Build\\win\\x64\\Debug
set SKIA_DIR=%CD:\=\\%\\..\\Build\\src\\skia
REM set SKIA_LIB_DIR=%LIBPATH%

REM cd ..\Build\src\resvg
REM cargo build --features=%BACKENDS%

REM cd capi
REM if exist .cargo rmdir /q /s .cargo
REM mkdir .cargo
REM echo [build] >> .cargo/config
REM echo rustflags = ["-C", "link-arg=/LIBPATH:%LIBPATH%", "-C", "link-arg=pixman.lib", "-C", "link-arg=freetype.lib", "-C", "link-arg=zlib.lib", "-C", "link-arg=Msimg32.lib", "-C", "link-arg=gdi32.lib", "-C", "link-arg=User32.lib", "-C", "target-feature=+crt-static"] >> .cargo/config
REM cargo build --features=%BACKENDS%
REM cd .. 

REM copy target\debug\resvg.lib ..\..\win\x64\Debug

REM cd ..\..\..\IGraphics

REM ---------------------------------------------------------

set LIBPATH=%CD:\=\\%\\..\\Build\\win\\x64\\Release
set SKIA_LIB_DIR=%LIBPATH%

cd ..\Build\src\resvg
cargo build --release --features=%BACKENDS%

cd capi
if exist .cargo rmdir /q /s .cargo
mkdir .cargo
echo [build] >> .cargo/config
REM "link-arg=pixman.lib", "-C", "link-arg=freetype.lib", "-C", "link-arg=zlib.lib", "-C", "link-arg=Msimg32.lib", "-C", "link-arg=gdi32.lib", "-C", "link-arg=User32.lib",
echo rustflags = ["-C", "link-arg=/LIBPATH:%LIBPATH%", "-C", "target-feature=+crt-static"] >> .cargo/config
REM echo rustflags = ["-C", "link-arg=/LIBPATH:%LIBPATH%", "-C", "link-arg=pixman.lib", "-C", "link-arg=freetype.lib", "-C", "link-arg=zlib.lib", "-C", "link-arg=Msimg32.lib", "-C", "link-arg=gdi32.lib", "-C", "link-arg=User32.lib", "-C", "target-feature=+crt-static"] >> .cargo/config
cargo build --release --features=%BACKENDS%
cd .. 

copy target\release\resvg.lib ..\..\win\x64\Release

cd ..\..\..\IGraphics

