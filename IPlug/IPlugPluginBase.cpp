/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

/**
 * @file
 * @brief IPluginBase implementation
 */

#include "IPlugPluginBase.h"
#include "wdlendian.h"
#include "wdl_base64.h"

using namespace iplug;

IPluginBase::IPluginBase(int nParams, int nPresets)
: EDITOR_DELEGATE_CLASS(nParams)
{  
#ifndef NO_PRESETS
  for (int i = 0; i < nPresets; ++i)
    mPresets.Add(new IPreset());
#endif
}

IPluginBase::~IPluginBase()
{
#ifndef NO_PRESETS
  mPresets.Empty(true);
#endif
}

int IPluginBase::GetPluginVersion(bool decimal) const
{
  if (decimal)
    return GetDecimalVersion(mVersion);
  else
    return mVersion;
}

void IPluginBase::GetPluginVersionStr(WDL_String& str) const
{
  GetVersionStr(mVersion, str);
#if defined TRACER_BUILD
  str.Append("T");
#endif
#if defined _DEBUG
  str.Append("D");
#endif
}

int IPluginBase::GetHostVersion(bool decimal) const
{
  if (decimal)
  {
    return GetDecimalVersion(mHostVersion);
  }
  return mHostVersion;
}

void IPluginBase::GetHostVersionStr(WDL_String& str) const
{
  GetVersionStr(mHostVersion, str);
}

const char* IPluginBase::GetAPIStr() const
{
  switch (GetAPI())
  {
    case kAPIVST2: return "VST2";
    case kAPIVST3: return "VST3";
    case kAPIAU: return "AU";
    case kAPIAUv3: return "AUv3";
    case kAPIAAX: return "AAX";
    case kAPIAPP: return "APP";
    case kAPIWAM: return "WAM";
    case kAPIWEB: return "WEB";
    default: return "";
  }
}

const char* IPluginBase::GetArchStr() const
{
#if defined OS_WEB
  return "WASM";
#elif defined ARCH_64BIT
  return "x64";
#else
  return "x86";
#endif
}

void IPluginBase::GetBuildInfoStr(WDL_String& str) const
{
  WDL_String version;
  GetPluginVersionStr(version);
  str.SetFormatted(MAX_BUILD_INFO_STR_LEN, "%s version %s %s (%s), built on %s at %.5s ", GetPluginName(), version.Get(), GetAPIStr(), GetArchStr(), __DATE__, __TIME__);
}

#pragma mark -

bool IPluginBase::SerializeParams(IByteChunk& chunk) const
{
  TRACE
  bool savedOK = true;
  int i, n = mParams.GetSize();
  for (i = 0; i < n && savedOK; ++i)
  {
    IParam* pParam = mParams.Get(i);
    Trace(TRACELOC, "%d %s %f", i, pParam->GetNameForHost(), pParam->Value());
    double v = pParam->Value();
    savedOK &= (chunk.Put(&v) > 0);
  }
  return savedOK;
}

int IPluginBase::UnserializeParams(const IByteChunk& chunk, int startPos)
{
  TRACE
  int i, n = mParams.GetSize(), pos = startPos;
  ENTER_PARAMS_MUTEX
  for (i = 0; i < n && pos >= 0; ++i)
  {
    IParam* pParam = mParams.Get(i);
    double v = 0.0;
    pos = chunk.Get(&v, pos);
    pParam->Set(v);
    Trace(TRACELOC, "%d %s %f", i, pParam->GetNameForHost(), pParam->Value());
  }

  OnParamReset(kPresetRecall);
  LEAVE_PARAMS_MUTEX

  return pos;
}

void IPluginBase::InitParamRange(int startIdx, int endIdx, int countStart, const char* nameFmtStr, double defaultVal, double minVal, double maxVal, double step, const char *label, int flags, const char *group, const IParam::Shape& shape, IParam::EParamUnit unit, IParam::DisplayFunc displayFunc)
{
  WDL_String nameStr;
  for (auto p = startIdx; p <= endIdx; p++)
  {
    nameStr.SetFormatted(MAX_PARAM_NAME_LEN, nameFmtStr, countStart + (p-startIdx));
    GetParam(p)->InitDouble(nameStr.Get(), defaultVal, minVal, maxVal, step, label, flags, group, shape, unit, displayFunc);
  }
}

void IPluginBase::CloneParamRange(int cloneStartIdx, int cloneEndIdx, int startIdx, const char* searchStr, const char* replaceStr, const char* newGroup)
{
  for (auto p = cloneStartIdx; p <= cloneEndIdx; p++)
  {
    IParam* pParam = GetParam(p);
    int outIdx = startIdx + (p - cloneStartIdx);
    GetParam(outIdx)->Init(*pParam, searchStr, replaceStr, newGroup);
    GetParam(outIdx)->Set(pParam->Value());
  }
}

void IPluginBase::CopyParamValues(int startIdx, int destIdx, int nParams)
{
  assert((startIdx + nParams) < NParams());
  assert((destIdx + nParams) < NParams());
  assert((startIdx + nParams) < destIdx);
  
  for (auto p = startIdx; p < startIdx + nParams; p++)
  {
    GetParam(destIdx++)->Set(GetParam(p)->Value());
  }
}

void IPluginBase::CopyParamValues(const char* inGroup, const char *outGroup)
{
  WDL_PtrList<IParam> inParams, outParams;
  
  for (auto p = 0; p < NParams(); p++)
  {
    IParam* pParam = GetParam(p);
    if(strcmp(pParam->GetGroupForHost(), inGroup) == 0)
    {
      inParams.Add(pParam);
    }
    else if(strcmp(pParam->GetGroupForHost(), outGroup) == 0)
    {
      outParams.Add(pParam);
    }
  }
  
  assert(inParams.GetSize() == outParams.GetSize());
  
  for (auto p = 0; p < inParams.GetSize(); p++)
  {
    outParams.Get(p)->Set(inParams.Get(p)->Value());
  }
}

void IPluginBase::ForParamInRange(int startIdx, int endIdx, std::function<void(int paramIdx, IParam&)>func)
{
  for (auto p = startIdx; p <= endIdx; p++)
  {
    func(p, * GetParam(p));
  }
}

void IPluginBase::ForParamInGroup(const char* paramGroup, std::function<void (int paramIdx, IParam&)> func)
{
  for (auto p = 0; p < NParams(); p++)
  {
    IParam* pParam = GetParam(p);
    if(strcmp(pParam->GetGroupForHost(), paramGroup) == 0)
    {
      func(p, *pParam);
    }
  }
}

void IPluginBase::DefaultParamValues()
{
  DefaultParamValues(0, NParams()-1);
}

void IPluginBase::DefaultParamValues(int startIdx, int endIdx)
{
  ForParamInRange(startIdx, endIdx, [](int paramIdx, IParam& param) {
                      param.SetToDefault();
                    });
}

void IPluginBase::DefaultParamValues(const char* paramGroup)
{
  ForParamInGroup(paramGroup, [](int paramIdx, IParam& param) {
                      param.SetToDefault();
                    });
}

void IPluginBase::RandomiseParamValues()
{
  RandomiseParamValues(0, NParams()-1);
}

void IPluginBase::RandomiseParamValues(int startIdx, int endIdx)
{
  ForParamInRange(startIdx, endIdx, [&](int paramIdx, IParam& param) { param.SetNormalized( static_cast<float>(std::rand()/(static_cast<float>(RAND_MAX)+1.f)) ); });
}

void IPluginBase::RandomiseParamValues(const char *paramGroup)
{
  ForParamInGroup(paramGroup, [&](int paramIdx, IParam& param) { param.SetNormalized( static_cast<float>(std::rand()/(static_cast<float>(RAND_MAX)+1.f)) ); });
}

void IPluginBase::PrintParamValues()
{
  ForParamInRange(0, NParams()-1, [](int paramIdx, IParam& param) {
    param.PrintDetails();
    DBGMSG("\n");
  });
}

#ifndef NO_PRESETS
static IPreset* GetNextUninitializedPreset(WDL_PtrList<IPreset>* pPresets)
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

void IPluginBase::MakeDefaultPreset(const char* name, int nPresets)
{
  for (int i = 0; i < nPresets; ++i)
  {
    IPreset* pPreset = GetNextUninitializedPreset(&mPresets);
    if (pPreset)
    {
      pPreset->mInitialized = true;
      strcpy(pPreset->mName, (name ? name : "Empty"));
      SerializeState(pPreset->mChunk);
    }
  }
}

void IPluginBase::MakePreset(const char* name, ...)
{
  IPreset* pPreset = GetNextUninitializedPreset(&mPresets);
  if (pPreset)
  {
    pPreset->mInitialized = true;
    strcpy(pPreset->mName, name);
    
    int i, n = NParams();
    
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

void IPluginBase::MakePresetFromNamedParams(const char* name, int nParamsNamed, ...)
{
  TRACE
  IPreset* pPreset = GetNextUninitializedPreset(&mPresets);
  if (pPreset)
  {
    pPreset->mInitialized = true;
    strcpy(pPreset->mName, name);
    
    int i = 0, n = NParams();
    
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
      assert(paramIdx > kNoParameter && paramIdx < n);
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

void IPluginBase::MakePresetFromChunk(const char* name, IByteChunk& chunk)
{
  IPreset* pPreset = GetNextUninitializedPreset(&mPresets);
  if (pPreset)
  {
    pPreset->mInitialized = true;
    strcpy(pPreset->mName, name);
    
    pPreset->mChunk.PutChunk(&chunk);
  }
}

void IPluginBase::MakePresetFromBlob(const char* name, const char* blob, int sizeOfChunk)
{
  IByteChunk presetChunk;
  presetChunk.Resize(sizeOfChunk);
  wdl_base64decode(blob, presetChunk.GetData(), sizeOfChunk);
  
  MakePresetFromChunk(name, presetChunk);
}

static void MakeDefaultUserPresetName(WDL_PtrList<IPreset>* pPresets, char* str)
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

void IPluginBase::EnsureDefaultPreset()
{
  TRACE
  MakeDefaultPreset("Empty", mPresets.GetSize());
}

void IPluginBase::PruneUninitializedPresets()
{
  TRACE
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

bool IPluginBase::RestorePreset(int idx)
{
  TRACE
  bool restoredOK = false;
  if (idx >= 0 && idx < mPresets.GetSize())
  {
    IPreset* pPreset = mPresets.Get(idx);
    
    if (!(pPreset->mInitialized))
    {
      pPreset->mInitialized = true;
      MakeDefaultUserPresetName(&mPresets, pPreset->mName);
      restoredOK = SerializeState(pPreset->mChunk);
    }
    else
    {
      restoredOK = (UnserializeState(pPreset->mChunk, 0) > 0);
    }
    
    if (restoredOK)
    {
      mCurrentPresetIdx = idx;
      OnPresetsModified();
      OnRestoreState();
    }
  }
  return restoredOK;
}

bool IPluginBase::RestorePreset(const char* name)
{
  if (CStringHasContents(name))
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

const char* IPluginBase::GetPresetName(int idx) const
{
  if (idx >= 0 && idx < mPresets.GetSize())
  {
    return mPresets.Get(idx)->mName;
  }
  return "";
}

void IPluginBase::ModifyCurrentPreset(const char* name)
{
  if (mCurrentPresetIdx >= 0 && mCurrentPresetIdx < mPresets.GetSize())
  {
    IPreset* pPreset = mPresets.Get(mCurrentPresetIdx);
    pPreset->mChunk.Clear();
    
    Trace(TRACELOC, "%d %s", mCurrentPresetIdx, pPreset->mName);
    
    SerializeState(pPreset->mChunk);
    
    if (CStringHasContents(name))
    {
      strcpy(pPreset->mName, name);
    }
  }
}

bool IPluginBase::SerializePresets(IByteChunk& chunk) const
{
  TRACE
  bool savedOK = true;
  int n = mPresets.GetSize();
  for (int i = 0; i < n && savedOK; ++i)
  {
    IPreset* pPreset = mPresets.Get(i);
    chunk.PutStr(pPreset->mName);
    
    Trace(TRACELOC, "%d %s", i, pPreset->mName);
    
    chunk.Put(&pPreset->mInitialized);
    if (pPreset->mInitialized)
    {
      savedOK &= (chunk.PutChunk(&(pPreset->mChunk)) > 0);
    }
  }
  return savedOK;
}

int IPluginBase::UnserializePresets(IByteChunk& chunk, int startPos)
{
  TRACE
  WDL_String name;
  int n = mPresets.GetSize(), pos = startPos;
  for (int i = 0; i < n && pos >= 0; ++i)
  {
    IPreset* pPreset = mPresets.Get(i);
    pos = chunk.GetStr(name, pos);
    strcpy(pPreset->mName, name.Get());
    
    Trace(TRACELOC, "%d %s", i, pPreset->mName);
    
    pos = chunk.Get<bool>(&(pPreset->mInitialized), pos);
    if (pPreset->mInitialized)
    {
      pos = UnserializeState(chunk, pos);
      if (pos > 0)
      {
        pPreset->mChunk.Clear();
        SerializeState(pPreset->mChunk);
      }
    }
  }
  RestorePreset(mCurrentPresetIdx);
  return pos;
}

void IPluginBase::DumpPresetSrcCode(const char* filename, const char* paramEnumNames[]) const
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
      const IParam* pParam = GetParam(i);
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

void IPluginBase::DumpAllPresetsBlob(const char* filename) const
{
  FILE* fp = fopen(filename, "w");
  
  char buf[MAX_BLOB_LENGTH] = "";
  IByteChunk chnk;
  
  for (int i = 0; i< NPresets(); i++)
  {
    IPreset* pPreset = mPresets.Get(i);
    fprintf(fp, "MakePresetFromBlob(\"%s\", \"", pPreset->mName);
    
    chnk.Clear();
    chnk.PutChunk(&(pPreset->mChunk));
    wdl_base64encode(chnk.GetData(), buf, chnk.Size());
    
    fprintf(fp, "%s\", %i, %i);\n", buf, chnk.Size(), pPreset->mChunk.Size());
  }
  
  fclose(fp);
}

void IPluginBase::DumpPresetBlob(const char* filename) const
{
  FILE* fp = fopen(filename, "w");
  fprintf(fp, "MakePresetFromBlob(\"name\", \"");
  
  char buf[MAX_BLOB_LENGTH];
  
  IByteChunk* pPresetChunk = &mPresets.Get(mCurrentPresetIdx)->mChunk;
  uint8_t* byteStart = pPresetChunk->GetData();
  
  wdl_base64encode(byteStart, buf, pPresetChunk->Size());
  
  fprintf(fp, "%s\", %i);\n", buf, pPresetChunk->Size());
  fclose(fp);
}

void IPluginBase::DumpBankBlob(const char* filename) const
{
  FILE* fp = fopen(filename, "w");
  
  if (!fp)
    return;
  
  char buf[MAX_BLOB_LENGTH] = "";
  
  for (int i = 0; i< NPresets(); i++)
  {
    IPreset* pPreset = mPresets.Get(i);
    fprintf(fp, "MakePresetFromBlob(\"%s\", \"", pPreset->mName);
    
    IByteChunk* pPresetChunk = &pPreset->mChunk;
    wdl_base64encode(pPresetChunk->GetData(), buf, pPresetChunk->Size());
    
    fprintf(fp, "%s\", %i);\n", buf, pPresetChunk->Size());
  }
  
  fclose(fp);
}

// confusing... IByteChunk will force storage as little endian on big endian platforms,
// so when we use it here, since vst fxp/fxb files are big endian, we need to swap the endianess
// regardless of the endianness of the host, and on big endian hosts it will get swapped back to
// big endian
bool IPluginBase::SaveProgramAsFXP(const char* file) const
{
  if (CStringHasContents(file))
  {
    FILE* fp = fopen(file, "wb");
    
    IByteChunk pgm;
    
    int32_t chunkMagic = WDL_bswap32('CcnK');
    int32_t byteSize = 0;
    int32_t fxpMagic;
    int32_t fxpVersion = WDL_bswap32(kFXPVersionNum);
    int32_t pluginID = WDL_bswap32(GetUniqueID());
    int32_t pluginVersion = WDL_bswap32(GetPluginVersion(true));
    int32_t numParams = WDL_bswap32(NParams());
    char prgName[28];
    memset(prgName, 0, 28);
    strcpy(prgName, GetPresetName(GetCurrentPresetIdx()));
    
    pgm.Put(&chunkMagic);
    
    if (DoesStateChunks())
    {
      IByteChunk state;
      int32_t chunkSize;
      
      fxpMagic = WDL_bswap32('FPCh');
      
      IByteChunk::InitChunkWithIPlugVer(state);
      SerializeState(state);
      
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
      pgm.PutBytes(state.GetData(), state.Size());
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
        v32.f = (float) GetParam(i)->GetNormalized();
        unsigned int swapped = WDL_bswap32(v32.int32);
        pgm.Put(&swapped);
      }
    }
    
    fwrite(pgm.GetData(), pgm.Size(), 1, fp);
    fclose(fp);
    
    return true;
  }
  return false;
}

bool IPluginBase::SaveBankAsFXB(const char* file) const
{
  if (CStringHasContents(file))
  {
    FILE* fp = fopen(file, "wb");
    
    IByteChunk bnk;
    
    int32_t chunkMagic = WDL_bswap32('CcnK');
    int32_t byteSize = 0;
    int32_t fxbMagic;
    int32_t fxbVersion = WDL_bswap32(kFXBVersionNum);
    int32_t pluginID = WDL_bswap32(GetUniqueID());
    int32_t pluginVersion = WDL_bswap32(GetPluginVersion(true));
    int32_t numPgms =  WDL_bswap32(NPresets());
    int32_t currentPgm = WDL_bswap32(GetCurrentPresetIdx());
    char future[124];
    memset(future, 0, 124);
    
    bnk.Put(&chunkMagic);
    
    if (DoesStateChunks())
    {
      IByteChunk state;
      int32_t chunkSize;
      
      fxbMagic = WDL_bswap32('FBCh');
      
      IByteChunk::InitChunkWithIPlugVer(state);
      SerializePresets(state);
      
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
      bnk.PutBytes(state.GetData(), state.Size());
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
      
      int32_t fxpMagic = WDL_bswap32('FxCk');
      int32_t fxpVersion = WDL_bswap32(kFXPVersionNum);
      int32_t numParams = WDL_bswap32(NParams());
      
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
          v32.f = (float) GetParam(i)->ToNormalized(v);
          uint32_t swapped = WDL_bswap32(v32.int32);
          bnk.Put(&swapped);
        }
      }
    }
    
    fwrite(bnk.GetData(), bnk.Size(), 1, fp);
    fclose(fp);
    
    return true;
  }
  else
    return false;
}

bool IPluginBase::LoadProgramFromFXP(const char* file)
{
  if (CStringHasContents(file))
  {
    FILE* fp = fopen(file, "rb");
    
    if (fp)
    {
      IByteChunk pgm;
      long fileSize;
      
      fseek(fp , 0 , SEEK_END);
      fileSize = ftell(fp);
      rewind(fp);
      
      pgm.Resize((int) fileSize);
      fread(pgm.GetData(), fileSize, 1, fp);
      
      fclose(fp);
      
      int pos = 0;
      
      int32_t chunkMagic;
      int32_t byteSize = 0;
      int32_t fxpMagic;
      int32_t fxpVersion;
      int32_t pluginID;
      int32_t pluginVersion;
      int32_t numParams;
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
      //if (pluginVersion != GetPluginVersion(true)) return false; // TODO: provide mechanism for loading earlier versions
      //if (numParams != NParams()) return false; // TODO: provide mechanism for loading earlier versions with less params
      
      if (DoesStateChunks() && fxpMagic == 'FPCh')
      {
        int32_t chunkSize;
        pos = pgm.Get(&chunkSize, pos);
        chunkSize = WDL_bswap_if_le(chunkSize);
        
        IByteChunk::GetIPlugVerFromChunk(pgm, pos);
        UnserializeState(pgm, pos);
        ModifyCurrentPreset(prgName);
        RestorePreset(GetCurrentPresetIdx());
        InformHostOfProgramChange();
        
        return true;
      }
      else if (fxpMagic == 'FxCk') // Due to the big Endian-ness of FXP/FXB format we cannot call SerializeParams()
      {
        ENTER_PARAMS_MUTEX
        for (int i = 0; i< NParams(); i++)
        {
          WDL_EndianFloat v32;
          pos = pgm.Get(&v32.int32, pos);
          v32.int32 = WDL_bswap_if_le(v32.int32);
          GetParam(i)->SetNormalized((double) v32.f);
        }
        LEAVE_PARAMS_MUTEX
        
        ModifyCurrentPreset(prgName);
        RestorePreset(GetCurrentPresetIdx());
        InformHostOfProgramChange();
        
        return true;
      }
    }
  }
  
  return false;
}

bool IPluginBase::LoadBankFromFXB(const char* file)
{
  if (CStringHasContents(file))
  {
    FILE* fp = fopen(file, "rb");
    
    if (fp)
    {
      IByteChunk bnk;
      long fileSize;
      
      fseek(fp , 0 , SEEK_END);
      fileSize = ftell(fp);
      rewind(fp);
      
      bnk.Resize((int) fileSize);
      fread(bnk.GetData(), fileSize, 1, fp);
      
      fclose(fp);
      
      int pos = 0;
      
      int32_t chunkMagic;
      int32_t byteSize = 0;
      int32_t fxbMagic;
      int32_t fxbVersion;
      int32_t pluginID;
      int32_t pluginVersion;
      int32_t numPgms;
      int32_t currentPgm;
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
      //if (pluginVersion != GetPluginVersion(true)) return false; // TODO: provide mechanism for loading earlier versions
      //if (numPgms != NPresets()) return false; // TODO: provide mechanism for loading earlier versions with less params
      
      if (DoesStateChunks() && fxbMagic == 'FBCh')
      {
        int32_t chunkSize;
        pos = bnk.Get(&chunkSize, pos);
        chunkSize = WDL_bswap_if_le(chunkSize);
        
        IByteChunk::GetIPlugVerFromChunk(bnk, pos);
        UnserializePresets(bnk, pos);
        //RestorePreset(currentPgm);
        InformHostOfProgramChange();
        return true;
      }
      else if (fxbMagic == 'FxBk') // Due to the big Endian-ness of FXP/FXB format we cannot call SerializeParams()
      {
        int32_t chunkMagic;
        int32_t byteSize;
        int32_t fxpMagic;
        int32_t fxpVersion;
        int32_t pluginID;
        int32_t pluginVersion;
        int32_t numParams;
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
          
          ENTER_PARAMS_MUTEX
          for (int j = 0; j< NParams(); j++)
          {
            WDL_EndianFloat v32;
            pos = bnk.Get(&v32.int32, pos);
            v32.int32 = WDL_bswap_if_le(v32.int32);
            GetParam(j)->SetNormalized((double) v32.f);
          }
          LEAVE_PARAMS_MUTEX
          
          ModifyCurrentPreset(prgName);
        }
        
        RestorePreset(currentPgm);
        InformHostOfProgramChange();
        
        return true;
      }
    }
  }
  
  return false;
}

bool IPluginBase::LoadProgramFromVSTPreset(const char* path)
{
  auto isEqualID = [](const ChunkID id1, const ChunkID id2) {
    return memcmp (id1, id2, sizeof (ChunkID)) == 0;
  };
  
  FILE* fp = fopen(path, "rb");
  
  if (fp)
  {
    IByteChunk pgm;
    long fileSize;
    
    fseek(fp , 0 , SEEK_END);
    fileSize = ftell(fp);
    rewind(fp);
    
    pgm.Resize((int) fileSize);
    fread(pgm.GetData(), fileSize, 1, fp);
    
    fclose(fp);
    
    int pos = 0;
    
    char classString[kClassIDSize + 1] = {0};
//    FUID cID;
    
    //HEADER
    ChunkID chunkID;
    int32_t version = 0;
    int64_t listOffset = 0;
    
    pos = pgm.Get(&chunkID, pos);
    
    if (!isEqualID(chunkID, commonChunks[kHeader]))
      return false;
    
    pos = pgm.Get(&version, pos);
    pos = pgm.GetBytes(&classString, kClassIDSize, pos);
//    cID.fromString(classString);
    
//    if (cID != mProcFUID)
//      return false;
    
    pos = pgm.Get(&listOffset, pos);
    
    //CHUNK LIST
    pos = pgm.Get(&chunkID, (int) listOffset); // jump forward to chunklist start
    
    if (!isEqualID(chunkID, commonChunks[kChunkList]))
      return false;
    
    int32_t entryCount = 0;
    pos = pgm.Get(&entryCount, pos);
    
    if (entryCount < 2)
      return false;
    
    pos = pgm.Get(&chunkID, pos);
    
    if (!isEqualID(chunkID, commonChunks[kComponentState]))
      return false;
    
    int64_t offsetToComponentState = 0;
    pos = pgm.Get(&offsetToComponentState, pos);
    
    int64_t componentStateSize = 0;
    pos = pgm.Get(&componentStateSize, pos);
    
    pos = pgm.Get(&chunkID, pos);
    
    if (!isEqualID(chunkID, commonChunks[kControllerState]))
      return false;
    
    int64_t offsetToCtrlrState = 0;
    pos = pgm.Get(&offsetToCtrlrState, pos);
    
    int64_t ctrlrStateSize = 0;
    pos = pgm.Get(&ctrlrStateSize, pos);
    
    //Forget about kMetaInfo chunk
  
    pos = (int) offsetToComponentState;
    //DATA AREA
    int componentChunkSizeIPlug;
    pos = pgm.Get(&componentChunkSizeIPlug, pos);
    IByteChunk::GetIPlugVerFromChunk(pgm, pos /* updates pos */ );
    
    pos += sizeof(int32_t); // FIXME: bypass
    
//    GetBypassStateFromChunk(&pgm, pos /* updates pos */);
    pos = UnserializeState(pgm, pos);
    
    int ctrlrChunkSizeIPlug;
    pos = pgm.Get(&ctrlrChunkSizeIPlug, pos);
    IByteChunk::GetIPlugVerFromChunk(pgm, pos /* updates pos */);
    pos = UnserializeVST3CtrlrState(pgm, pos);
    
    DirtyParametersFromUI();
    OnRestoreState();
    
    return true;
  }
  
  return false;
}

void IPluginBase::MakeVSTPresetChunk(IByteChunk& chunk, IByteChunk& componentState, IByteChunk& controllerState) const
{
  WDL_String metaInfo("");
  
  metaInfo.Append("<?xml version=\"1.0\" encoding=\"utf-8\"?>\n", 1024);
  metaInfo.Append("<MetaInfo>\n", 1024);
  metaInfo.Append("\t<Attribute id=\"MediaType\" value=\"VstPreset\" type=\"string\" flags=\"writeProtected\"/>\n", 1024);
  metaInfo.AppendFormatted(1024, "\t<Attribute id=\"plugname\" value=\"%s\" type=\"string\" flags=\"\"/>\n", mProductName.Get());
  metaInfo.AppendFormatted(1024, "\t<Attribute id=\"plugtype\" value=\"%s\" type=\"string\" flags=\"\"/>\n", mVST3ProductCategory.Get());
  metaInfo.Append("</MetaInfo>\n", 1024);
  
  //HEADER
  chunk.Put(&commonChunks[kHeader]);
  int32_t version = kFormatVersion;
  chunk.Put(&version);
  chunk.PutBytes(mVST3ProcessorUIDStr.Get(), kClassIDSize);
  int64_t offsetToChunkList = componentState.Size() + controllerState.Size() + (2 * sizeof(int) /* 2 ints for sizes */) + strlen(metaInfo.Get()) + kHeaderSize;
  chunk.Put(&offsetToChunkList);
  
  //DATA AREA
  int componentSize = componentState.Size();
  chunk.Put(&componentSize);
  chunk.PutChunk(&componentState);
  int ctrlrSize = controllerState.Size();
  chunk.Put(&ctrlrSize);
  chunk.PutChunk(&controllerState);
  chunk.PutBytes(metaInfo.Get(), metaInfo.GetLength());
  
  //CHUNK LIST
  chunk.Put(&commonChunks[kChunkList]);
  int32_t entryCount = 3;
  chunk.Put(&entryCount);
  
  chunk.Put(&commonChunks[kComponentState]);
  int64_t offsetToComponentState = kHeaderSize;
  chunk.Put(&offsetToComponentState);
  int64_t componentStateSize = componentState.Size() + sizeof(int);
  chunk.Put(&componentStateSize);
  
  chunk.Put(&commonChunks[kControllerState]);
  int64_t offsetToCtrlrState = kHeaderSize + componentStateSize;
  chunk.Put(&offsetToCtrlrState);
  int64_t ctrlrStateSize = controllerState.Size() + sizeof(int);
  chunk.Put(&ctrlrStateSize);
  
  chunk.Put(&commonChunks[kMetaInfo]);
  int64_t offsetToMetaInfo = kHeaderSize + componentStateSize + ctrlrStateSize;
  chunk.Put(&offsetToMetaInfo);
  int64_t metaInfoSize = metaInfo.GetLength();
  chunk.Put(&metaInfoSize);
}

bool IPluginBase::SaveProgramAsVSTPreset(const char* path) const
{
  if (path)
  {
    FILE* fp = fopen(path, "wb");
    
    if (fp)
    {
      IByteChunk componentState;
      IByteChunk::InitChunkWithIPlugVer(componentState);
      //      AppendBypassStateToChunk(&componentState, false); //TODO:!!
      SerializeState(componentState);
      
      IByteChunk ctrlrState;
      IByteChunk::InitChunkWithIPlugVer(ctrlrState);
      SerializeVST3CtrlrState(ctrlrState);
      
      IByteChunk pgm;
      
      MakeVSTPresetChunk(pgm, componentState, ctrlrState);
      
      fwrite(pgm.GetData(), pgm.Size(), 1, fp);
      fclose(fp);
      
      return true;
    }
  }
  
  return false;
}

#endif
