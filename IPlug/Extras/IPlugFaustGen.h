#pragma once

#ifndef FAUST_COMPILED

#include <iostream>
#include <string>
#include <set>
#include <vector>
#include <map>

#include "IPlugPlatform.h"

#define FAUSTFLOAT double

#include "faust/dsp/llvm-dsp.h"
#include "IPlugFaust.h"

#include "mutex.h"

#ifndef OS_WIN
#include <libgen.h>
#endif

#define DEFAULT_SOURCE_CODE "import(\"stdfaust.lib\");\nprocess=_;"
#define FAUSTGEN_VERSION "1.19"
#define LLVM_OPTIMIZATION -1  // means 'maximum'
#define FAUST_EXE "/usr/local/bin/faust"

using namespace std;

class FaustGen;

class FaustFactory
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
  FaustFactory(const char* name, const char* libPath, const char* drawPath);
  ~FaustFactory();
  
  llvm_dsp_factory* CreateFactoryFromBitCode();
  llvm_dsp_factory* CreateFactoryFromSourceCode();
  ::dsp* CreateDSPAux(const char* str = 0);
  
  void FreeDSPFactory();
  void SetDefaultCompileOptions();
  void PrintCompileOptions();
  
  int GetInstanceID() { return mInstanceID; }
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
  int mInstanceID;
  WDL_Mutex mDSPMutex;
  set<FaustGen*> mInstances;

  llvm_dsp_factory* mFactory = nullptr;
  //  midi_handler mMidiHandler;
  WDL_String mSourceCodeStr;
  WDL_String mBitCodeStr;
  WDL_String mDrawPath;
  WDL_String mName;
  
  WDL_TypedBuf<const char*> mLibraryPaths;
  vector<const char*> mOptions;
  vector<const char*> mCompileOptions;

  int mNDSPInputs = 0;
  int mNDSPOutputs = 0;
  int mOptimizationLevel = LLVM_OPTIMIZATION;
  static int gFaustGenCounter;
  static map<string, FaustFactory*> gFactoryMap;
};

class FaustGen : public IPlugFaust
{
  friend class FaustFactory;
public:
  
  FaustGen(const char* name,
           const char* inputDSPFile = 0,
           const char* outputCPPFile = 0,
           const char* archFile = 0,
           const char* drawPath = 0,
           const char* libraryPath = DEFAULT_FAUST_LIBRARY_PATH,
           int nVoices = 1);
  
  ~FaustGen();
  
  //IPlugFaust
  void Init(const char* sourceStr = "", int maxNInputs = -1, int maxNOutputs = -1) override;
//  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void GetSVGPath(WDL_String& path) override;
  bool CompileArchitectureFile() override;
  
private:
  void SourceCodeChanged();
  FaustFactory* mFactory = nullptr;
  WDL_String mInputDSPFile;
  WDL_String mOutputCPPFile;
  WDL_String mArchitectureFile;
};

#endif // #ifndef FAUST_COMPILED
