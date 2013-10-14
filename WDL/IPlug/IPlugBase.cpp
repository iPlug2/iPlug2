#include "IPlugBase.h"
#ifndef OS_IOS
#include "IGraphics.h"
#include "IControl.h"
#endif
#include <math.h>
#include <stdio.h>
#include <time.h>
#include "../wdlendian.h"
#include "../base64encdec.h"

#ifndef VstInt32
  #ifdef WIN32
    typedef int VstInt32;
  #else
    #include <stdint.h>
    typedef int32_t VstInt32;
  #endif
#endif

const double DEFAULT_SAMPLE_RATE = 44100.0;

template <class SRC, class DEST>
void CastCopy(DEST* pDest, SRC* pSrc, int n)
{
  for (int i = 0; i < n; ++i, ++pDest, ++pSrc)
  {
    *pDest = (DEST) *pSrc;
  }
}

void GetVersionParts(int version, int* pVer, int* pMaj, int* pMin)
{
  *pVer = (version & 0xFFFF0000) >> 16;
  *pMaj = (version & 0x0000FF00) >> 8;
  *pMin = version & 0x000000FF;
}

int GetDecimalVersion(int version)
{
  int ver, rmaj, rmin;
  GetVersionParts(version, &ver, &rmaj, &rmin);
  return 10000 * ver + 100 * rmaj + rmin;
}

void GetVersionStr(int version, char* str)
{
  int ver, rmaj, rmin;
  GetVersionParts(version, &ver, &rmaj, &rmin);
  sprintf(str, "v%d.%d.%d", ver, rmaj, rmin);
}

#ifndef MAX_PATH
  #define MAX_PATH 1024
#endif

IPlugBase::IPlugBase(int nParams,
                     const char* channelIOStr,
                     int nPresets,
                     const char* effectName,
                     const char* productName,
                     const char* mfrName,
                     int vendorVersion,
                     int uniqueID,
                     int mfrID,
                     int latency,
                     bool plugDoesMidi,
                     bool plugDoesChunks,
                     bool plugIsInst,
                     EAPI plugAPI)
  : mUniqueID(uniqueID)
  , mMfrID(mfrID)
  , mVersion(vendorVersion)
  , mSampleRate(DEFAULT_SAMPLE_RATE)
  , mBlockSize(0)
  , mLatency(latency)
  , mHost(kHostUninit)
  , mHostVersion(0)
  , mStateChunks(plugDoesChunks)
  , mGraphics(0)
  , mCurrentPresetIdx(0)
  , mIsInst(plugIsInst)
  , mDoesMIDI(plugDoesMidi)
  , mAPI(plugAPI)
  , mIsBypassed(false)
  , mDelay(0)
{
  Trace(TRACELOC, "%s:%s", effectName, CurrentTime());

  mPreviousPath.Set("", MAX_PATH);

  for (int i = 0; i < nParams; ++i)
  {
    mParams.Add(new IParam);
  }

  for (int i = 0; i < nPresets; ++i)
  {
    mPresets.Add(new IPreset(i));
  }

  strcpy(mEffectName, effectName);
  strcpy(mProductName, productName);
  strcpy(mMfrName, mfrName);

  int nInputs = 0, nOutputs = 0;

  while (channelIOStr)
  {
    int nIn = 0, nOut = 0;
#ifndef NDEBUG
    bool channelIOStrValid = sscanf(channelIOStr, "%d-%d", &nIn, &nOut) == 2;
    assert(channelIOStrValid);
#else
    sscanf(channelIOStr, "%d-%d", &nIn, &nOut);
#endif
    nInputs = IPMAX(nInputs, nIn);
    nOutputs = IPMAX(nOutputs, nOut);
    mChannelIO.Add(new ChannelIO(nIn, nOut));
    channelIOStr = strstr(channelIOStr, " ");
    
    if (channelIOStr)
    {
      ++channelIOStr;
    }
  }

  mInData.Resize(nInputs);
  mOutData.Resize(nOutputs);
  
  double** ppInData = mInData.Get();

  for (int i = 0; i < nInputs; ++i, ++ppInData)
  {
    InChannel* pInChannel = new InChannel;
    pInChannel->mConnected = false;
    pInChannel->mSrc = ppInData;
    mInChannels.Add(pInChannel);
  }

  double** ppOutData = mOutData.Get();

  for (int i = 0; i < nOutputs; ++i, ++ppOutData)
  {
    OutChannel* pOutChannel = new OutChannel;
    pOutChannel->mConnected = false;
    pOutChannel->mDest = ppOutData;
    pOutChannel->mFDest = 0;
    mOutChannels.Add(pOutChannel);
  }
}

IPlugBase::~IPlugBase()
{
  TRACE;
  #ifndef OS_IOS
  DELETE_NULL(mGraphics);
  #endif
  mParams.Empty(true);
  mPresets.Empty(true);
  mInChannels.Empty(true);
  mOutChannels.Empty(true);
  mChannelIO.Empty(true);
  mInputBusLabels.Empty(true);
  mOutputBusLabels.Empty(true);
 
  if (mDelay) 
  {
    DELETE_NULL(mDelay);
  }
}

int IPlugBase::GetHostVersion(bool decimal)
{
  GetHost();
  if (decimal)
  {
    return GetDecimalVersion(mHostVersion);
  }
  return mHostVersion;
}

void IPlugBase::GetHostVersionStr(char* str)
{
  GetHost();
  GetVersionStr(mHostVersion, str);
}

bool IPlugBase::LegalIO(int nIn, int nOut)
{
  bool legal = false;
  int i, n = mChannelIO.GetSize();
  
  for (i = 0; i < n && !legal; ++i)
  {
    ChannelIO* pIO = mChannelIO.Get(i);
    legal = ((nIn < 0 || nIn == pIO->mIn) && (nOut < 0 || nOut == pIO->mOut));
  }
  
  Trace(TRACELOC, "%d:%d:%s", nIn, nOut, (legal ? "legal" : "illegal"));
  return legal;
}

void IPlugBase::LimitToStereoIO()
{
  int nIn = NInChannels(), nOut = NOutChannels();
  
  if (nIn > 2)
  {
    SetInputChannelConnections(2, nIn - 2, false);
  }
  
  if (nOut > 2)
  {
    SetOutputChannelConnections(2, nOut - 2, true);
  }
}

void IPlugBase::SetHost(const char* host, int version)
{
  mHost = LookUpHost(host);
  mHostVersion = version;

  char vStr[32];
  GetVersionStr(version, vStr);
  Trace(TRACELOC, "host_%sknown:%s:%s", (mHost == kHostUnknown ? "un" : ""), host, vStr);
}
#ifndef OS_IOS
void IPlugBase::AttachGraphics(IGraphics* pGraphics)
{
  if (pGraphics)
  {
    WDL_MutexLock lock(&mMutex);
    int i, n = mParams.GetSize();
    
    for (i = 0; i < n; ++i)
    {
      pGraphics->SetParameterFromPlug(i, GetParam(i)->GetNormalized(), true);
    }
    
    pGraphics->PrepDraw();
    mGraphics = pGraphics;
  }
}
#endif

// Decimal = VVVVRRMM, otherwise 0xVVVVRRMM.
int IPlugBase::GetEffectVersion(bool decimal)
{
  if (decimal)
  {
    return GetDecimalVersion(mVersion);
  }
  else
  {
    return mVersion;
  }
}

void IPlugBase::GetEffectVersionStr(char* str)
{
  GetVersionStr(mVersion, str);
#if defined _DEBUG
  strcat(str, "D");
#elif defined TRACER_BUILD
  strcat(str, "T");
#endif
}

const char* IPlugBase::GetAPIString()
{
  switch (GetAPI()) 
  {
    case kAPIVST2: return "VST2";
    case kAPIVST3: return "VST3";
    case kAPIAU: return "AU";
    case kAPIRTAS: return "RTAS";
    case kAPIAAX: return "AAX";
    case kAPISA: return "Standalone";
    default: return "";
  }
}

const char* IPlugBase::GetArchString()
{
#ifdef ARCH_64BIT
  return "x64";
#else
  return "x86";
#endif
}

double IPlugBase::GetSamplesPerBeat()
{
  double tempo = GetTempo();
  
  if (tempo > 0.0)
  {
    return GetSampleRate() * 60.0 / tempo;
  }
  
  return 0.0;
}

void IPlugBase::SetSampleRate(double sampleRate)
{
  mSampleRate = sampleRate;
}

void IPlugBase::SetBlockSize(int blockSize)
{
  if (blockSize != mBlockSize)
  {
    int i, nIn = NInChannels(), nOut = NOutChannels();
    
    for (i = 0; i < nIn; ++i)
    {
      InChannel* pInChannel = mInChannels.Get(i);
      pInChannel->mScratchBuf.Resize(blockSize);
      memset(pInChannel->mScratchBuf.Get(), 0, blockSize * sizeof(double));
    }
    
    for (i = 0; i < nOut; ++i)
    {
      OutChannel* pOutChannel = mOutChannels.Get(i);
      pOutChannel->mScratchBuf.Resize(blockSize);
      memset(pOutChannel->mScratchBuf.Get(), 0, blockSize * sizeof(double));
    }
    
    mBlockSize = blockSize;
  }
}

void IPlugBase::SetInputChannelConnections(int idx, int n, bool connected)
{
  int iEnd = IPMIN(idx + n, mInChannels.GetSize());
  
  for (int i = idx; i < iEnd; ++i)
  {
    InChannel* pInChannel = mInChannels.Get(i);
    pInChannel->mConnected = connected;
    
    if (!connected)
    {
      *(pInChannel->mSrc) = pInChannel->mScratchBuf.Get();
    }
  }
}

void IPlugBase::SetOutputChannelConnections(int idx, int n, bool connected)
{
  int iEnd = IPMIN(idx + n, mOutChannels.GetSize());
  
  for (int i = idx; i < iEnd; ++i)
  {
    OutChannel* pOutChannel = mOutChannels.Get(i);
    pOutChannel->mConnected = connected;
    
    if (!connected)
    {
      *(pOutChannel->mDest) = pOutChannel->mScratchBuf.Get();
    }
  }
}

bool IPlugBase::IsInChannelConnected(int chIdx)
{
  return (chIdx < mInChannels.GetSize() && mInChannels.Get(chIdx)->mConnected);
}

bool IPlugBase::IsOutChannelConnected(int chIdx)
{
  return (chIdx < mOutChannels.GetSize() && mOutChannels.Get(chIdx)->mConnected);
}

void IPlugBase::AttachInputBuffers(int idx, int n, double** ppData, int nFrames)
{
  int iEnd = IPMIN(idx + n, mInChannels.GetSize());
  
  for (int i = idx; i < iEnd; ++i)
  {
    InChannel* pInChannel = mInChannels.Get(i);
    if (pInChannel->mConnected)
    {
      *(pInChannel->mSrc) = *(ppData++);
    }
  }
}

void IPlugBase::AttachInputBuffers(int idx, int n, float** ppData, int nFrames)
{
  int iEnd = IPMIN(idx + n, mInChannels.GetSize());
  for (int i = idx; i < iEnd; ++i)
  {
    InChannel* pInChannel = mInChannels.Get(i);
    if (pInChannel->mConnected)
    {
      double* pScratch = pInChannel->mScratchBuf.Get();
      CastCopy(pScratch, *(ppData++), nFrames);
      *(pInChannel->mSrc) = pScratch;
    }
  }
}

void IPlugBase::AttachOutputBuffers(int idx, int n, double** ppData)
{
  int iEnd = IPMIN(idx + n, mOutChannels.GetSize());
  for (int i = idx; i < iEnd; ++i)
  {
    OutChannel* pOutChannel = mOutChannels.Get(i);
    if (pOutChannel->mConnected)
    {
      *(pOutChannel->mDest) = *(ppData++);
    }
  }
}

void IPlugBase::AttachOutputBuffers(int idx, int n, float** ppData)
{
  int iEnd = IPMIN(idx + n, mOutChannels.GetSize());
  for (int i = idx; i < iEnd; ++i)
  {
    OutChannel* pOutChannel = mOutChannels.Get(i);
    if (pOutChannel->mConnected)
    {
      *(pOutChannel->mDest) = pOutChannel->mScratchBuf.Get();
      pOutChannel->mFDest = *(ppData++);
    }
  }
}

void IPlugBase::PassThroughBuffers(double sampleType, int nFrames)
{
  if (mLatency && mDelay) 
  {
    mDelay->ProcessBlock(mInData.Get(), mOutData.Get(), nFrames);
  }
  else 
  {
    IPlugBase::ProcessDoubleReplacing(mInData.Get(), mOutData.Get(), nFrames);
  }
}

void IPlugBase::PassThroughBuffers(float sampleType, int nFrames)
{
  // for 32 bit buffers, first run the delay (if mLatency) on the 64bit IPlug buffers
  PassThroughBuffers(0., nFrames);
  
  int i, n = NOutChannels();
  OutChannel** ppOutChannel = mOutChannels.GetList();
  
  for (i = 0; i < n; ++i, ++ppOutChannel)
  {
    OutChannel* pOutChannel = *ppOutChannel;
    if (pOutChannel->mConnected)
    {
      CastCopy(pOutChannel->mFDest, *(pOutChannel->mDest), nFrames);
    }
  }
}

void IPlugBase::ProcessBuffers(double sampleType, int nFrames)
{
  ProcessDoubleReplacing(mInData.Get(), mOutData.Get(), nFrames);
}

void IPlugBase::ProcessBuffers(float sampleType, int nFrames)
{
  ProcessDoubleReplacing(mInData.Get(), mOutData.Get(), nFrames);
  int i, n = NOutChannels();
  OutChannel** ppOutChannel = mOutChannels.GetList();
  
  for (i = 0; i < n; ++i, ++ppOutChannel)
  {
    OutChannel* pOutChannel = *ppOutChannel;
    
    if (pOutChannel->mConnected)
    {
      CastCopy(pOutChannel->mFDest, *(pOutChannel->mDest), nFrames);
    }
  }
}

void IPlugBase::ProcessBuffersAccumulating(float sampleType, int nFrames)
{
  ProcessDoubleReplacing(mInData.Get(), mOutData.Get(), nFrames);
  int i, n = NOutChannels();
  OutChannel** ppOutChannel = mOutChannels.GetList();
  
  for (i = 0; i < n; ++i, ++ppOutChannel)
  {
    OutChannel* pOutChannel = *ppOutChannel;
    if (pOutChannel->mConnected)
    {
      float* pDest = pOutChannel->mFDest;
      double* pSrc = *(pOutChannel->mDest);
      
      for (int j = 0; j < nFrames; ++j, ++pDest, ++pSrc)
      {
        *pDest += (float) *pSrc;
      }
    }
  }
}

void IPlugBase::ZeroScratchBuffers()
{
  int i, nIn = NInChannels(), nOut = NOutChannels();

  for (i = 0; i < nIn; ++i)
  {
    InChannel* pInChannel = mInChannels.Get(i);
    memset(pInChannel->mScratchBuf.Get(), 0, mBlockSize * sizeof(double));
  }

  for (i = 0; i < nOut; ++i)
  {
    OutChannel* pOutChannel = mOutChannels.Get(i);
    memset(pOutChannel->mScratchBuf.Get(), 0, mBlockSize * sizeof(double));
  }
}

// If latency changes after initialization (often not supported by the host).
void IPlugBase::SetLatency(int samples)
{
  mLatency = samples;
  
  if (mDelay) 
  {
    mDelay->SetDelayTime(mLatency);
  }
}

// this is over-ridden for AAX
void IPlugBase::SetParameterFromGUI(int idx, double normalizedValue)
{
  Trace(TRACELOC, "%d:%f", idx, normalizedValue);
  WDL_MutexLock lock(&mMutex);
  GetParam(idx)->SetNormalized(normalizedValue);
  InformHostOfParamChange(idx, normalizedValue);
  OnParamChange(idx);
}

void IPlugBase::OnParamReset()
{
  for (int i = 0; i < mParams.GetSize(); ++i)
  {
    OnParamChange(i);
  }
  //Reset();
}

// Default passthrough.
void IPlugBase::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
  // Mutex is already locked.
  int i, nIn = mInChannels.GetSize(), nOut = mOutChannels.GetSize();
  int j = 0;
  for (i = 0; i < nOut; ++i)
  {
    if (i < nIn)
    {
      memcpy(outputs[i], inputs[i], nFrames * sizeof(double));
      j++;
    }
  }
  // zero remaining outs
  for (/* same j */; j < nOut; ++j)
  {
    memset(outputs[j], 0, nFrames * sizeof(double));
  }
}

// Default passthrough ONLY USED BY IOS.
void IPlugBase::ProcessSingleReplacing(float** inputs, float** outputs, int nFrames)
{
  // Mutex is already locked.
  int i, nIn = mInChannels.GetSize(), nOut = mOutChannels.GetSize();
  for (i = 0; i < nIn; ++i)
  {
    memcpy(outputs[i], inputs[i], nFrames * sizeof(float));
  }
  for (/* same i */; i < nOut; ++i)
  {
    memset(outputs[i], 0, nFrames * sizeof(float));
  }
}

// Default passthrough.
void IPlugBase::ProcessMidiMsg(IMidiMsg* pMsg)
{
  SendMidiMsg(pMsg);
}

IPreset* GetNextUninitializedPreset(WDL_PtrList<IPreset>* pPresets)
{
  int n = pPresets->GetSize();
  for (int i = 0; i < n; ++i)
  {
    IPreset* pPreset = pPresets->Get(i);
    if (!(pPreset->mInitialized))
    {
      return pPreset;
    }
  }
  return 0;
}

void IPlugBase::MakeDefaultPreset(char* name, int nPresets)
{
  for (int i = 0; i < nPresets; ++i)
  {
    IPreset* pPreset = GetNextUninitializedPreset(&mPresets);
    if (pPreset)
    {
      pPreset->mInitialized = true;
      strcpy(pPreset->mName, (name ? name : "Empty"));
      SerializeState(&(pPreset->mChunk));
    }
  }
}

#define GET_PARAM_FROM_VARARG(paramType, vp, v) \
{ \
  v = 0.0; \
  switch (paramType) { \
    case IParam::kTypeBool: \
    case IParam::kTypeInt: \
    case IParam::kTypeEnum: { \
      v = (double) va_arg(vp, int); \
      break; \
    } \
    case IParam::kTypeDouble: \
    default: { \
      v = (double) va_arg(vp, double); \
      break; \
    } \
  } \
}

void IPlugBase::MakePreset(char* name, ...)
{
  IPreset* pPreset = GetNextUninitializedPreset(&mPresets);
  if (pPreset)
  {
    pPreset->mInitialized = true;
    strcpy(pPreset->mName, name);

    int i, n = mParams.GetSize();

    double v = 0.0;
    va_list vp;
    va_start(vp, name);
    for (i = 0; i < n; ++i)
    {
      GET_PARAM_FROM_VARARG(GetParam(i)->Type(), vp, v);
      pPreset->mChunk.Put(&v);
    }
  }
}

#define PARAM_UNINIT 99.99e-9

void IPlugBase::MakePresetFromNamedParams(char* name, int nParamsNamed, ...)
{
  TRACE;
  IPreset* pPreset = GetNextUninitializedPreset(&mPresets);
  if (pPreset)
  {
    pPreset->mInitialized = true;
    strcpy(pPreset->mName, name);

    int i = 0, n = mParams.GetSize();

    WDL_TypedBuf<double> vals;
    vals.Resize(n);
    double* pV = vals.Get();
    for (i = 0; i < n; ++i, ++pV)
    {
      *pV = PARAM_UNINIT;
    }

    va_list vp;
    va_start(vp, nParamsNamed);
    for (int i = 0; i < nParamsNamed; ++i)
    {
      int paramIdx = (int) va_arg(vp, int);
      // This assert will fire if any of the passed-in param values do not match
      // the type that the param was initialized with (int for bool, int, enum; double for double).
      assert(paramIdx >= 0 && paramIdx < n);
      GET_PARAM_FROM_VARARG(GetParam(paramIdx)->Type(), vp, *(vals.Get() + paramIdx));
    }
    va_end(vp);

    pV = vals.Get();
    for (int i = 0; i < n; ++i, ++pV)
    {
      if (*pV == PARAM_UNINIT)        // Any that weren't explicitly set, use the defaults.
      {
        *pV = GetParam(i)->Value();
      }
      pPreset->mChunk.Put(pV);
    }
  }
}

void IPlugBase::MakePresetFromChunk(char* name, ByteChunk* pChunk)
{
  IPreset* pPreset = GetNextUninitializedPreset(&mPresets);
  if (pPreset)
  {
    pPreset->mInitialized = true;
    strcpy(pPreset->mName, name);

    pPreset->mChunk.PutChunk(pChunk);
  }
}

void IPlugBase::MakePresetFromBlob(char* name, const char* blob, int sizeOfChunk)
{
  ByteChunk presetChunk;
  presetChunk.Resize(sizeOfChunk);
  base64decode(blob, presetChunk.GetBytes(), sizeOfChunk);

  MakePresetFromChunk(name, &presetChunk);
}

#define DEFAULT_USER_PRESET_NAME "user preset"

void MakeDefaultUserPresetName(WDL_PtrList<IPreset>* pPresets, char* str)
{
  int nDefaultNames = 0;
  int n = pPresets->GetSize();
  for (int i = 0; i < n; ++i)
  {
    IPreset* pPreset = pPresets->Get(i);
    if (strstr(pPreset->mName, DEFAULT_USER_PRESET_NAME))
    {
      ++nDefaultNames;
    }
  }
  sprintf(str, "%s %d", DEFAULT_USER_PRESET_NAME, nDefaultNames + 1);
}

void IPlugBase::EnsureDefaultPreset()
{
  TRACE;
  MakeDefaultPreset("Empty", mPresets.GetSize());
}

void IPlugBase::PruneUninitializedPresets()
{
  TRACE;
  int i = 0;
  while (i < mPresets.GetSize())
  {
    IPreset* pPreset = mPresets.Get(i);
    if (pPreset->mInitialized)
    {
      ++i;
    }
    else
    {
      mPresets.Delete(i, true);
    }
  }
}

bool IPlugBase::RestorePreset(int idx)
{
  TRACE;
  bool restoredOK = false;
  if (idx >= 0 && idx < mPresets.GetSize())
  {
    IPreset* pPreset = mPresets.Get(idx);

    if (!(pPreset->mInitialized))
    {
      pPreset->mInitialized = true;
      MakeDefaultUserPresetName(&mPresets, pPreset->mName);
      restoredOK = SerializeState(&(pPreset->mChunk));
    }
    else
    {
      restoredOK = (UnserializeState(&(pPreset->mChunk), 0) > 0);
    }

    if (restoredOK)
    {
      mCurrentPresetIdx = idx;
      PresetsChangedByHost();
      #ifndef OS_IOS
      RedrawParamControls();
      #endif
    }
  }
  return restoredOK;
}

bool IPlugBase::RestorePreset(const char* name)
{
  if (CSTR_NOT_EMPTY(name))
  {
    int n = mPresets.GetSize();
    for (int i = 0; i < n; ++i)
    {
      IPreset* pPreset = mPresets.Get(i);
      if (!strcmp(pPreset->mName, name))
      {
        return RestorePreset(i);
      }
    }
  }
  return false;
}

const char* IPlugBase::GetPresetName(int idx)
{
  if (idx >= 0 && idx < mPresets.GetSize())
  {
    return mPresets.Get(idx)->mName;
  }
  return "";
}

void IPlugBase::ModifyCurrentPreset(const char* name)
{
  if (mCurrentPresetIdx >= 0 && mCurrentPresetIdx < mPresets.GetSize())
  {
    IPreset* pPreset = mPresets.Get(mCurrentPresetIdx);
    pPreset->mChunk.Clear();

    Trace(TRACELOC, "%d %s", mCurrentPresetIdx, pPreset->mName);

    SerializeState(&(pPreset->mChunk));

    if (CSTR_NOT_EMPTY(name))
    {
      strcpy(pPreset->mName, name);
    }
  }
}

bool IPlugBase::SerializePresets(ByteChunk* pChunk)
{
  TRACE;
  bool savedOK = true;
  int n = mPresets.GetSize();
  for (int i = 0; i < n && savedOK; ++i)
  {
    IPreset* pPreset = mPresets.Get(i);
    pChunk->PutStr(pPreset->mName);

    Trace(TRACELOC, "%d %s", i, pPreset->mName);

    pChunk->PutBool(pPreset->mInitialized);
    if (pPreset->mInitialized)
    {
      savedOK &= (pChunk->PutChunk(&(pPreset->mChunk)) > 0);
    }
  }
  return savedOK;
}

int IPlugBase::UnserializePresets(ByteChunk* pChunk, int startPos)
{
  TRACE;
  WDL_String name;
  int n = mPresets.GetSize(), pos = startPos;
  for (int i = 0; i < n && pos >= 0; ++i)
  {
    IPreset* pPreset = mPresets.Get(i);
    pos = pChunk->GetStr(&name, pos);
    strcpy(pPreset->mName, name.Get());

    Trace(TRACELOC, "%d %s", i, pPreset->mName);

    pos = pChunk->GetBool(&(pPreset->mInitialized), pos);
    if (pPreset->mInitialized)
    {
      pos = UnserializeState(pChunk, pos);
      if (pos > 0)
      {
        pPreset->mChunk.Clear();
        SerializeState(&(pPreset->mChunk));
      }
    }
  }
  RestorePreset(mCurrentPresetIdx);
  return pos;
}

bool IPlugBase::SerializeParams(ByteChunk* pChunk)
{
  TRACE;

  WDL_MutexLock lock(&mMutex);
  bool savedOK = true;
  int i, n = mParams.GetSize();
  for (i = 0; i < n && savedOK; ++i)
  {
    IParam* pParam = mParams.Get(i);
    Trace(TRACELOC, "%d %s %f", i, pParam->GetNameForHost(), pParam->Value());
    double v = pParam->Value();
    savedOK &= (pChunk->Put(&v) > 0);
  }
  return savedOK;
}

int IPlugBase::UnserializeParams(ByteChunk* pChunk, int startPos)
{
  TRACE;

  WDL_MutexLock lock(&mMutex);
  int i, n = mParams.GetSize(), pos = startPos;
  for (i = 0; i < n && pos >= 0; ++i)
  {
    IParam* pParam = mParams.Get(i);
    double v = 0.0;
    Trace(TRACELOC, "%d %s %f", i, pParam->GetNameForHost(), pParam->Value());
    pos = pChunk->Get(&v, pos);
    pParam->Set(v);
  }
  OnParamReset();
  return pos;
}

bool IPlugBase::CompareState(const unsigned char* incomingState, int startPos)
{
  bool isEqual = true;
  
  const double* data = (const double*) incomingState + startPos;
  
  // dirty hack here because protools treats param values as 32 bit int and in IPlug they are 64bit float
  // if we memcmp() the incoming state with the current they may have tiny differences due to the quantization
  for (int i = 0; i < NParams(); i++)
  {
    float v = (float) GetParam(i)->Value();
    float vi = (float) *(data++);
    
    isEqual &= (fabsf(v - vi) < 0.00001);
  }
  
  return isEqual;
}

#ifndef OS_IOS
void IPlugBase::RedrawParamControls()
{
  if (mGraphics)
  {
    int i, n = mParams.GetSize();
    for (i = 0; i < n; ++i)
    {
      double v = mParams.Get(i)->Value();
      mGraphics->SetParameterFromPlug(i, v, false);
    }
  }
}
#endif
void IPlugBase::DirtyParameters()
{
  WDL_MutexLock lock(&mMutex);

  for (int p = 0; p < NParams(); p++)
  {
    double normalizedValue = GetParam(p)->GetNormalized();
    InformHostOfParamChange(p, normalizedValue);
  }
}

void IPlugBase::DumpPresetSrcCode(const char* filename, const char* paramEnumNames[])
{
// static bool sDumped = false;
  bool sDumped = false;

  if (!sDumped)
  {
    sDumped = true;
    int i, n = NParams();
    FILE* fp = fopen(filename, "w");
    fprintf(fp, "  MakePresetFromNamedParams(\"name\", %d", n);
    for (i = 0; i < n; ++i)
    {
      IParam* pParam = GetParam(i);
      char paramVal[32];
      switch (pParam->Type())
      {
        case IParam::kTypeBool:
          sprintf(paramVal, "%s", (pParam->Bool() ? "true" : "false"));
          break;
        case IParam::kTypeInt:
          sprintf(paramVal, "%d", pParam->Int());
          break;
        case IParam::kTypeEnum:
          sprintf(paramVal, "%d", pParam->Int());
          break;
        case IParam::kTypeDouble:
        default:
          sprintf(paramVal, "%.6f", pParam->Value());
          break;
      }
      fprintf(fp, ",\n    %s, %s", paramEnumNames[i], paramVal);
    }
    fprintf(fp, ");\n");
    fclose(fp);
  }
}

#ifndef MAX_BLOB_LENGTH
#define MAX_BLOB_LENGTH 1024
#endif

void IPlugBase::DumpPresetBlob(const char* filename)
{
  FILE* fp = fopen(filename, "w");
  fprintf(fp, "MakePresetFromBlob(\"name\", \"");

  char buf[MAX_BLOB_LENGTH];

  ByteChunk* pPresetChunk = &mPresets.Get(mCurrentPresetIdx)->mChunk;
  BYTE* byteStart = pPresetChunk->GetBytes();

  base64encode(byteStart, buf, pPresetChunk->Size());

  fprintf(fp, "%s\", %i);\n", buf, pPresetChunk->Size());
  fclose(fp);
}

void IPlugBase::SetInputLabel(int idx, const char* pLabel)
{
  if (idx >= 0 && idx < NInChannels())
  {
    mInChannels.Get(idx)->mLabel.Set(pLabel);
  }
}

void IPlugBase::SetOutputLabel(int idx, const char* pLabel)
{
  if (idx >= 0 && idx < NOutChannels())
  {
    mOutChannels.Get(idx)->mLabel.Set(pLabel);
  }
}

void IPlugBase::SetInputBusLabel(int idx, const char* pLabel)
{
  if (idx >= 0 && idx < 2) // only possible to have two input buses
  {
    if (mInputBusLabels.Get(idx))
    {
      mInputBusLabels.Delete(idx, true);
    }

    mInputBusLabels.Insert(idx, new WDL_String(pLabel, strlen(pLabel)));
  }
}

void IPlugBase::SetOutputBusLabel(int idx, const char* pLabel)
{
  if (idx >= 0)
  {
    if (mOutputBusLabels.Get(idx))
    {
      mOutputBusLabels.Delete(idx, true);
    }

    mOutputBusLabels.Insert(idx, new WDL_String(pLabel, strlen(pLabel)));
  }
}

const int kFXPVersionNum = 1;
const int kFXBVersionNum = 2;

// confusing... bytechunk will force storage as little endian on big endian platforms,
// so when we use it here, since vst fxp/fxb files are big endian, we need to swap the endianess
// regardless of the endianness of the host, and on big endian hosts it will get swapped back to
// big endian
#ifndef OS_IOS
bool IPlugBase::SaveProgramAsFXP(const char* defaultFileName)
{
  if (mGraphics)
  {
    WDL_String fileName(defaultFileName, strlen(defaultFileName));
    mGraphics->PromptForFile(&fileName, kFileSave, &mPreviousPath, "fxp");

    if (fileName.GetLength())
    {
      FILE* fp = fopen(fileName.Get(), "wb");

      ByteChunk pgm;

      VstInt32 chunkMagic = WDL_bswap32('CcnK');
      VstInt32 byteSize = 0;
      VstInt32 fxpMagic;
      VstInt32 fxpVersion = WDL_bswap32(kFXPVersionNum);
      VstInt32 pluginID = WDL_bswap32(GetUniqueID());
      VstInt32 pluginVersion = WDL_bswap32(GetEffectVersion(true));
      VstInt32 numParams = WDL_bswap32(NParams());
      char prgName[28];
      memset(prgName, 0, 28);
      strcpy(prgName, GetPresetName(GetCurrentPresetIdx()));

      pgm.Put(&chunkMagic);

      if (DoesStateChunks())
      {
        ByteChunk state;
        VstInt32 chunkSize;

        fxpMagic = WDL_bswap32('FPCh');

        InitChunkWithIPlugVer(&state);
        SerializeState(&state);

        chunkSize = WDL_bswap32(state.Size());
        byteSize = WDL_bswap32(state.Size() + 60);

        pgm.Put(&byteSize);
        pgm.Put(&fxpMagic);
        pgm.Put(&fxpVersion);
        pgm.Put(&pluginID);
        pgm.Put(&pluginVersion);
        pgm.Put(&numParams);
        pgm.PutBytes(prgName, 28); // not PutStr (we want all 28 bytes)
        pgm.Put(&chunkSize);
        pgm.PutBytes(state.GetBytes(), state.Size());
      }
      else
      {
        fxpMagic = WDL_bswap32('FxCk');
        //byteSize = WDL_bswap32(20 + 28 + (NParams() * 4) );
        pgm.Put(&byteSize);
        pgm.Put(&fxpMagic);
        pgm.Put(&fxpVersion);
        pgm.Put(&pluginID);
        pgm.Put(&pluginVersion);
        pgm.Put(&numParams);
        pgm.PutBytes(prgName, 28); // not PutStr (we want all 28 bytes)

        for (int i = 0; i< NParams(); i++)
        {
          WDL_EndianFloat v32;
          v32.f = (float) mParams.Get(i)->GetNormalized();
          unsigned int swapped = WDL_bswap32(v32.int32);
          pgm.Put(&swapped);
        }
      }

      fwrite(pgm.GetBytes(), pgm.Size(), 1, fp);
      fclose(fp);

      return true;
    }
  }
  return false;
}

bool IPlugBase::SaveBankAsFXB(const char* defaultFileName)
{
  if (mGraphics)
  {
    WDL_String fileName(defaultFileName, strlen(defaultFileName));
    mGraphics->PromptForFile(&fileName, kFileSave, &mPreviousPath, "fxb");

    if (fileName.GetLength())
    {
      FILE* fp = fopen(fileName.Get(), "wb");

      ByteChunk bnk;

      VstInt32 chunkMagic = WDL_bswap32('CcnK');
      VstInt32 byteSize = 0;
      VstInt32 fxbMagic;
      VstInt32 fxbVersion = WDL_bswap32(kFXBVersionNum);
      VstInt32 pluginID = WDL_bswap32(GetUniqueID());
      VstInt32 pluginVersion = WDL_bswap32(GetEffectVersion(true));
      VstInt32 numPgms =  WDL_bswap32(NPresets());
      VstInt32 currentPgm = WDL_bswap32(GetCurrentPresetIdx());
      char future[124];
      memset(future, 0, 124);

      bnk.Put(&chunkMagic);

      if (DoesStateChunks())
      {
        ByteChunk state;
        VstInt32 chunkSize;

        fxbMagic = WDL_bswap32('FBCh');

        InitChunkWithIPlugVer(&state);
        SerializePresets(&state);

        chunkSize = WDL_bswap32(state.Size());
        byteSize = WDL_bswap32(160 + state.Size() );

        bnk.Put(&byteSize);
        bnk.Put(&fxbMagic);
        bnk.Put(&fxbVersion);
        bnk.Put(&pluginID);
        bnk.Put(&pluginVersion);
        bnk.Put(&numPgms);
        bnk.Put(&currentPgm);
        bnk.PutBytes(&future, 124);

        bnk.Put(&chunkSize);
        bnk.PutBytes(state.GetBytes(), state.Size());
      }
      else
      {
        fxbMagic = WDL_bswap32('FxBk');

        bnk.Put(&byteSize);
        bnk.Put(&fxbMagic);
        bnk.Put(&fxbVersion);
        bnk.Put(&pluginID);
        bnk.Put(&pluginVersion);
        bnk.Put(&numPgms);
        bnk.Put(&currentPgm);
        bnk.PutBytes(&future, 124);

        VstInt32 fxpMagic = WDL_bswap32('FxCk');
        VstInt32 fxpVersion = WDL_bswap32(kFXPVersionNum);
        VstInt32 numParams = WDL_bswap32(NParams());

        for (int p = 0; p < NPresets(); p++)
        {
          IPreset* pPreset = mPresets.Get(p);

          char prgName[28];
          memset(prgName, 0, 28);
          strcpy(prgName, pPreset->mName);

          bnk.Put(&chunkMagic);
          //byteSize = WDL_bswap32(20 + 28 + (NParams() * 4) );
          bnk.Put(&byteSize);
          bnk.Put(&fxpMagic);
          bnk.Put(&fxpVersion);
          bnk.Put(&pluginID);
          bnk.Put(&pluginVersion);
          bnk.Put(&numParams);
          bnk.PutBytes(prgName, 28);

          int pos = 0;

          for (int i = 0; i< NParams(); i++)
          {
            double v = 0.0;
            pos = pPreset->mChunk.Get(&v, pos);

            WDL_EndianFloat v32;
            v32.f = (float) mParams.Get(i)->GetNormalized(v);
            unsigned int swapped = WDL_bswap32(v32.int32);
            bnk.Put(&swapped);
          }
        }
      }

      fwrite(bnk.GetBytes(), bnk.Size(), 1, fp);
      fclose(fp);

      return true;
    }
  }
  return false;
}

bool IPlugBase::LoadProgramFromFXP()
{
  if (mGraphics)
  {
    WDL_String fileName;
    mGraphics->PromptForFile(&fileName, kFileOpen, &mPreviousPath, "fxp");

    if (fileName.GetLength())
    {
      FILE* fp = fopen(fileName.Get(), "rb");

      if (fp)
      {
        ByteChunk pgm;
        long fileSize;

        fseek(fp , 0 , SEEK_END);
        fileSize = ftell(fp);
        rewind(fp);

        pgm.Resize(fileSize);
        fread(pgm.GetBytes(), fileSize, 1, fp);

        fclose(fp);

        int pos = 0;

        VstInt32 chunkMagic;
        VstInt32 byteSize = 0;
        VstInt32 fxpMagic;
        VstInt32 fxpVersion;
        VstInt32 pluginID;
        VstInt32 pluginVersion;
        VstInt32 numParams;
        char prgName[28];

        pos = pgm.Get(&chunkMagic, pos);
        chunkMagic = WDL_bswap_if_le(chunkMagic);
        pos = pgm.Get(&byteSize, pos);
        byteSize = WDL_bswap_if_le(byteSize);
        pos = pgm.Get(&fxpMagic, pos);
        fxpMagic = WDL_bswap_if_le(fxpMagic);
        pos = pgm.Get(&fxpVersion, pos);
        fxpVersion = WDL_bswap_if_le(fxpVersion);
        pos = pgm.Get(&pluginID, pos);
        pluginID = WDL_bswap_if_le(pluginID);
        pos = pgm.Get(&pluginVersion, pos);
        pluginVersion = WDL_bswap_if_le(pluginVersion);
        pos = pgm.Get(&numParams, pos);
        numParams = WDL_bswap_if_le(numParams);
        pos = pgm.GetBytes(prgName, 28, pos);

        if (chunkMagic != 'CcnK') return false;
        if (fxpVersion != kFXPVersionNum) return false; // TODO: what if a host saves as a different version?
        if (pluginID != GetUniqueID()) return false;
        //if (pluginVersion != GetEffectVersion(true)) return false; // TODO: provide mechanism for loading earlier versions
        if (numParams != NParams()) return false; // TODO: provide mechanism for loading earlier versions with less params

        if (DoesStateChunks() && fxpMagic == 'FPCh')
        {
          VstInt32 chunkSize;
          pos = pgm.Get(&chunkSize, pos);
          chunkSize = WDL_bswap_if_le(chunkSize);

          GetIPlugVerFromChunk(&pgm, &pos);
          UnserializeState(&pgm, pos);
          ModifyCurrentPreset(prgName);
          RestorePreset(GetCurrentPresetIdx());
          InformHostOfProgramChange();

          return true;
        }
        else if (fxpMagic == 'FxCk')
        {
          for (int i = 0; i< NParams(); i++)
          {
            WDL_EndianFloat v32;
            pos = pgm.Get(&v32.int32, pos);
            v32.int32 = WDL_bswap_if_le(v32.int32);
            mParams.Get(i)->SetNormalized((double) v32.f);
          }

          ModifyCurrentPreset(prgName);
          RestorePreset(GetCurrentPresetIdx());
          InformHostOfProgramChange();

          return true;
        }
      }
    }
  }
  return false;
}

bool IPlugBase::LoadBankFromFXB()
{
  if (mGraphics)
  {
    WDL_String fileName;
    mGraphics->PromptForFile(&fileName, kFileOpen, &mPreviousPath, "fxb");

    if (fileName.GetLength())
    {
      FILE* fp = fopen(fileName.Get(), "rb");

      if (fp)
      {
        ByteChunk bnk;
        long fileSize;

        fseek(fp , 0 , SEEK_END);
        fileSize = ftell(fp);
        rewind(fp);

        bnk.Resize(fileSize);
        fread(bnk.GetBytes(), fileSize, 1, fp);

        fclose(fp);

        int pos = 0;

        VstInt32 chunkMagic;
        VstInt32 byteSize = 0;
        VstInt32 fxbMagic;
        VstInt32 fxbVersion;
        VstInt32 pluginID;
        VstInt32 pluginVersion;
        VstInt32 numPgms;
        VstInt32 currentPgm;
        char future[124];
        memset(future, 0, 124);

        pos = bnk.Get(&chunkMagic, pos);
        chunkMagic = WDL_bswap_if_le(chunkMagic);
        pos = bnk.Get(&byteSize, pos);
        byteSize = WDL_bswap_if_le(byteSize);
        pos = bnk.Get(&fxbMagic, pos);
        fxbMagic = WDL_bswap_if_le(fxbMagic);
        pos = bnk.Get(&fxbVersion, pos);
        fxbVersion = WDL_bswap_if_le(fxbVersion);
        pos = bnk.Get(&pluginID, pos);
        pluginID = WDL_bswap_if_le(pluginID);
        pos = bnk.Get(&pluginVersion, pos);
        pluginVersion = WDL_bswap_if_le(pluginVersion);
        pos = bnk.Get(&numPgms, pos);
        numPgms = WDL_bswap_if_le(numPgms);
        pos = bnk.Get(&currentPgm, pos);
        currentPgm = WDL_bswap_if_le(currentPgm);
        pos = bnk.GetBytes(future, 124, pos);

        if (chunkMagic != 'CcnK') return false;
        //if (fxbVersion != kFXBVersionNum) return false; // TODO: what if a host saves as a different version?
        if (pluginID != GetUniqueID()) return false;
        //if (pluginVersion != GetEffectVersion(true)) return false; // TODO: provide mechanism for loading earlier versions
        //if (numPgms != NPresets()) return false; // TODO: provide mechanism for loading earlier versions with less params

        if (DoesStateChunks() && fxbMagic == 'FBCh')
        {
          VstInt32 chunkSize;
          pos = bnk.Get(&chunkSize, pos);
          chunkSize = WDL_bswap_if_le(chunkSize);

          GetIPlugVerFromChunk(&bnk, &pos);
          UnserializePresets(&bnk, pos);
          //RestorePreset(currentPgm);
          InformHostOfProgramChange();
          return true;
        }
        else if (fxbMagic == 'FxBk')
        {
          VstInt32 chunkMagic;
          VstInt32 byteSize;
          VstInt32 fxpMagic;
          VstInt32 fxpVersion;
          VstInt32 pluginID;
          VstInt32 pluginVersion;
          VstInt32 numParams;
          char prgName[28];

          for(int i = 0; i<numPgms; i++)
          {
            pos = bnk.Get(&chunkMagic, pos);
            chunkMagic = WDL_bswap_if_le(chunkMagic);

            pos = bnk.Get(&byteSize, pos);
            byteSize = WDL_bswap_if_le(byteSize);

            pos = bnk.Get(&fxpMagic, pos);
            fxpMagic = WDL_bswap_if_le(fxpMagic);

            pos = bnk.Get(&fxpVersion, pos);
            fxpVersion = WDL_bswap_if_le(fxpVersion);

            pos = bnk.Get(&pluginID, pos);
            pluginID = WDL_bswap_if_le(pluginID);

            pos = bnk.Get(&pluginVersion, pos);
            pluginVersion = WDL_bswap_if_le(pluginVersion);

            pos = bnk.Get(&numParams, pos);
            numParams = WDL_bswap_if_le(numParams);

            if (chunkMagic != 'CcnK') return false;
            if (fxpMagic != 'FxCk') return false;
            if (fxpVersion != kFXPVersionNum) return false;
            if (numParams != NParams()) return false;

            pos = bnk.GetBytes(prgName, 28, pos);

            RestorePreset(i);

            for (int j = 0; j< NParams(); j++)
            {
              WDL_EndianFloat v32;
              pos = bnk.Get(&v32.int32, pos);
              v32.int32 = WDL_bswap_if_le(v32.int32);
              mParams.Get(j)->SetNormalized((double) v32.f);
            }

            ModifyCurrentPreset(prgName);
          }

          RestorePreset(currentPgm);
          InformHostOfProgramChange();

          return true;
        }
      }
    }
  }
  return false;
}

#endif

void IPlugBase::InitChunkWithIPlugVer(ByteChunk* pChunk)
{
  pChunk->Clear();
  int magic = IPLUG_VERSION_MAGIC;
  pChunk->Put(&magic);
  int ver = IPLUG_VERSION;
  pChunk->Put(&ver);
}

int IPlugBase::GetIPlugVerFromChunk(ByteChunk* pChunk, int* pPos)
{
  int magic = 0, ver = 0;
  int pos = pChunk->Get(&magic, *pPos);
  if (pos > *pPos && magic == IPLUG_VERSION_MAGIC)
  {
    *pPos = pChunk->Get(&ver, pos);
  }
  return ver;
}

bool IPlugBase::SendMidiMsgs(WDL_TypedBuf<IMidiMsg>* pMsgs)
{
  bool rc = true;
  int n = pMsgs->GetSize();
  IMidiMsg* pMsg = pMsgs->Get();
  for (int i = 0; i < n; ++i, ++pMsg) {
    rc &= SendMidiMsg(pMsg);
  }
  return rc;
}