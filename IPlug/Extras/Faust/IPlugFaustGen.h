/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

#ifndef FAUST_COMPILED
#define FAUST_BLOCK(class, member, file, nvoices, rate) FaustGen member {#class, file, nvoices, rate}
#else
#define FAUST_BLOCK(class, member, file, nvoices, rate) Faust_##class member {#class, file, nvoices, rate}
// if this file is not found, you need to run the code without FAUST_COMPILED defined and make sure to call CompileCPP();
#include "FaustCode.hpp"
using FaustGen = iplug::IPlugFaust; // not used, except for CompileCPP();
#endif

#ifndef FAUST_COMPILED

#include <iostream>
#include <string>
#include <set>
#include <vector>
#include <map>

#include "IPlugPlatform.h"
#include "IPlugConstants.h"
#include "IPlugPaths.h"

#include <sys/stat.h>

#if defined OS_MAC || defined OS_LINUX
typedef struct stat StatType;
typedef timespec StatTime;

static inline int GetStat(const char* path, StatType* pStatbuf) { return stat(path, pStatbuf); }
static inline StatTime GetModifiedTime(StatType &s) { return s.st_mtimespec; }
static inline bool Equal(StatTime a, StatTime b) { return (a.tv_sec == b.tv_sec && a.tv_nsec == b.tv_nsec); }
static inline StatTime TimeZero()
{
  StatTime ts;
  ts.tv_sec = 0;
  ts.tv_nsec = 0;
  return ts;
}
#else //OS_WIN
typedef struct _stat64i32 StatType;
typedef time_t StatTime;

static inline int GetStat(const char* path, StatType* pStatbuf)
{
  wchar_t utf16str[MAX_PATH];
  iplug::UTF8ToUTF16(utf16str, path, MAX_PATH);
  return _wstat(utf16str, pStatbuf);
}
static inline StatTime GetModifiedTime(StatType &s) { return s.st_mtime; }
static inline bool Equal(StatTime a, StatTime b) { return a == b; }
static inline StatTime TimeZero() { return (StatTime) 0; }
#endif

#define FAUSTFLOAT iplug::sample

#include "faust/dsp/llvm-dsp.h"
#include "IPlugFaust.h"
#include "IPlugTimer.h"

#include "mutex.h"

#ifdef OS_WIN
#pragma comment(lib, "faust.lib")
#else
#include <libgen.h>
#endif

#define DEFAULT_SOURCE_CODE_FMT_STR_FX "import(\"stdfaust.lib\");\nprocess=par(i,%i,_);"
#define DEFAULT_SOURCE_CODE_FMT_STR_INSTRUMENT "import(\"stdfaust.lib\");\nprocess=par(i,%i,0);"

#define FAUSTGEN_VERSION "1.19"
#define LLVM_OPTIMIZATION -1  // means 'maximum'

#define FAUST_CLASS_PREFIX "F"
#define FAUST_RECOMPILE_INTERVAL 5000 //ms

#ifndef FAUST_EXE
  #if defined OS_MAC || defined OS_LINUX
    #define FAUST_EXE "/usr/local/bin/faust"
  #else
    #define FAUST_EXE "C:\\\"Program Files\"\\Faust\\bin\\faust.exe"//Double quotes around "Program Files" because of whitespace
  #endif
#endif

#ifndef FAUST_DLL_PATH
  #if defined OS_MAC || defined OS_LINUX
    #define FAUST_DLL_PATH "/usr/local/lib/"
  #else
    #define FAUST_DLL_PATH "C:\\Program Files\\Faust\\lib"
  #endif
#endif

BEGIN_IPLUG_NAMESPACE

class FaustGen : public IPlugFaust
{
  class Factory
  {
#ifdef OS_MAC
    static std::string GetLLVMArchStr()
    {
      int tmp;
      return (sizeof(&tmp) == 8) ? "" : "i386-apple-darwin10.6.0";
    }
#else
    static std::string GetLLVMArchStr()
    {
      return "";
    }

    static std::string getTarget() { return ""; }

#endif

    //static const char *getCodeSize()
    //{
    //  int tmp;
    //  return (sizeof(&tmp) == 8) ? "64 bits" : "32 bits";
    //}

    friend class FaustGen;

  public:
    Factory(const char* name, const char* libPath, const char* drawPath, const char* inputDSP);
    ~Factory();

    Factory(const Factory&) = delete;
    Factory& operator=(const Factory&) = delete;
      
    llvm_dsp_factory* CreateFactoryFromBitCode();
    llvm_dsp_factory* CreateFactoryFromSourceCode();
    
    /** If DSP already exists will return it, otherwise create it
     * @return pointer to the DSP instance */
    ::dsp* GetDSP(int maxInputs, int maxOutputs);

    void FreeDSPFactory();
    void SetDefaultCompileOptions();
    void PrintCompileOptions();

    int GetInstanceID() { return mInstanceIdx; }
    const char* GetName() { return mName.Get(); }
    const char* GetSourceCode() { return mSourceCodeStr.Get(); }

    void UpdateSourceCode(const char* str);

    ::dsp* CreateDSPInstance(int nVoices = 0);
    void AddInstance(FaustGen* pDSP) { mInstances.insert(pDSP); }
    void RemoveInstance(FaustGen* pDSP);

    bool LoadFile(const char* file);
    bool WriteToFile(const char* file);
    void SetCompileOptions(std::initializer_list<const char*> options);

  private:
    void AddLibraryPath(const char* libraryPath);
    void AddCompileOption(const char* key, const char* value = "");
  private:
    struct FMeta : public Meta
    {
      void declare(const char* key, const char* value) override
      {
        DBGMSG("FaustGen: metadata:\n");

        if ((strcmp("name", key) == 0) || (strcmp("author", key) == 0))
        {
          DBGMSG("\t\tkey:%s : %s\n", key, value);
        }
        items.AddUnsorted(key, value);
      }

      const std::string get(const char* key, const char* value)
      {
        return items.Get(key, value);
      }

      WDL_StringKeyedArray<const char*> items;
    };

  private:
    int mInstanceIdx;
    WDL_Mutex mDSPMutex;
    std::set<FaustGen*> mInstances;

    llvm_dsp_factory* mLLVMFactory = nullptr;
    //  midi_handler mMidiHandler;
    WDL_FastString mSourceCodeStr;
    WDL_FastString mBitCodeStr;
    WDL_String mDrawPath;
    WDL_String mName;

    std::vector<std::string> mLibraryPaths;
    std::vector<std::string> mOptions;
    std::vector<std::string> mCompileOptions;

    int mNInputs = 0;
    int mNOutputs = 0;
    int mOptimizationLevel = LLVM_OPTIMIZATION;
    static int sFactoryCounter;
    static std::map<std::string, Factory*> sFactoryMap;
    WDL_String mInputDSPFile;
    StatTime mPreviousTime;
  };
public:

  FaustGen(const char* name, const char* inputDSPFile = 0, int nVoices = 1, int rate = 1,
           const char* outputCPPFile = 0, const char* drawPath = 0, const char* libraryPath = FAUST_LIBRARY_PATH);

  ~FaustGen();

  FaustGen(const FaustGen&) = delete;
  FaustGen& operator=(const FaustGen&) = delete;
    
  /** Call this method after constructing the class to inform FaustGen what the maximum I/O count is
   * @param maxNInputs Specify a number here to tell FaustGen the maximum number of inputs the hosting code can accommodate
   * @param maxNOutputs Specify a number here to tell FaustGen the maximum number of outputs the hosting code can accommodate */
  void SetMaxChannelCount(int maxNInputs, int maxNOutputs) override { mMaxNInputs = maxNInputs; mMaxNOutputs = maxNOutputs; }
  
  /** Call this method after constructing the class to JIT compile */
  void Init() override;

  void LoadFile(const char* path) { mFactory->FreeDSPFactory(); mFactory->LoadFile(path); }
  
  /** This method allows SVG files generated by a specific instance of FaustGen can be located. The path to the SVG file for process.svg will be returned, if drawPath has been specified in the constructor.
   * This method will trigger an assertion if drawPath has not been specified
   * @param path The absolute path to process.svg for this instance. */
  void GetDrawPath(WDL_String& path) override;

  /** This is a static method that can be called to compile all .dsp files used to a single .hpp file, using the commandline FAUST compiler
   * @return \c true on success */
  static bool CompileCPP();

  //bool CompileObjectFile(const char* fileName);

  void SetAutoRecompile(bool enable);
  
  void SetCompileFunc(std::function<void()> func) { mOnCompileFunc = func; }
  
  void OnTimer(Timer& timer);
  
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  
  void SetErrored(bool errored) { mErrored = errored; }
  
private:
  Factory* mFactory = nullptr;
  static Timer* sTimer;
  static int sFaustGenCounter;
  static bool sAutoRecompile;
  int mMaxNInputs = -1;
  int mMaxNOutputs = -1;
  bool mErrored = false;
  std::function<void()> mOnCompileFunc = nullptr;
  
  WDL_Mutex mMutex;
};

END_IPLUG_NAMESPACE

#endif // #ifndef FAUST_COMPILED
