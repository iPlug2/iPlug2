#should be executed in Extras/faust
$DEPS_DIR=$pwd
$BUILD_DIR="$pwd\..\..\Build"
$SRC_DIR="$BUILD_DIR\src"
$TMP_DIR="$BUILD_DIR\tmp"
$LLVM_REPO_DIR="$TMP_DIR\llvm"
$FAUST_REPO_DIR="$TMP_DIR\faust"
$LLVM_CMAKE_BUILD_DIR="$TMP_DIR\llvm-cmake"
$FAUST_CMAKE_BUILD_DIR="$TMP_DIR\faust-cmake"
$INSTALL_DIR="$BUILD_DIR\win\x64\Debug"
$LLVM_VER="llvmorg-9.0.1"
$LLVM_DIR=C:\Users\oli\Dev\iPlug2\Dependencies\Build\tmp\llvm-cmake\lib\cmake\llvm
if(![System.IO.File]::Exists($LLVM_REPO_DIR)) {
    git clone --config core.autocrlf=false https://github.com/llvm/llvm-project.git $LLVM_REPO_DIR
    git checkout $LLVM_VER
}

if(![System.IO.File]::Exists($FAUST_REPO_DIR)) {
    git clone https://github.com/grame-cncm/faust.git $FAUST_REPO_DIR
}

mkdir $LLVM_CMAKE_BUILD_DIR
Set-Location $LLVM_CMAKE_BUILD_DIR

cmake -Thost=x64 $LLVM_REPO_DIR/llvm
cmake --build . --config Debug
# Move-Item .\Debug\bin\* $LLVM_REPO_DIR\llvm\bin\ -Force
# Move-Item .\Debug\lib\* $LLVM_REPO_DIR\llvm\lib\ -Force

# env:LLVM = $LLVM_REPO_DIR\llvm

mkdir $FAUST_CMAKE_BUILD_DIR
Set-Location $FAUST_CMAKE_BUILD_DIR
cmake -C $DEPS_DIR\iplug-backends.cmake -C $DEPS_DIR\iplug-targets-win.cmake -DUSE_LLVM_CONFIG=off -DLLVM_DIR=$LLVM_DIR $FAUST_REPO_DIR\build -DINCLUDE_STATIC=off -DINCLUDE_DYNAMIC=on -DINCLUDE_OSC=off -DINCLUDE_HTTP=off -G "Visual Studio 16 2019" -Thost=x64
cmake --build . --config Debug