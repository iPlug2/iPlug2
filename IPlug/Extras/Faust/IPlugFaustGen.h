/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc FaustGen
 *
 * Include this file to use FAUST_BLOCK, which declares a Faust DSP member that is either:
 * - a FaustGen, which compiles a .dsp file at runtime with the libfaust interpreter backend and recompiles it when it changes (for development), or
 * - a Faust_<class> statically compiled with the IPlugFaust_arch.cpp architecture file, if FAUST_COMPILED is defined (for release builds).
 *   FaustCode.hpp must include the generated classes. The CMake Faust module (iplug_add_faust()) generates it.
 *
 * @param class The name of the generated class (Faust's -cn argument)
 * @param member The name of the member variable
 * @param file The absolute path of the .dsp file (only used by FaustGen)
 * @param nVoices The number of polyphonic voices, if the DSP doesn't declare "[nvoices:N]" in its options */

#ifdef FAUST_COMPILED
  #include "FaustCode.hpp"
  #define FAUST_BLOCK(class, member, file, nVoices) Faust_##class member {#class, file, nVoices}
#else
  #define FAUST_BLOCK(class, member, file, nVoices) iplug::FaustGen member {#class, file, nVoices}
#endif

#ifndef FAUST_COMPILED

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "IPlugFaust.h"

#ifndef FAUST_RECOMPILE_INTERVAL
  #define FAUST_RECOMPILE_INTERVAL 1000 // ms
#endif

class interpreter_dsp_factory;

BEGIN_IPLUG_NAMESPACE

/** FaustGen compiles Faust code at runtime using the libfaust interpreter backend, so that the DSP can be edited while the plug-in is running.
 * The interpreter is much slower than compiled code, so use it for development and switch to the statically compiled DSP (FAUST_COMPILED) for release builds.
 * All methods except ProcessBlock() and ProcessMidiMsg() must be called on the main thread. */
class FaustGen : public IPlugFaust
{
public:
  /** @param name A name for the DSP
   * @param inputDSPFile The absolute path of a .dsp file to load. Its folder is added to the library search paths.
   * @param nVoices Number of polyphonic voices, if the DSP doesn't declare "[nvoices:N]" in its options. Values < 2 mean monophonic.
   * @param libraryPath The path to the Faust standard libraries (the share/faust folder of a Faust installation) */
  FaustGen(const char* name, const char* inputDSPFile = nullptr, int nVoices = 0, const char* libraryPath = FAUST_SHARE_PATH);
  ~FaustGen();

  FaustGen(const FaustGen&) = delete;
  FaustGen& operator=(const FaustGen&) = delete;

  /** Compile the DSP. If no file was given, or it fails to compile, a pass-through/silent DSP that matches the max channel count is created.
   * Call SetMaxChannelCount() first. */
  void Init() override;

  /** Load and compile a .dsp file, replacing the current DSP if compilation succeeds
   * @return \c true on success */
  bool LoadFile(const char* path);

  /** Compile Faust source code, replacing the current DSP if compilation succeeds
   * @return \c true on success */
  bool SetSourceCode(const char* sourceCode);

  /** Add a path that is searched for imported .lib/.dsp files */
  void AddLibraryPath(const char* path);

  /** Add a compiler option such as "-vec" (see faust --help) */
  void AddCompileOption(const char* option);

  /** Watch the .dsp file and recompile it when it changes */
  void SetAutoRecompile(bool enable);

  /** Set a function that is called on the main thread after a (re)compilation. When it is called after a successful recompilation,
   * the parameters have already been updated, but the plug-in's UI should be rebuilt e.g. with IGraphicsFaustUI.
   * After it returns, the host is told to re-read the parameter details (IPluginBase::InformHostOfParameterDetailsChange()),
   * so any other changes the plug-in makes to its parameters in this function are included */
  void SetOnCompileFunc(std::function<void(bool success)> func) { mOnCompileFunc = func; }

  /** @return The error message of the last failed compilation, or an empty string */
  const char* GetErrorMessage() const { return mErrorMessage.c_str(); }

  const char* GetSourceCode() const { return mSourceCode.c_str(); }
  const char* GetDSPFile() const { return mDSPFile.c_str(); }

private:
  bool Compile(const std::string& sourceCode);
  void CompileDefault();
  bool ReadFile(std::string& contents);
  void OnTimer(Timer& timer);
  void OnCompiled(bool success);

  std::string mDSPFile;
  std::string mSourceCode;
  std::string mErrorMessage;
  std::vector<std::string> mLibraryPaths;
  std::vector<std::string> mCompileOptions;
  int64_t mFileModifiedTime = 0;

  interpreter_dsp_factory* mFactory = nullptr;
  std::unique_ptr<Timer> mFileWatchTimer;
  std::function<void(bool success)> mOnCompileFunc;
};

END_IPLUG_NAMESPACE

#endif // #ifndef FAUST_COMPILED
