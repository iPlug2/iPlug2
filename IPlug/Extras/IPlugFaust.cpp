#include "IPlugFaust.h"

#include "faust/dsp/libfaust.h"

#define LLVM_DSP
#include "faust/dsp/poly-dsp.h"

#ifndef OS_WIN
#include "faust/sound-file.h"
#endif

int FaustFactory::gFaustGenCounter = 0;
map<string, FaustFactory *> FaustFactory::gFactoryMap;
std::list<GUI*> GUI::fGuiList;
ztimedmap GUI::gTimedZoneMap;

FaustFactory::FaustFactory(const char* name, const char* libraryPath, const char* drawPath)
{
  mName.Set(name);
  mInstanceID = gFaustGenCounter++;
//  mMidiHandler.start_midi();

  mLibraryPaths.Add(libraryPath);
  mDrawPath.Set(drawPath);
}

FaustFactory::~FaustFactory()
{
  FreeDSPFactory();
  mSourceCodeStr.Set("");
  mBitCodeStr.Set("");
//  mMidiHandler.stop_midi();
}

void FaustFactory::FreeDSPFactory()
{
  WDL_MutexLock lock(&mDSPMutex);

  for (auto inst : mInstances)
  {
    inst->FreeDSP();
  }

  deleteDSPFactory(mFactory); // this is commented in faustgen~
  mFactory = nullptr;
}

llvm_dsp_factory *FaustFactory::CreateFactoryFromBitCode()
{
  //return readDSPFactoryFromBitCodeStr(mBitCodeStr.Get(), getTarget(), mOptimizationLevel);

  // Alternate model using machine code
  return readDSPFactoryFromMachine(mBitCodeStr.Get(), GetLLVMArchStr());

  /*
  // Alternate model using LLVM IR
  return readDSPFactoryFromIR(mBitCodeStr.Get(), getTarget(), mOptimizationLevel);
  */
}

llvm_dsp_factory *FaustFactory::CreateFactoryFromSourceCode()
{
  WDL_String name;
  name.SetFormatted(64, "FaustGen-%d", mInstanceID);

  SetDefaultCompileOptions();
  PrintCompileOptions();

  // Prepare compile options
  string error;
  const char* argv[64];

  const int N = (int) mCompileOptions.size();

  assert(N < 64);

  for (auto i = 0; i< N; i++)
  {
    argv[i] = mCompileOptions[i];
  }

  // Generate SVG file
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

::dsp *FaustFactory::CreateDSPInstance(int nVoices)
{
  ::dsp* pMonoDSP = mFactory->createDSPInstance();

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

::dsp *FaustFactory::CreateDSPAux(const char* str)
{
  ::dsp* pDSP = nullptr;
  FMeta meta;
  std::string error;
  
  //init with str
  if(str)
    mSourceCodeStr.Set(str);

  // Factory already allocated
  if (mFactory)
  {
    pDSP = CreateDSPInstance();
    DBGMSG("FaustGen: Factory already allocated, %i input(s), %i output(s)\n", pDSP->getNumInputs(), pDSP->getNumOutputs());
    goto end;
  }

  // Tries to create from bitcode
  if (mBitCodeStr.GetLength())
  {
    mFactory = CreateFactoryFromBitCode();
    if (mFactory)
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
    mFactory = CreateFactoryFromSourceCode();
    if (mFactory)
    {
      pDSP = CreateDSPInstance();
      pDSP->metadata(&meta);
      DBGMSG("FaustGen: Compilation from source code succeeded, %i input(s), %i output(s)\n", pDSP->getNumInputs(), pDSP->getNumOutputs());
      goto end;
    }
  }

    // Otherwise creates default DSP keeping the same input/output number
#ifdef OS_WIN
  // Prepare compile options
  const char* argv[64];
  
  const int N = (int) mCompileOptions.size();
  
  assert(N < 64);
  
  for (auto i = 0; i< N; i++)
  {
    argv[i] = mCompileOptions[i];
  }

  argv[N] = "-l";
  argv[N + 1] = "llvm_math.ll";
  argv[N + 2] = 0; // NULL terminated argv

  mFactory = createDSPFactoryFromString("default", DEFAULT_SOURCE_CODE, N + 2, argv, getTarget(), error, 0);
#else
  mSourceCodeStr.Set(DEFAULT_SOURCE_CODE);
  mFactory = createDSPFactoryFromString("default", mSourceCodeStr.Get(), 0, 0, GetLLVMArchStr(), error, 0);
#endif
  pDSP = CreateDSPInstance();
  DBGMSG("FaustGen: Allocation of default DSP succeeded, %i input(s), %i output(s)\n", pDSP->getNumInputs(), pDSP->getNumOutputs());

end:
  assert(pDSP);
  mNDSPInputs = pDSP->getNumInputs();
  mNDSPOutputs = pDSP->getNumOutputs();
  // Prepare JSON
//  MakeJson(pDSP);
  return pDSP;
}

void FaustFactory::MakeJson(::dsp *dsp)
{
  // Prepare JSON
//  JSONUI builder(m_siginlets, m_sigoutlets);
//  dsp->buildUserInterface(&builder);
//  dsp->metadata(&builder);
//  fJSON = builder.JSON();
}

void FaustFactory::AddLibraryPath(const char* libraryPath)
{
  if (CStringHasContents(libraryPath))
  {
    if(!mLibraryPaths.Find(libraryPath))
      mLibraryPaths.Add(libraryPath);
  }
}

void FaustFactory::AddCompileOption(const char* key, const char* value)
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

void FaustFactory::PrintCompileOptions()
{
  if (mCompileOptions.size() > 0)
  {
    DBGMSG("FaustGen: Compile options\n");

    int idx = 0;
    
    for (auto c : mCompileOptions)
    {
      DBGMSG("\t\t%i: = %s\n", idx++, c);
    }
  }
}

void FaustFactory::SetDefaultCompileOptions()
{
  // Clear and set default value
  mCompileOptions.clear();

  if (sizeof(sample) == 8)
    AddCompileOption("-double");

  // All library paths
  for (auto i = 0; i< mLibraryPaths.GetSize(); i++)
  {
    AddCompileOption("-I", mLibraryPaths.Get()[i]);
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
    if (strcmp(c, "-opt") == 0)
    {
      mOptimizationLevel = atoi(c);
    }
    else
    {
      AddCompileOption(c);
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

void FaustFactory::UpdateSourceCode(const char* str)
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

void FaustFactory::RemoveInstance(FaustGen* pDSP)
{
  mInstances.erase(pDSP);

  // Last instance : remove factory from global table and commit suicide...
  if (mInstances.size() == 0)
  {
    gFactoryMap.erase(mName.Get());
    delete this;
  }
}

bool FaustFactory::LoadFile(const char* file)
{
  // Delete the existing Faust module
  FreeDSPFactory();

  mBitCodeStr.Set("");

//  mSourceCodeStr // load text file into here;

  // Add path of file to library path
//  AddLibraryPath();

  // Update all instances
  for (auto inst : mInstances)
  {
    inst->SourceCodeChanged();
  }

  return true; // TODO: return false if fail
}

bool FaustFactory::WriteToFile(const char* file)
{
  return false;
}

void FaustFactory::SetCompileOptions(std::initializer_list<const char*> options)
{
  DBGMSG("FaustGen: Compiler options modified for FaustGen\n");

  if (options.size() == 0) {
    DBGMSG("FaustGen: No argument entered, no additional compilation option will be used");
  }

  mOptions = options;
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

FaustGen::FaustGen(const char* name, const char* inputFile, const char* outputFile, const char* drawPath, const char* libraryPath)
{
  if (FaustFactory::gFactoryMap.find(name) != FaustFactory::gFactoryMap.end())
  {
    mFactory = FaustFactory::gFactoryMap[name];
  }
  else
  {
    mFactory = new FaustFactory(name, libraryPath, drawPath);
    FaustFactory::gFactoryMap[name] = mFactory;
  }
  
  mFactory->AddInstance(this);
}

FaustGen::~FaustGen()
{
  FreeDSP();

  // Has to be done *before* RemoveInstance that may free mFactory and thus mFactory->mMidiHandler
  //delete mMidiUI;

  if(mFactory)
    mFactory->RemoveInstance(this);
}

void FaustGen::FreeDSP()
{
//  RemoveMidiHandler();
  delete mDSP;
  mDSP = nullptr;
//  fDSPUI.clear();
}

void FaustGen::SourceCodeChanged()
{
//  Init();

  //  SetDirty();
}

void FaustGen::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  //TODO: implement thread safety mechanism / mutex to prevent errors if DSP is not compiled
  if (mDSP)
  {
    mDSP->compute(nFrames, inputs, outputs);
  }
}

void FaustGen::SetSampleRate(double sampleRate)
{
  mDSP->init(sampleRate);
}

void FaustGen::Init(const char* str)
{
  mDSP = mFactory->CreateDSPAux(str);
  assert(mDSP);

  //TODO: build user interface
  // Initialize User Interface (here connnection with controls)
//    mDSP->buildUserInterface(&mDSPUI);
//
//    AddMidiHandler();
//    mDSP->buildUserInterface(mMidiUI);

  mDSP->init(DEFAULT_SAMPLE_RATE);

  if ((mFactory->mNDSPInputs != mDSP->getNumInputs()) || (mFactory->mNDSPOutputs != mDSP->getNumOutputs()))
  {
    //TODO: do something when I/O is wrong
  }
}
