#pragma once

#ifndef FAUST_COMPILED
#define FAUST_BLOCK(class, member, file, nvoices, rate) FaustGen member = FaustGen(*this, #class, file, nvoices, rate)
#else
#define FAUST_BLOCK(class, member, file, nvoices, rate) class member = class(*this, #class, file, nvoices, rate)
#include "FaustCode.hpp"
typedef IPlugFaust FaustGen; // not used, except for CompileCPP();
#endif

#ifndef FAUST_COMPILED

#include <iostream>
#include <string>
#include <set>
#include <vector>
#include <map>

#include "IPlugPlatform.h"

#include <sys/stat.h>

#if defined OS_MAC || defined OS_LINUX
typedef struct stat StatType;
typedef timespec Time;

static inline int GetStat(const char* pPath, StatType* pStatbuf) { return stat(pPath, pStatbuf); }
static inline Time GetModifiedTime(StatType &s) { return s.st_mtimespec; }
static inline bool Equal(Time a, Time b) { return (a.tv_sec == b.tv_sec && a.tv_nsec == b.tv_nsec); }
static inline Time TimeZero()
{
  Time ts;
  ts.tv_sec = 0;
  ts.tv_nsec = 0;
  return ts;
}
#else //OS_WIN
typedef struct _stat64i32 StatType;
typedef time_t Time;

static inline int GetStat(std::wstring& path, StatType* pStatbuf) { return _wstat(path.c_str(), pStatbuf); }
static inline Time GetModifiedTime(StatType &s) { return s.st_mtime; }
static inline bool Equal(Time a, Time b) { return a == b; }
static inline Time TimeZero() { return (Time) 0; }
#endif

#define FAUSTFLOAT double

#include "faust/dsp/llvm-dsp.h"
#include "IPlugFaust.h"

#include "base/source/timer.h"

#include "mutex.h"

#ifndef OS_WIN
#include <libgen.h>
#endif

#define DEFAULT_SOURCE_CODE "import(\"stdfaust.lib\");\nprocess=_,_;"
#define FAUSTGEN_VERSION "1.19"
#define LLVM_OPTIMIZATION -1  // means 'maximum'
#define FAUST_EXE "/usr/local/bin/faust"
#define FAUST_CLASS_PREFIX "F"
#define FAUST_RECOMPILE_INTERVAL 5000 //ms

using namespace std;

class FaustGen : public IPlugFaust,
                 public Steinberg::ITimerCallback
{
  class Factory
  {
#ifdef OS_MAC
    static string GetLLVMArchStr()
    {
      int tmp;
      return (sizeof(&tmp) == 8) ? "" : "i386-apple-darwin10.6.0";
    }
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

    llvm_dsp_factory* CreateFactoryFromBitCode();
    llvm_dsp_factory* CreateFactoryFromSourceCode();
    
    
    /** If DSP allready exists will return it, otherwise create it
     * @return pointer to the DSP instance */
    ::dsp* GetDSP();

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
    struct FMeta : public Meta, public std::map<std::string, std::string>
    {
      void declare(const char *key, const char *value)
      {
        //      DBGMSG("FaustGen: metadata:\n");
        //
        //      if ((strcmp("name", key) == 0) || (strcmp("author", key) == 0))
        //      {
        //        DBGMSG("\t\tkey:%s : %s\n", key, value);
        //      }
        //
        (*this)[key] = value;
      }

      const std::string get(const char *key, const char *def)
      {
        if (this->find(key) != this->end())
        {
          return (*this)[key];
        }
        else
        {
          return def;
        }
      }
    };

  private:
    int mInstanceIdx;
    WDL_Mutex mDSPMutex;
    set<FaustGen*> mInstances;

    llvm_dsp_factory* mLLVMFactory = nullptr;
    //  midi_handler mMidiHandler;
    WDL_FastString mSourceCodeStr;
    WDL_FastString mBitCodeStr;
    WDL_String mDrawPath;
    WDL_String mName;

    vector<string> mLibraryPaths;
    vector<string> mOptions;
    vector<string> mCompileOptions;

    int mNInputs = 0;
    int mNOutputs = 0;
    int mOptimizationLevel = LLVM_OPTIMIZATION;
    static int sFactoryCounter;
    static map<string, Factory*> sFactoryMap;
    WDL_String mInputDSPFile;
    Time mPreviousTime;
  };
public:

  FaustGen(IPlugBase& plug, const char* name, const char* inputDSPFile = 0, int nVoices = 1, int rate = 1,
           const char* outputCPPFile = 0, const char* drawPath = 0, const char* libraryPath = DEFAULT_FAUST_LIBRARY_PATH);

  ~FaustGen();


  /** Call this method after constructing the class to initialise
   * @param maxNInputs Specify a number here to tell FaustGen the maximum number of inputs the hosting code can accommodate
   * @param maxNOutputs Specify a number here to tell FaustGen the maximum number of outputs the hosting code can accommodate */
  void Init(int maxNInputs = -1, int maxNOutputs = -1) override;

  /** This method allows SVG files generated by a specific instance of FaustGen can be located. The path to the SVG file for process.svg will be returned, if drawPath has been specified in the constructor.
   * This method will trigger an assertion if drawPath has not been specified
   * @param path The absolute path to process.svg for this instance. */
  void GetDrawPath(WDL_String& path) override;

  /** This is a static method that can be called to compile all .dsp files used to a single .hpp file, using the commandline FAUST compiler
   * @return \c true on success */
  static bool CompileCPP();

  void EnableTimer(bool enable);

  //ITimerCallback
  void onTimer(Steinberg::Timer* timer) override;
  
private:
  void SourceCodeChanged();
  Factory* mFactory = nullptr;
  static Steinberg::Timer* sTimer;
  static int sFaustGenCounter;
  static bool sEnableTimer;
};

#endif // #ifndef FAUST_COMPILED
