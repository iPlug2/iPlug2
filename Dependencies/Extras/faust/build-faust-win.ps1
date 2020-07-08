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

mkdir $FAUST_CMAKE_BUILD_DIR
Set-Location $FAUST_CMAKE_BUILD_DIR
#cmake -C C:\Users\oli\Dev\iPlug2\Dependencies\Extras\faust\iplug-backends.cmake -C C:\Users\oli\Dev\iPlug2\Dependencies\Extras\faust\iplug-targets-win.cmake -DUSE_LLVM_CONFIG=off -DLLVM_DIR=C:\Users\oli\Dev\iPlug2\Dependencies\Build\tmp\llvm-cmake\lib\cmake\llvm $FAUST_REPO_DIR\build -G "Visual Studio 16 2019" -Thost=x64
cmake -C "$DEPS_DIR\iplug-backends.cmake" -C "$DEPS_DIR\iplug-targets-win.cmake" -DUSE_LLVM_CONFIG=off -DLLVM_DIR="$LLVM_CMAKE_BUILD_DIR\lib\cmake\llvm" $FAUST_REPO_DIR\build -G "Visual Studio 16 2019" -Thost=x64
cmake --build . --config Debug