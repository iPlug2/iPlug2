call "%ProgramFiles(x86)%\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x86_x64

set LIBPATH=%CD:\=\\%\\..\\Build\\win\\x64\\Debug

cd ..\Build\src\resvg
cargo build --features="cairo-backend"

cd capi
if exist .cargo rmdir /q /s .cargo
mkdir .cargo
echo [build] >> .cargo/config
echo rustflags = ["-C", "link-arg=/LIBPATH:%LIBPATH%", "-C", "link-arg=pixman.lib", "-C", "link-arg=freetype.lib", "-C", "link-arg=zlib.lib", "-C", "link-arg=Msimg32.lib", "-C", "link-arg=gdi32.lib", "-C", "link-arg=User32.lib", "-C", "target-feature=+crt-static"] >> .cargo/config
cargo build --features="cairo-backend"
cd .. 

copy capi\include\resvg.h ..\resvg.h 
copy target\debug\resvg.lib ..\..\win\x64\Debug

cd ..\..\..\IGraphics

REM ---------------------------------------------------------

set LIBPATH=%CD:\=\\%\\..\\Build\\win\\x64\\Release

cd ..\Build\src\resvg
cargo build --release --features="cairo-backend"

cd capi
if exist .cargo rmdir /q /s .cargo
mkdir .cargo
echo [build] >> .cargo/config
echo rustflags = ["-C", "link-arg=/LIBPATH:%LIBPATH%", "-C", "link-arg=pixman.lib", "-C", "link-arg=freetype.lib", "-C", "link-arg=zlib.lib", "-C", "link-arg=Msimg32.lib", "-C", "link-arg=gdi32.lib", "-C", "link-arg=User32.lib", "-C", "target-feature=+crt-static"] >> .cargo/config
cargo build --release --features="cairo-backend"
cd .. 

copy capi\include\resvg.h ..\resvg.h 
copy target\release\resvg.lib ..\..\win\x64\Release

cd ..\..\..\IGraphics

