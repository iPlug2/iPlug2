#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <set>
#include <vector>
#include <map>

#include "IPlugPlatform.h"

#define FAUSTFLOAT double

#include "faust/dsp/llvm-dsp.h"
// #include "faust/gui/JSONUI.h"
// #include "faust/gui/MidiUI.h"

#include "mutex.h"

#ifndef OS_WIN
#include <libgen.h>
#endif

#define DEFAULT_SOURCE_CODE "import(\"stdfaust.lib\");\nprocess=_;"
#define FAUSTGEN_VERSION "1.19"
#define LLVM_OPTIMIZATION -1  // means 'maximum'

#if defined OS_MAC || defined OS_LINUX
#define DEFAULT_FAUST_LIBRARY_PATH "/usr/local/share/faust/"
#else
#define DEFAULT_FAUST_LIBRARY_PATH //TODO
#endif

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
  const char* GetJSON() { return mJSON.Get(); }
  
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
  void MakeJson(::dsp* pDSP);
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
  WDL_String mJSON;
  
  WDL_TypedBuf<const char*> mLibraryPaths;
  vector<const char*> mOptions;
  vector<const char*> mCompileOptions;

  int mNDSPInputs = 0;
  int mNDSPOutputs = 0;
  int mOptimizationLevel = LLVM_OPTIMIZATION;
  static int gFaustGenCounter;
  static map<string, FaustFactory*> gFactoryMap;
};

class FaustGen
{
  friend class FaustFactory;
public:
  
  FaustGen(const char* name, const char* inputFile = 0, const char* outputFile = 0, const char* drawPath = 0, const char* libraryPath = DEFAULT_FAUST_LIBRARY_PATH);
  ~FaustGen();

  void GetSVGPath(WDL_String& path)
  {
    path.SetFormatted(MAX_WIN32_PATH_LEN, "%sFaustGen-%d-svg/process.svg", mFactory->mDrawPath.Get(), mFactory->mInstanceID);
  }
  
  void Init(const char* str);
  void FreeDSP();
//  void AddMidiHandler();
//  void RemoveMidiHandler();
  
  void SetSampleRate(double sampleRate);
  
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames);
private:
  void SourceCodeChanged();
  FaustFactory* mFactory = nullptr;
  // MidiUI* mMidiUI;
  ::dsp* mDSP = nullptr;
};

