/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#ifndef FAUST_COMPILED



#include "IPlugFaustGen.h"
#include "IPlugUtilities.h"

#include "faust/dsp/libfaust.h"
#define LLVM_DSP
#include "faust/dsp/poly-dsp.h"
#include "fileread.h"

using namespace iplug;

int FaustGen::sFaustGenCounter = 0;
int FaustGen::Factory::sFactoryCounter = 0;
bool FaustGen::sAutoRecompile = false;
std::map<std::string, FaustGen::Factory *> FaustGen::Factory::sFactoryMap;
std::list<GUI*> GUI::fGuiList;
Timer* FaustGen::sTimer = nullptr;

FaustGen::Factory::Factory(const char* name, const char* libraryPath, const char* drawPath, const char* inputDSP)
{
  mPreviousTime = TimeZero();
  mName.Set(name);
  mInstanceIdx = sFactoryCounter++;
//  mMidiHandler.start_midi();

  AddLibraryPath(libraryPath);
  mDrawPath.Set(drawPath);
  
  if(inputDSP)
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

llvm_dsp_factory* FaustGen::Factory::CreateFactoryFromBitCode()
{
  //return readDSPFactoryFromBitCodeStr(mBitCodeStr.Get(), getTarget(), mOptimizationLevel);
  std::string err;
  
  // Alternate model using machine code
  llvm_dsp_factory* pFactory = readDSPFactoryFromMachine(mBitCodeStr.Get(), GetLLVMArchStr(), err);
  
  // TODO: Print error
  return pFactory;
  
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
  std::string error;
  const char* argv[64];

  const int N = (int) mCompileOptions.size();

  assert(N < 64);

  for (auto i = 0; i< N; i++)
  {
    argv[i] = mCompileOptions[i].c_str();
  }

  // Generate SVG file // this shouldn't get called if we not making SVGs
//  if (!generateAuxFilesFromString(name.Get(), mSourceCodeStr.Get(), N, argv, error))
//  {
//    DBGMSG("FaustGen-%s: Generate SVG error : %s\n", error.c_str());
//  }

  argv[N] = 0; // NULL terminated argv

  llvm_dsp_factory* pFactory = createDSPFactoryFromString(name.Get(), mSourceCodeStr.Get(), N, argv, GetLLVMArchStr(), error, mOptimizationLevel);

  assert(pFactory);

  if (pFactory)
  {
    // Update all instances
    for (auto inst : mInstances)
    {
      inst->SetErrored(false);
    }
    
    return pFactory;
  }
  else
  {
    // Update all instances
    for (auto inst : mInstances)
    {
      inst->SetErrored(true);
    }

    //WHAT IS THIS?
//    if (mInstances.begin() != mInstances.end())
//    {
//      (*mInstances.begin())->hilight_error(error);
//    }
    DBGMSG("FaustGen-%s: Invalid Faust code or compile options : %s\n", mName.Get(), error.c_str());
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

::dsp *FaustGen::Factory::GetDSP(int maxInputs, int maxOutputs)
{
  ::dsp* pDSP = nullptr;
  FMeta meta;
  std::string error;

  // Factory already allocated
  if (mLLVMFactory)
  {
    pDSP = CreateDSPInstance();
    DBGMSG("FaustGen-%s: Factory already allocated, %i input(s), %i output(s)\n", mName.Get(), pDSP->getNumInputs(), pDSP->getNumOutputs());
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
      DBGMSG("FaustGen-%s: Compilation from bitcode succeeded, %i input(s), %i output(s)\n", mName.Get(), pDSP->getNumInputs(), pDSP->getNumOutputs());
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
      DBGMSG("FaustGen-%s: Compilation from source code succeeded, %i input(s), %i output(s)\n", mName.Get(), pDSP->getNumInputs(), pDSP->getNumOutputs());
      goto end;
    }
  }

    // Otherwise creates default DSP keeping the same input/output number
  mSourceCodeStr.SetFormatted(256, maxInputs == 0 ? DEFAULT_SOURCE_CODE_FMT_STR_INSTRUMENT : DEFAULT_SOURCE_CODE_FMT_STR_FX, maxOutputs);
  mLLVMFactory = createDSPFactoryFromString("default", mSourceCodeStr.Get(), 0, 0, GetLLVMArchStr(), error, 0);

  pDSP = CreateDSPInstance();
  DBGMSG("FaustGen-%s: Allocation of default DSP succeeded, %i input(s), %i output(s)\n", mName.Get(), pDSP->getNumInputs(), pDSP->getNumOutputs());

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
    DBGMSG("FaustGen-%s: Compile options: ", mName.Get());

    for (auto c : mCompileOptions)
    {
      DBGMSG("%s ", c.c_str());
    }
    
    DBGMSG("\n");
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
  DBGMSG("FaustGen-%s: Updating source code %s...\n", mName.Get(), str);

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
      inst->Init();
    }
  }
  else
  {
    DBGMSG("FaustGen-%s: DSP code has not been changed...\n", mName.Get());
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

  WDL_FileRead infile(file);

  if (infile.IsOpen() == true)
  {
    std::vector<char> buffer(infile.GetSize() + 1); // +1 to have space for the terminating zero
    infile.Read(buffer.data(), static_cast<int>(infile.GetSize()));
    buffer[infile.GetSize()] = '\0'; // put in the string terminating zero

    StatType buf;
    GetStat(fileStr.Get(), &buf);
    mPreviousTime = GetModifiedTime(buf);

    mSourceCodeStr.Set(buffer.data());
    
    // Add path of file to library path
    fileStr.remove_filepart(true);
    AddLibraryPath(fileStr.Get());
    
    mInputDSPFile.Set(file);
    
    // Update all instances
    for (auto inst : mInstances)
    {
      inst->Init();
    }
    
    return true;
  }
  
  assert(0); // The FAUST_BLOCK file was not found // TODO: warning about codesign
  
  return false;
}

bool FaustGen::Factory::WriteToFile(const char* file)
{
  return false;
}

void FaustGen::Factory::SetCompileOptions(std::initializer_list<const char*> options)
{
  DBGMSG("FaustGen-%s: Compiler options modified for FaustGen\n", mName.Get());

  if (options.size() == 0)
    DBGMSG("FaustGen-%s: No argument entered, no additional compilation option will be used", mName.Get());

  //TODO
//  mOptions = options;
//
//  /*
//  if (optimize) {
//
//    DBGMSG("FaustGen-%s: Start looking for optimal compilation options...\n");
//
//  #ifdef OS_MAC
//    double best;
//    dsp_optimizer optimizer(string(mSourceCodeStr.Get()), (*mLibraryPaths.begin()).c_str(), getTarget(), sys_getblksize());
//    mOptions = optimizer.findOptimizedParameters(best);
//  #endif
//
//    DBGMSG("FaustGen-%s: Optimal compilation options found\n");
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

FaustGen::FaustGen(const char* name, const char* inputDSPFile, int nVoices, int rate,
                   const char* outputCPPFile, const char* drawPath, const char* libraryPath)
: IPlugFaust(name, nVoices, rate)
{
  sFaustGenCounter++;

#ifdef OS_WIN
  SetDllDirectoryA(FAUST_DLL_PATH);
#endif
  
  //if a factory doesn't already exist for this name, create one otherwise set mFactory to the existing one
  if (FaustGen::Factory::sFactoryMap.find(name) != FaustGen::Factory::sFactoryMap.end())
  {
    mFactory = FaustGen::Factory::sFactoryMap[name];
  }
  else
  {
    DBGMSG("FaustGen: attempting to load %s\n", inputDSPFile);
    mFactory = new Factory(name, libraryPath, drawPath, inputDSPFile);
    FaustGen::Factory::sFactoryMap[name] = mFactory;
  }

  mFactory->AddInstance(this);
}

FaustGen::~FaustGen()
{
  if (--sFaustGenCounter <= 0)
  {
    SetAutoRecompile(false);
  }

  FreeDSP();

  if(mFactory)
    mFactory->RemoveInstance(this);
}

void FaustGen::Init()
{
  mZones.Empty(); // remove existing pointers to zones
  
  mDSP = std::unique_ptr<::dsp>(mFactory->GetDSP(mMaxNInputs, mMaxNOutputs));
  assert(mDSP);

//    AddMidiHandler();
//    mDSP->buildUserInterface(mMidiUI);
  mDSP->buildUserInterface(this);
  mDSP->init(DEFAULT_SAMPLE_RATE);

  assert((mDSP->getNumInputs() <= mMaxNInputs) && (mDSP->getNumOutputs() <= mMaxNOutputs)); // don't have enough buffers to process the DSP
  
  if ((mFactory->mNInputs != mDSP->getNumInputs()) || (mFactory->mNOutputs != mDSP->getNumOutputs()))
  {
    //TODO: do something when I/O is wrong
  }
  
  BuildParameterMap(); // build a new map based on updated code
  mInitialized = true;
  
  if(mPlug)
    mPlug->OnParamReset(EParamSource::kRecompile);
  
  if(mOnCompileFunc)
    mOnCompileFunc();
}

void FaustGen::GetDrawPath(WDL_String& path)
{
  assert(!CStringHasContents(mFactory->mDrawPath.Get()));

  path.SetFormatted(MAX_WIN32_PATH_LEN, "%sFaustGen-%d-svg/process.svg", mFactory->mDrawPath.Get(), mFactory->mInstanceIdx);
}

bool FaustGen::CompileCPP()
{
//#ifndef OS_WIN
  WDL_String archFile;
  archFile.Set(__FILE__);
  archFile.remove_filepart(true);
  archFile.Append("IPlugFaust_arch.cpp");

  WDL_String command;
  WDL_String inputFile;
  WDL_String outputFile;

  for (auto f : Factory::sFactoryMap)
  {
    inputFile = f.second->mInputDSPFile;
    outputFile = inputFile;
    outputFile.remove_fileext();
    outputFile.AppendFormatted(1024, ".tmp");
    //-double
    command.SetFormatted(1024, "%s -cn %s -i -a %s -o %s %s", FAUST_EXE, f.second->mName.Get(), archFile.Get(), outputFile.Get(), inputFile.Get());

    DBGMSG("exec: %s\n", command.Get());

    if(system(command.Get()) == -1)
      return false;
  }

  WDL_String folder = inputFile;
  folder.remove_filepart(true);
  WDL_String finalOutput = folder;
  finalOutput.AppendFormatted(1024, "FaustCode.hpp");
#ifndef OS_WIN
  command.SetFormatted(1024, "cat %s*.tmp > %s", folder.Get(), finalOutput.Get());
#else
  command.SetFormatted(1024, "copy %s*.tmp %s", folder.Get(), finalOutput.Get());
#endif

  if(system(command.Get()) == -1)
  {
    DBGMSG("Error concatanating files %s %i\n", __FILE__, __LINE__);

    return false;
  }

#ifndef OS_WIN
  command.SetFormatted(1024, "rm %s*.tmp", folder.Get());
#else
  command.SetFormatted(1024, "del %s*.tmp", folder.Get());
#endif

  if(system(command.Get()) == -1)
  {
    DBGMSG("Error removing output files %s %i\n", __FILE__, __LINE__);

    return false;
  }

//#endif

  return true;
}

//bool FaustGen::CompileObjectFile(const char* fileName)
//{
//  if (!writeDSPFactoryToObjectcodeFile(mFactory, fileName, "")) {
//    return false;
//  }
//
//  return true;
//}

void FaustGen::OnTimer(Timer& timer)
{
  WDL_String* pInputFile;
  bool recompile = false;

  for (auto f : Factory::sFactoryMap)
  {
    pInputFile = &f.second->mInputDSPFile;
    StatType buf;
    GetStat(pInputFile->Get(), &buf);
    StatTime oldTime = f.second->mPreviousTime;
    StatTime newTime = GetModifiedTime(buf);

    if(!Equal(newTime, oldTime))
    {
      WDL_MutexLock lock(&mMutex);

      recompile = true;
      f.second->FreeDSPFactory();
      DBGMSG("FaustGen-%s: File change detected ----------------------------------\n", mName.Get());
      DBGMSG("FaustGen-%s: JIT compiling %s\n", mName.Get(), pInputFile->Get());
      f.second->LoadFile(pInputFile->Get());
      Init();
    }
      
    f.second->mPreviousTime = newTime;
  }

  if(recompile)
  {
    // TODO: should check for successfull JIT
    DBGMSG("FaustGen-%s: Statically compiling all FAUST blocks\n", mName.Get());
    CompileCPP();
    //WDL_String objFile;
    //objFile.Set(pInputFile);
    //objFile.remove_fileext();

//#ifdef OS_WIN
//    objFile.Append(".obj");
//#else
//    objFile.Append(".o");
//#endif
//
//    CompileObjectFile(objFile.Get());
  }
}

//static
void FaustGen::SetAutoRecompile(bool enable)
{
  if(enable)
  {
    if(sTimer == nullptr)
      sTimer = Timer::Create(std::bind(&FaustGen::OnTimer, this, std::placeholders::_1), FAUST_RECOMPILE_INTERVAL);
  }
  else
  {
    if(sTimer != nullptr)
    {
      sTimer->Stop();
      sTimer = nullptr;
    }
  }
  
  sAutoRecompile = enable;
}

void FaustGen::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  WDL_MutexLock lock(&mMutex);
  if(!mErrored)
    IPlugFaust::ProcessBlock(inputs, outputs, nFrames);
  else
    memset(outputs[0], 0, nFrames * mMaxNOutputs * sizeof(sample));
}

#endif // #ifndef FAUST_COMPILED

