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
$LLVM_COMMIT_HASH="ef32c611aa214dea855364efd7ba451ec5ec3f74"

if(!(Test-Path -PathType Container $LLVM_REPO_DIR)) {
    mkdir $LLVM_REPO_DIR
    Push-Location $LLVM_REPO_DIR
    git init
    git remote add origin https://github.com/llvm/llvm-project.git
    git fetch --depth 1 origin $LLVM_COMMIT_HASH
    git checkout FETCH_HEAD
    Pop-Location
}

if(!(Test-Path -PathType Container $FAUST_REPO_DIR)) {
    git clone --depth=1 https://github.com/grame-cncm/faust.git $FAUST_REPO_DIR
}

mkdir $LLVM_CMAKE_BUILD_DIR
Set-Location $LLVM_CMAKE_BUILD_DIR

cmake -Thost=x64 $LLVM_REPO_DIR/llvm
cmake --build . --config Debug

mkdir $FAUST_CMAKE_BUILD_DIR
Set-Location $FAUST_CMAKE_BUILD_DIR
#cmake -C C:\Users\oli\Dev\iPlug2\Dependencies\Extras\faust\iplug-backends.cmake -C C:\Users\oli\Dev\iPlug2\Dependencies\Extras\faust\iplug-targets-win.cmake -DUSE_LLVM_CONFIG=off -DLLVM_DIR=C:\Users\oli\Dev\iPlug2\Dependencies\Build\tmp\llvm-cmake\lib\cmake\llvm $FAUST_REPO_DIR\build -G "Visual Studio 16 2019" -Thost=x64
cmake -C "$DEPS_DIR\iplug-backends.cmake" -C "$DEPS_DIR\iplug-targets-win.cmake" -DUSE_LLVM_CONFIG=off -DLLVM_DIR="$LLVM_CMAKE_BUILD_DIR\lib\cmake\llvm" $FAUST_REPO_DIR\build -G "Visual Studio 16 2019" -Thost=x64

# Now manually add the following to the dynamic_lib.vcxproj "Additional Dependencies" and add the "Additional Library Directory": "C:\Users\oli\Dev\iPlug2\Dependencies\Build\tmp\llvm-cmake\Debug\lib;"
# LLVMAArch64AsmParser.lib
# LLVMAArch64CodeGen.lib
# LLVMAArch64Desc.lib
# LLVMAArch64Disassembler.lib
# LLVMAArch64Info.lib
# LLVMAArch64Utils.lib
# LLVMAggressiveInstCombine.lib
# LLVMAnalysis.lib
# LLVMAsmParser.lib
# LLVMAsmPrinter.lib
# LLVMBinaryFormat.lib
# LLVMBitReader.lib
# LLVMBitstreamReader.lib
# LLVMBitWriter.lib
# LLVMCFGuard.lib
# LLVMCodeGen.lib
# LLVMCore.lib
# LLVMCoroutines.lib
# LLVMCoverage.lib
# LLVMDebugInfoCodeView.lib
# LLVMDebugInfoDWARF.lib
# LLVMDebugInfoGSYM.lib
# LLVMDebugInfoMSF.lib
# LLVMDebugInfoPDB.lib
# LLVMDemangle.lib
# LLVMDlltoolDriver.lib
# LLVMDWARFLinker.lib
# LLVMExecutionEngine.lib
# LLVMFrontendOpenMP.lib
# LLVMFuzzMutate.lib
# LLVMGlobalISel.lib
# LLVMInstCombine.lib
# LLVMInstrumentation.lib
# LLVMInterpreter.lib
# LLVMipo.lib
# LLVMIRReader.lib
# LLVMJITLink.lib
# LLVMLibDriver.lib
# LLVMLineEditor.lib
# LLVMLinker.lib
# LLVMLTO.lib
# LLVMMC.lib
# LLVMMCA.lib
# LLVMMCDisassembler.lib
# LLVMMCJIT.lib
# LLVMMCParser.lib
# LLVMMIRParser.lib
# LLVMObjCARCOpts.lib
# LLVMObject.lib
# LLVMObjectYAML.lib
# LLVMOption.lib
# LLVMOrcError.lib
# LLVMOrcJIT.lib
# LLVMPasses.lib
# LLVMProfileData.lib
# LLVMRemarks.lib
# LLVMRuntimeDyld.lib
# LLVMScalarOpts.lib
# LLVMSelectionDAG.lib
# LLVMSupport.lib
# LLVMSymbolize.lib
# LLVMTableGen.lib
# LLVMTarget.lib
# LLVMTextAPI.lib
# LLVMTransformUtils.lib
# LLVMVectorize.lib
# LLVMWindowsManifest.lib
# LLVMX86AsmParser.lib
# LLVMX86CodeGen.lib
# LLVMX86Desc.lib
# LLVMX86Disassembler.lib
# LLVMX86Info.lib
# LLVMXRay.lib
# LTO.lib
# Remarks.lib

cmake --build . --config Debug