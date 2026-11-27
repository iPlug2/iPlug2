/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

/**
 * @file
 * @copydoc FaustGen
 */

#ifndef FAUST_COMPILED

#include "IPlugFaustGen.h"
#include "IPlugUtilities.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>

#include "faust/dsp/interpreter-dsp.h"

#ifdef OS_WIN
  #include <windows.h>
#endif

using namespace iplug;

namespace
{
  // The libfaust compiler API is not thread safe
  std::mutex& GetCompileMutex()
  {
    static std::mutex sMutex;
    return sMutex;
  }

  std::filesystem::path ToPath(const std::string& utf8Path)
  {
#if defined OS_WIN
    return std::filesystem::u8path(utf8Path);
#else
    return std::filesystem::path(utf8Path);
#endif
  }

  void LoadFaustLibrary()
  {
#if defined OS_WIN && defined FAUST_DLL_PATH
    // faust.dll is delay-loaded, load it from the Faust installation the plug-in was built against
    static HMODULE sModule = LoadLibraryExA(FAUST_DLL_PATH, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    assert(sModule && "Could not load faust.dll");
#endif
  }
}

FaustGen::FaustGen(const char* name, const char* inputDSPFile, int nVoices, const char* libraryPath)
: IPlugFaust(name, nVoices)
{
  LoadFaustLibrary();

  if (CStringHasContents(libraryPath))
    AddLibraryPath(libraryPath);

  if (CStringHasContents(inputDSPFile))
  {
    mDSPFile = inputDSPFile;
    AddLibraryPath(ToPath(mDSPFile).parent_path().u8string().c_str());
  }

  if (sizeof(FAUSTFLOAT) == sizeof(double))
    mCompileOptions.push_back("-double");
}

FaustGen::~FaustGen()
{
  SetAutoRecompile(false);

  // DSP instances must be deleted before the factory that created them
  ReleaseDSP();

  if (mFactory)
  {
    std::lock_guard<std::mutex> lock(GetCompileMutex());
    deleteInterpreterDSPFactory(mFactory);
  }
}

void FaustGen::Init()
{
  bool success = false;

  if (mDSPFile.size())
  {
    std::string sourceCode;

    if (ReadFile(sourceCode))
      success = Compile(sourceCode);
    else
      mErrorMessage = "Could not read " + mDSPFile;
  }
  else if (mSourceCode.size())
  {
    success = Compile(mSourceCode);
  }

  if (!success)
  {
    DBGMSG("FaustGen-%s: %s\n", mName.Get(), mErrorMessage.c_str());
    CompileDefault();
  }

  if (mOnCompileFunc)
    mOnCompileFunc(success);
}

bool FaustGen::LoadFile(const char* path)
{
  mDSPFile = path;
  AddLibraryPath(ToPath(mDSPFile).parent_path().u8string().c_str());

  std::string sourceCode;

  if (!ReadFile(sourceCode))
  {
    mErrorMessage = "Could not read " + mDSPFile;
    return false;
  }

  const bool success = Compile(sourceCode);

  if (mOnCompileFunc)
    mOnCompileFunc(success);

  return success;
}

bool FaustGen::SetSourceCode(const char* sourceCode)
{
  const bool success = Compile(sourceCode);

  if (mOnCompileFunc)
    mOnCompileFunc(success);

  return success;
}

void FaustGen::AddLibraryPath(const char* path)
{
  if (CStringHasContents(path) && std::find(mLibraryPaths.begin(), mLibraryPaths.end(), path) == mLibraryPaths.end())
    mLibraryPaths.push_back(path);
}

void FaustGen::AddCompileOption(const char* option)
{
  if (CStringHasContents(option))
    mCompileOptions.push_back(option);
}

void FaustGen::SetAutoRecompile(bool enable)
{
  if (enable && !mFileWatchTimer && mDSPFile.size())
  {
    mFileWatchTimer = std::unique_ptr<Timer>(Timer::Create(std::bind(&FaustGen::OnTimer, this, std::placeholders::_1), FAUST_RECOMPILE_INTERVAL));
  }
  else if (!enable && mFileWatchTimer)
  {
    mFileWatchTimer->Stop();
    mFileWatchTimer = nullptr;
  }
}

bool FaustGen::Compile(const std::string& sourceCode)
{
  std::vector<std::string> args = mCompileOptions;

  for (const auto& path : mLibraryPaths)
  {
    args.push_back("-I");
    args.push_back(path);
  }

  std::vector<const char*> argv;

  for (const auto& arg : args)
    argv.push_back(arg.c_str());

  argv.push_back(nullptr);

  std::string error;
  interpreter_dsp_factory* pFactory = nullptr;
  ::dsp* pDSP = nullptr;

  {
    std::lock_guard<std::mutex> lock(GetCompileMutex());
    pFactory = createInterpreterDSPFactoryFromString(mName.Get(), sourceCode, static_cast<int>(args.size()), argv.data(), error);

    if (pFactory)
      pDSP = pFactory->createDSPInstance();
  }

  if (!pFactory || !pDSP)
  {
    mErrorMessage = error.size() ? error : "Unknown error";
    DBGMSG("FaustGen-%s: compilation failed: %s\n", mName.Get(), mErrorMessage.c_str());

    if (pFactory)
    {
      std::lock_guard<std::mutex> lock(GetCompileMutex());
      deleteInterpreterDSPFactory(pFactory);
    }

    return false;
  }

  // Swap in the new DSP, then the previous factory can be deleted
  SetDSP(pDSP);

  if (mFactory)
  {
    std::lock_guard<std::mutex> lock(GetCompileMutex());
    deleteInterpreterDSPFactory(mFactory);
  }

  mFactory = pFactory;
  mSourceCode = sourceCode;
  mErrorMessage.clear();

  DBGMSG("FaustGen-%s: compiled, %i input(s), %i output(s), %i parameter(s)\n", mName.Get(), NInputs(), NOutputs(), NParams());

  return true;
}

void FaustGen::CompileDefault()
{
  // Pass through as many channels as possible, and output silence on the rest
  const int nOutputs = GetMaxNOutputs() > -1 ? GetMaxNOutputs() : 2;
  const int nInputs = std::min(GetMaxNInputs() > -1 ? GetMaxNInputs() : 2, nOutputs);

  std::string code = "process = ";

  if (nInputs > 0)
    code += "par(i, " + std::to_string(nInputs) + ", _)";

  if (nOutputs > nInputs)
    code += std::string(nInputs > 0 ? ", " : "") + "par(i, " + std::to_string(nOutputs - nInputs) + ", 0)";

  if (nOutputs == 0)
    code += "0 : !";

  code += ";";

  const std::string errorMessage = mErrorMessage;
  Compile(code);
  mErrorMessage = errorMessage;
}

bool FaustGen::ReadFile(std::string& contents)
{
  std::error_code ec;
  const auto path = ToPath(mDSPFile);
  const auto modifiedTime = std::filesystem::last_write_time(path, ec);

  if (ec)
    return false;

  std::ifstream file(path, std::ios::in | std::ios::binary);

  if (!file.is_open())
    return false;

  std::stringstream buffer;
  buffer << file.rdbuf();
  contents = buffer.str();
  mFileModifiedTime = modifiedTime.time_since_epoch().count();

  return true;
}

void FaustGen::OnTimer(Timer& timer)
{
  std::error_code ec;
  const auto modifiedTime = std::filesystem::last_write_time(ToPath(mDSPFile), ec);

  if (ec || modifiedTime.time_since_epoch().count() == mFileModifiedTime)
    return;

  std::string sourceCode;

  if (!ReadFile(sourceCode) || sourceCode == mSourceCode)
    return;

  DBGMSG("FaustGen-%s: %s changed, recompiling\n", mName.Get(), mDSPFile.c_str());

  const bool success = Compile(sourceCode); // if this fails, the previous DSP keeps running

  if (mOnCompileFunc)
    mOnCompileFunc(success);
}

#endif // #ifndef FAUST_COMPILED
