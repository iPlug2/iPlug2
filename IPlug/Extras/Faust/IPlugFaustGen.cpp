#ifndef FAUST_COMPILED

#include "IPlugFaustGen.h"
#include "IPlugUtilities.h"

#include "faust/dsp/libfaust.h"

#define LLVM_DSP
#include "faust/dsp/poly-dsp.h"

#ifndef OS_WIN
#include "faust/sound-file.h"
#endif

int FaustGen::sFaustGenCounter = 0;
int FaustGen::Factory::sFactoryCounter = 0;
bool FaustGen::sEnableTimer = false;
map<string, FaustGen::Factory *> FaustGen::Factory::sFactoryMap;
std::list<GUI*> GUI::fGuiList;
Steinberg::Timer* FaustGen::sTimer = nullptr;

FaustGen::Factory::Factory(const char* name, const char* libraryPath, const char* drawPath, const char* inputDSP)
{
  mPreviousTime = TimeZero();
  mName.Set(name);
  mInstanceIdx = sFactoryCounter++;
//  mMidiHandler.start_midi();

  AddLibraryPath(libraryPath);
  mDrawPath.Set(drawPath);
  
  LoadFile(inputDSP);
}

FaustGen::Factory::~Factory()
{
  FreeDSPFactory();
  mSourceCodeStr.Set("");
  mBitCodeStr.Set("");
//  mMidiHandler.stop_midi();
}

void FaustGen::Factory::FreeDSPFactory()
{
  WDL_MutexLock lock(&mDSPMutex);

  for (auto inst : mInstances)
  {
    inst->FreeDSP();
  }

  if(mLLVMFactory)
  {
    deleteDSPFactory(mLLVMFactory); // this is commented in faustgen~
    mLLVMFactory = nullptr;
  }
}

llvm_dsp_factory *FaustGen::Factory::CreateFactoryFromBitCode()
{
  //return readDSPFactoryFromBitCodeStr(mBitCodeStr.Get(), getTarget(), mOptimizationLevel);

  // Alternate model using machine code
  return readDSPFactoryFromMachine(mBitCodeStr.Get(), GetLLVMArchStr());

  /*
  // Alternate model using LLVM IR
  return readDSPFactoryFromIR(mBitCodeStr.Get(), getTarget(), mOptimizationLevel);
  */
}

llvm_dsp_factory *FaustGen::Factory::CreateFactoryFromSourceCode()
{
  WDL_String name;
  name.SetFormatted(64, "FaustGen-%d", mInstanceIdx);

  SetDefaultCompileOptions();
  PrintCompileOptions();

  // Prepare compile options
  string error;
  const char* argv[64];

  const int N = (int) mCompileOptions.size();

  assert(N < 64);

  for (auto i = 0; i< N; i++)
  {
    argv[i] = mCompileOptions[i].c_str();
  }

  // Generate SVG file // this shouldn't get called if we not making SVGs
  if (!generateAuxFilesFromString(name.Get(), mSourceCodeStr.Get(), N, argv, error))
  {
    DBGMSG("FaustGen: Generate SVG error : %s\n", error.c_str());
  }

#ifdef OS_WIN
  argv[N] = "-l";
  argv[N + 1] = "llvm_math.ll";
  argv[N + 2] = 0; // NULL terminated argv
  llvm_dsp_factory* pFactory = createDSPFactoryFromString(name.Get(), mSourceCodeStr.Get(), N + 2, argv, getTarget(), error, mOptimizationLevel);
#else
  argv[N] = 0; // NULL terminated argv
  llvm_dsp_factory* pFactory = createDSPFactoryFromString(name.Get(), mSourceCodeStr.Get(), N, argv, GetLLVMArchStr(), error, mOptimizationLevel);
#endif

  if (pFactory)
  {

    return pFactory;
  }
  else
  {
    // Update all instances
//    for (auto inst : mInstances)
//    {
//      inst->hilight_on();
//    }

    //WHAT IS THIS?
//    if (mInstances.begin() != mInstances.end())
//    {
//      (*mInstances.begin())->hilight_error(error);
//    }
    DBGMSG("FaustGen: Invalid Faust code or compile options : %s\n", error.c_str());
    return 0;
  }
}

::dsp *FaustGen::Factory::CreateDSPInstance(int nVoices)
{
  ::dsp* pMonoDSP = mLLVMFactory->createDSPInstance();

  // Check 'nvoices' metadata
  if (nVoices == 0)
  {
    FMeta meta;
    pMonoDSP->metadata(&meta);
    std::string numVoices = meta.get("nvoices", "0");
    nVoices = atoi(numVoices.c_str());
    if (nVoices < 0)
      nVoices = 0;
  }

  if (nVoices > 0)
    return new mydsp_poly(pMonoDSP, nVoices, true);
  else
    return pMonoDSP;
}

::dsp *FaustGen::Factory::GetDSP()
{
  ::dsp* pDSP = nullptr;
  FMeta meta;
  std::string error;

  // Factory already allocated
  if (mLLVMFactory)
  {
    pDSP = CreateDSPInstance();
    DBGMSG("FaustGen: Factory already allocated, %i input(s), %i output(s)\n", pDSP->getNumInputs(), pDSP->getNumOutputs());
    goto end;
  }

  // Tries to create from bitcode
  if (mBitCodeStr.GetLength())
  {
    mLLVMFactory = CreateFactoryFromBitCode();
    if (mLLVMFactory)
    {
      pDSP = CreateDSPInstance();
      pDSP->metadata(&meta);
      DBGMSG("FaustGen: Compilation from bitcode succeeded, %i input(s), %i output(s)\n", pDSP->getNumInputs(), pDSP->getNumOutputs());
      goto end;
    }
  }

  // Otherwise tries to create from source code
  if (mSourceCodeStr.GetLength())
  {
    mLLVMFactory = CreateFactoryFromSourceCode();
    if (mLLVMFactory)
    {
      pDSP = CreateDSPInstance();
      pDSP->metadata(&meta);
      DBGMSG("FaustGen: Compilation from source code succeeded, %i input(s), %i output(s)\n", pDSP->getNumInputs(), pDSP->getNumOutputs());
      goto end;
    }
  }

    // Otherwise creates default DSP keeping the same input/output number
#ifdef OS_WIN
//  // Prepare compile options
//  const char* argv[64];
//
//  const int N = (int) mCompileOptions.size();
//
//  assert(N < 64);
//
//  for (auto i = 0; i< N; i++)
//  {
//    argv[i] = mCompileOptions[i];
//  }
//
//  argv[N] = "-l";
//  argv[N + 1] = "llvm_math.ll";
//  argv[N + 2] = 0; // NULL terminated argv
//
//  mLLVMFactory = createDSPFactoryFromString("default", DEFAULT_SOURCE_CODE, N + 2, argv, getTarget(), error, 0);
#else
  mSourceCodeStr.Set(DEFAULT_SOURCE_CODE);
  mLLVMFactory = createDSPFactoryFromString("default", mSourceCodeStr.Get(), 0, 0, GetLLVMArchStr(), error, 0);
#endif

  pDSP = CreateDSPInstance();
  DBGMSG("FaustGen: Allocation of default DSP succeeded, %i input(s), %i output(s)\n", pDSP->getNumInputs(), pDSP->getNumOutputs());

end:
  assert(pDSP);
  mNInputs = pDSP->getNumInputs();
  mNOutputs = pDSP->getNumOutputs();
  return pDSP;
}

void FaustGen::Factory::AddLibraryPath(const char* libraryPath)
{
  if (CStringHasContents(libraryPath))
  {
    if(std::find(mLibraryPaths.begin(), mLibraryPaths.end(), libraryPath) == mLibraryPaths.end())
      mLibraryPaths.push_back(libraryPath);
  }
}

void FaustGen::Factory::AddCompileOption(const char* key, const char* value)
{
  if (CStringHasContents(key))
  {
    mCompileOptions.push_back(key);
  }

  if (CStringHasContents(value))
  {
    mCompileOptions.push_back(value);
  }
}

void FaustGen::Factory::PrintCompileOptions()
{
  if (mCompileOptions.size() > 0)
  {
    DBGMSG("FaustGen: Compile options\n");

    int idx = 0;

    for (auto c : mCompileOptions)
    {
      DBGMSG("\t\t%i: = %s\n", idx++, c.c_str());
    }
  }
}

void FaustGen::Factory::SetDefaultCompileOptions()
{
  // Clear and set default value
  mCompileOptions.clear();

  if (sizeof(sample) == 8)
    AddCompileOption("-double");

  // All library paths
  for (auto i = 0; i< mLibraryPaths.size(); i++)
  {
    AddCompileOption("-I", mLibraryPaths[i].c_str());
  }

  // Draw path
  if(mDrawPath.GetLength())
  {
    AddCompileOption("-svg");
    AddCompileOption("-O", mDrawPath.Get());
  }

  // All options set in the 'compileoptions' message
  for (auto c : mOptions)
  {
    // '-opt v' : parsed for LLVM optimization level
    if (c == "-opt")
    {
      mOptimizationLevel = atoi(c.c_str());
    }
    else
    {
      AddCompileOption(c.c_str());
    }
  }

  // Vector mode by default
  /*
  AddCompileOption("-vec");
  AddCompileOption("-lv");
  AddCompileOption("1");
  */
  /*
  Seems not necessary...
  AddCompileOption("-vs", TOSTRING(GetBlockSize()));
  */
}

void FaustGen::Factory::UpdateSourceCode(const char* str)
{
  DBGMSG("FaustGen: Updating source code %s...\n", str);

  // Recompile only if text has been changed
  if (strcmp(str, mSourceCodeStr.Get()) != 0)
  {
    // Update all instances
//    for (auto inst : mInstances)
    //      inst->hilight_off();
    //    }

    // Delete the existing Faust module
    FreeDSPFactory();

    mSourceCodeStr.Set(str);

    // Free the memory allocated for fBitCode
    mBitCodeStr.Set("");

    // Update all instances
    for (auto inst : mInstances)
    {
      inst->SourceCodeChanged();
    }
  }
  else
  {
    DBGMSG("FaustGen: DSP code has not been changed...\n");
  }
}

void FaustGen::Factory::RemoveInstance(FaustGen* pDSP)
{
  mInstances.erase(pDSP);

  // Last instance : remove factory from global table and commit suicide...
  if (mInstances.size() == 0)
  {
    sFactoryMap.erase(mName.Get());
    delete this;
  }
}

bool FaustGen::Factory::LoadFile(const char* file)
{
  // Delete the existing Faust module
  //FreeDSPFactory();
  WDL_String fileStr(file);

  mBitCodeStr.Set("");

  FILE* fp = fopen(file, "r");
  WDL_String data;

  if (fp)
  {
    long fileSize;
    
    fseek(fp , 0 , SEEK_END);
    fileSize = ftell(fp);
    rewind(fp);
    data.SetLen((int) fileSize);
    fread(data.Get(), fileSize, 1, fp);
    
    fclose(fp);
    
    StatType buf;
    GetStat(fileStr.Get(), &buf);
    mPreviousTime = GetModifiedTime(buf);
  }
  
  mSourceCodeStr.Set(data.Get());

  // Add path of file to library path
  fileStr.remove_filepart(true);
  AddLibraryPath(fileStr.Get());

  mInputDSPFile.Set(file);

  // Update all instances
  for (auto inst : mInstances)
  {
    inst->SourceCodeChanged();
  }
  
  return true; // TODO: return false if fail
}

bool FaustGen::Factory::WriteToFile(const char* file)
{
  return false;
}

void FaustGen::Factory::SetCompileOptions(std::initializer_list<const char*> options)
{
  DBGMSG("FaustGen: Compiler options modified for FaustGen\n");

  if (options.size() == 0)
    DBGMSG("FaustGen: No argument entered, no additional compilation option will be used");

  //TODO
//  mOptions = options;
//
//  /*
//  if (optimize) {
//
//    DBGMSG("FaustGen: Start looking for optimal compilation options...\n");
//
//  #ifdef OS_MAC
//    double best;
//    dsp_optimizer optimizer(string(mSourceCodeStr.Get()), (*mLibraryPaths.begin()).c_str(), getTarget(), sys_getblksize());
//    mOptions = optimizer.findOptimizedParameters(best);
//  #endif
//
//    DBGMSG("FaustGen: Optimal compilation options found\n");
//  }
//  */
//
//  // Delete the existing Faust module
//  FreeDSPFactory();
//  mBitCodeStr.Set("");
//
//  // Update all instances
//  for (auto i : fInstances) {
//    i->UpdateSourceCode();
//  }

}

#pragma mark -

FaustGen::FaustGen(IPlugBase& plug, const char* name, const char* inputDSPFile, int nVoices, int rate,
                   const char* outputCPPFile, const char* drawPath, const char* libraryPath)
: IPlugFaust(plug, name, nVoices, rate)
{
  sFaustGenCounter++;

  //if a factory doesn't already exist for this name, create one otherwise set mFactory to the existing one
  if (FaustGen::Factory::sFactoryMap.find(name) != FaustGen::Factory::sFactoryMap.end())
  {
    mFactory = FaustGen::Factory::sFactoryMap[name];
  }
  else
  {
    mFactory = new Factory(name, libraryPath, drawPath, inputDSPFile);
    FaustGen::Factory::sFactoryMap[name] = mFactory;
  }

  mFactory->AddInstance(this);
}

FaustGen::~FaustGen()
{
  if (--sFaustGenCounter <= 0)
  {
    EnableTimer(false);
  }

  FreeDSP();

  if(mFactory)
    mFactory->RemoveInstance(this);
}

void FaustGen::SourceCodeChanged()
{
  Init(true);
}

void FaustGen::Init(int maxNInputs, int maxNOutputs)
{
  mZones.Empty(); // remove existing pointers to zones
  
  mDSP = mFactory->GetDSP();
  assert(mDSP);

//    AddMidiHandler();
//    mDSP->buildUserInterface(mMidiUI);
  mDSP->buildUserInterface(this);
  mDSP->init(DEFAULT_SAMPLE_RATE);

  if ((mFactory->mNInputs != mDSP->getNumInputs()) || (mFactory->mNOutputs != mDSP->getNumOutputs()))
  {
    //TODO: do something when I/O is wrong
  }
  
  BuildParameterMap(); // build a new map based on updated code
  mInitialized = true;
  
  mPlug.OnParamReset(EParamSource::kRecompile);
}

void FaustGen::GetDrawPath(WDL_String& path)
{
  assert(!CStringHasContents(mFactory->mDrawPath.Get()));

  path.SetFormatted(MAX_WIN32_PATH_LEN, "%sFaustGen-%d-svg/process.svg", mFactory->mDrawPath.Get(), mFactory->mInstanceIdx);
}

bool FaustGen::CompileCPP()
{
  WDL_String archFile;
  archFile.Set(__FILE__);
  archFile.remove_filepart(true);
  archFile.Append("IPlugFaust_arch.cpp");

  WDL_String command;
  WDL_String inputFile;
  WDL_String outputFile;
//  WDL_String outputFiles;

  for (auto f : Factory::sFactoryMap)
  {
    inputFile = f.second->mInputDSPFile;
    outputFile = inputFile;
    outputFile.remove_fileext();
    outputFile.AppendFormatted(1024, ".tmp");
//    outputFiles.AppendFormatted(1024, "%s ", outputFile.Get());
    command.SetFormatted(1024, "%s -cn %s%s -double -i -a %s %s -o %s", FAUST_EXE, FAUST_CLASS_PREFIX, f.second->mName.Get(), archFile.Get(), inputFile.Get(), outputFile.Get());

    DBGMSG("Executing faust shell command: %s\n", command.Get());

    if(system(command.Get()) > -1)
    {
      // BSD sed, may not work on linux
      command.SetFormatted(1024, "sed -i \"\" \"s/FaustGen/%s/g\" %s", f.second->mName.Get(), outputFile.Get());

      DBGMSG("Executing sed shell command: %s\n", command.Get());

      if(system(command.Get()) == -1)
      {
        DBGMSG("Error executing sed\n");

        return false;
      }
    }
    else
    {
      return false;
    }
  }

  WDL_String folder = inputFile;
  folder.remove_filepart(true);
  WDL_String finalOutput = folder;
  finalOutput.AppendFormatted(1024, "FaustCode.hpp");
//  command.SetFormatted(1024, "cat %s > %s", outputFiles.Get(), finalOutput.Get());
  command.SetFormatted(1024, "cat %s*.tmp > %s", folder.Get(), finalOutput.Get());

  if(system(command.Get()) == -1)
  {
    DBGMSG("Error concatanating files\n");

    return false;
  }

  // TODO: annoying we have to do this to avoid min/max problems
  // BSD sed, may not work on linux
  command.SetFormatted(1024, "sed -i \"\" \"s/min(/std::min(/g\" %s", finalOutput.Get());

  DBGMSG("Executing sed shell command: %s\n", command.Get());

  if(system(command.Get()) == -1)
  {
    DBGMSG("Error executing sed\n");

    return false;
  }

  // BSD sed, may not work on linux
  command.SetFormatted(1024, "sed -i \"\" \"s/max(/std::max(/g\" %s", finalOutput.Get());

  DBGMSG("Executing sed shell command: %s\n", command.Get());

  if(system(command.Get()) == -1)
  {
    DBGMSG("Error executing sed\n");

    return false;
  }

//  command.SetFormatted(1024, "rm %s", outputFiles.Get());
  command.SetFormatted(1024, "rm %s*.tmp", folder.Get());

  if(system(command.Get()) == -1)
  {
    DBGMSG("Error removing output files\n");

    return false;
  }

  return true;
}

void FaustGen::onTimer(Steinberg::Timer* pTimer)
{
  WDL_String* pInputFile;
  bool recompile = false;

  for (auto f : Factory::sFactoryMap)
  {
    pInputFile = &f.second->mInputDSPFile;
    StatType buf;
    GetStat(pInputFile->Get(), &buf);
    Time oldTime = f.second->mPreviousTime;
    Time newTime = GetModifiedTime(buf);

    if(!Equal(newTime, oldTime))
    {
      recompile = true;
      f.second->FreeDSPFactory();
      DBGMSG("File change detected----------------------------------\n");
      DBGMSG("Dynamically compiling %s\n", pInputFile->Get());
      f.second->LoadFile(pInputFile->Get());
      Init();
    }
      
    f.second->mPreviousTime = newTime;
  }

  if(recompile)
  {
    DBGMSG("Statically compiling all FAUST BLOCKS\n");
    CompileCPP();
  }
}

void FaustGen::EnableTimer(bool enable)
{
  if(enable)
  {
    if(sTimer == nullptr)
      sTimer = Steinberg::Timer::create(this, FAUST_RECOMPILE_INTERVAL);
  }
  else
  {
    if(sTimer != nullptr)
    {
      sTimer->stop();
      sTimer->release();
      sTimer = nullptr;
    }
  }
  
  sEnableTimer = enable;
}

#endif // #ifndef FAUST_COMPILED

