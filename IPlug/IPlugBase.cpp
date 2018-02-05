#include <cmath>
#include <cstdio>
#include <ctime>
#include <cassert>

#include "wdlendian.h"
#include "wdl_base64.h"

#include "IPlugBase.h"

IPlugBase::IPlugBase(IPlugConfig c, EAPI plugAPI)
  : mUniqueID(c.uniqueID)
  , mMfrID(c.mfrID)
  , mVersion(c.vendorVersion)
  , mStateChunks(c.plugDoesChunks)
  , mEffectName(c.effectName, MAX_EFFECT_NAME_LEN)
  , mProductName(c.productName, MAX_EFFECT_NAME_LEN)
  , mMfrName(c.mfrName, MAX_EFFECT_NAME_LEN)
  , mAPI(plugAPI)
{
  Trace(TRACELOC, "%s:%s", c.effectName, CurrentTime());

  for (int i = 0; i < c.nParams; ++i)
    mParams.Add(new IParam());
  
  for (int i = 0; i < c.nPresets; ++i)
    mPresets.Add(new IPreset());
}

IPlugBase::~IPlugBase()
{
  TRACE;

  mParams.Empty(true);
  mPresets.Empty(true);
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

void IPlugBase::GetHostVersionStr(WDL_String& str)
{
  GetHost();
  GetVersionStr(mHostVersion, str);
}

void IPlugBase::SetHost(const char* host, int version)
{
  mHost = LookUpHost(host);
  mHostVersion = version;

  WDL_String vStr;
  GetVersionStr(version, vStr);
  Trace(TRACELOC, "host_%sknown:%s:%s", (mHost == kHostUnknown ? "un" : ""), host, vStr.Get());
}

// Decimal = VVVVRRMM, otherwise 0xVVVVRRMM.
int IPlugBase::GetEffectVersion(bool decimal) const
{
  if (decimal)
    return GetDecimalVersion(mVersion);
  else
    return mVersion;
}

void IPlugBase::GetEffectVersionStr(WDL_String& str) const
{
  GetVersionStr(mVersion, str);
#if defined TRACER_BUILD
  str.Append("T");
#endif
#if defined _DEBUG
  str.Append("D");
#endif
}

const char* IPlugBase::GetAPIStr()
{
  switch (GetAPI()) 
  {
    case kAPIVST2: return "VST2";
    case kAPIVST3: return "VST3";
    case kAPIAU: return "AU";
    case kAPIAAX: return "AAX";
    case kAPISA: return "Standalone";
    default: return "";
  }
}

const char* IPlugBase::GetArchStr()
{
#ifdef ARCH_64BIT
  return "x64";
#else
  return "x86";
#endif
}

void IPlugBase::GetBuildInfoStr(WDL_String& str)
{
  WDL_String version;
  GetEffectVersionStr(version);
  str.SetFormatted(MAX_BUILD_INFO_STR_LEN, "%s version %s %s %s, built on %s at %.5s ", GetEffectName(), version.Get(), GetArchStr(), GetAPIStr(), __DATE__, __TIME__);
}

// this is over-ridden for AAX
void IPlugBase::SetParameterFromUI(int idx, double normalizedValue)
{
  Trace(TRACELOC, "%d:%f", idx, normalizedValue);
  GetParam(idx)->SetNormalized(normalizedValue);
  InformHostOfParamChange(idx, normalizedValue);
  OnParamChange(idx, kGUI);
}

void IPlugBase::OnParamReset(EParamSource source)
{
  for (int i = 0; i < mParams.GetSize(); ++i)
  {
    OnParamChange(i, source);
  }
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

void IPlugBase::MakeDefaultPreset(const char* name, int nPresets)
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

void IPlugBase::MakePreset(const char* name, ...)
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

void IPlugBase::MakePresetFromNamedParams(const char* name, int nParamsNamed, ...)
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

void IPlugBase::MakePresetFromChunk(const char* name, IByteChunk& chunk)
{
  IPreset* pPreset = GetNextUninitializedPreset(&mPresets);
  if (pPreset)
  {
    pPreset->mInitialized = true;
    strcpy(pPreset->mName, name);

    pPreset->mChunk.PutChunk(&chunk);
  }
}

void IPlugBase::MakePresetFromBlob(const char* name, const char* blob, int sizeOfChunk)
{
  IByteChunk presetChunk;
  presetChunk.Resize(sizeOfChunk);
  wdl_base64decode(blob, presetChunk.GetBytes(), sizeOfChunk);

  MakePresetFromChunk(name, presetChunk);
}

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
      restoredOK = SerializeState(pPreset->mChunk);
    }
    else
    {
      restoredOK = (UnserializeState(pPreset->mChunk, 0) > 0);
    }

    if (restoredOK)
    {
      mCurrentPresetIdx = idx;
      PresetsChangedByHost();
      RedrawParamControls();
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

    SerializeState(pPreset->mChunk);

    if (CSTR_NOT_EMPTY(name))
    {
      strcpy(pPreset->mName, name);
    }
  }
}

bool IPlugBase::SerializePresets(IByteChunk& chunk)
{
  TRACE;
  bool savedOK = true;
  int n = mPresets.GetSize();
  for (int i = 0; i < n && savedOK; ++i)
  {
    IPreset* pPreset = mPresets.Get(i);
    chunk.PutStr(pPreset->mName);

    Trace(TRACELOC, "%d %s", i, pPreset->mName);

    chunk.PutBool(pPreset->mInitialized);
    if (pPreset->mInitialized)
    {
      savedOK &= (chunk.PutChunk(&(pPreset->mChunk)) > 0);
    }
  }
  return savedOK;
}

int IPlugBase::UnserializePresets(IByteChunk& chunk, int startPos)
{
  TRACE;
  WDL_String name;
  int n = mPresets.GetSize(), pos = startPos;
  for (int i = 0; i < n && pos >= 0; ++i)
  {
    IPreset* pPreset = mPresets.Get(i);
    pos = chunk.GetStr(name, pos);
    strcpy(pPreset->mName, name.Get());

    Trace(TRACELOC, "%d %s", i, pPreset->mName);

    pos = chunk.GetBool(&(pPreset->mInitialized), pos);
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

bool IPlugBase::SerializeParams(IByteChunk& chunk)
{
  TRACE;
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

int IPlugBase::UnserializeParams(IByteChunk& chunk, int startPos)
{
  TRACE;
  WDL_MutexLock lock(&mParams_mutex); 
  int i, n = mParams.GetSize(), pos = startPos;
  for (i = 0; i < n && pos >= 0; ++i)
  {
    IParam* pParam = mParams.Get(i);
    double v = 0.0;
    pos = chunk.Get(&v, pos);
    pParam->Set(v);
    Trace(TRACELOC, "%d %s %f", i, pParam->GetNameForHost(), pParam->Value());
  }
  OnParamReset(kPresetRecall);
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

void IPlugBase::DirtyParameters()
{
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

void IPlugBase::DumpPresetBlob(const char* filename)
{
  FILE* fp = fopen(filename, "w");
  fprintf(fp, "MakePresetFromBlob(\"name\", \"");

  char buf[MAX_BLOB_LENGTH];

  IByteChunk* pPresetChunk = &mPresets.Get(mCurrentPresetIdx)->mChunk;
  uint8_t* byteStart = pPresetChunk->GetBytes();

  wdl_base64encode(byteStart, buf, pPresetChunk->Size());

  fprintf(fp, "%s\", %i);\n", buf, pPresetChunk->Size());
  fclose(fp);
}

void IPlugBase::DumpBankBlob(const char* filename)
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
    wdl_base64encode(pPresetChunk->GetBytes(), buf, pPresetChunk->Size());
    
    fprintf(fp, "%s\", %i);\n", buf, pPresetChunk->Size());
  }
  
  fclose(fp);
}

const int kFXPVersionNum = 1;
const int kFXBVersionNum = 2;

// confusing... IByteChunk will force storage as little endian on big endian platforms,
// so when we use it here, since vst fxp/fxb files are big endian, we need to swap the endianess
// regardless of the endianness of the host, and on big endian hosts it will get swapped back to
// big endian
bool IPlugBase::SaveProgramAsFXP(const char* file)
{
  if (CSTR_NOT_EMPTY(file))
  {
    FILE* fp = fopen(file, "wb");

    IByteChunk pgm;

    int32_t chunkMagic = WDL_bswap32('CcnK');
    int32_t byteSize = 0;
    int32_t fxpMagic;
    int32_t fxpVersion = WDL_bswap32(kFXPVersionNum);
    int32_t pluginID = WDL_bswap32(GetUniqueID());
    int32_t pluginVersion = WDL_bswap32(GetEffectVersion(true));
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

      InitChunkWithIPlugVer(state);
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
  return false;
}

bool IPlugBase::SaveBankAsFXB(const char* file)
{
  if (CSTR_NOT_EMPTY(file))
  {
    FILE* fp = fopen(file, "wb");

    IByteChunk bnk;

    int32_t chunkMagic = WDL_bswap32('CcnK');
    int32_t byteSize = 0;
    int32_t fxbMagic;
    int32_t fxbVersion = WDL_bswap32(kFXBVersionNum);
    int32_t pluginID = WDL_bswap32(GetUniqueID());
    int32_t pluginVersion = WDL_bswap32(GetEffectVersion(true));
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

      InitChunkWithIPlugVer(state);
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
  else
    return false;
}

bool IPlugBase::LoadProgramFromFXP(const char* file)
{
  if (CSTR_NOT_EMPTY(file))
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
      fread(pgm.GetBytes(), fileSize, 1, fp);

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
      //if (pluginVersion != GetEffectVersion(true)) return false; // TODO: provide mechanism for loading earlier versions
      //if (numParams != NParams()) return false; // TODO: provide mechanism for loading earlier versions with less params

      if (DoesStateChunks() && fxpMagic == 'FPCh')
      {
        int32_t chunkSize;
        pos = pgm.Get(&chunkSize, pos);
        chunkSize = WDL_bswap_if_le(chunkSize);

        GetIPlugVerFromChunk(pgm, pos);
        UnserializeState(pgm, pos);
        ModifyCurrentPreset(prgName);
        RestorePreset(GetCurrentPresetIdx());
        InformHostOfProgramChange();

        return true;
      }
      else if (fxpMagic == 'FxCk')
      {
        mParams_mutex.Enter();
        for (int i = 0; i< NParams(); i++)
        {
          WDL_EndianFloat v32;
          pos = pgm.Get(&v32.int32, pos);
          v32.int32 = WDL_bswap_if_le(v32.int32);
          mParams.Get(i)->SetNormalized((double) v32.f);
        }
        mParams_mutex.Leave();

        ModifyCurrentPreset(prgName);
        RestorePreset(GetCurrentPresetIdx());
        InformHostOfProgramChange();

        return true;
      }
    }
  }

  return false;
}

bool IPlugBase::LoadBankFromFXB(const char* file)
{
  if (CSTR_NOT_EMPTY(file))
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
      fread(bnk.GetBytes(), fileSize, 1, fp);

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
      //if (pluginVersion != GetEffectVersion(true)) return false; // TODO: provide mechanism for loading earlier versions
      //if (numPgms != NPresets()) return false; // TODO: provide mechanism for loading earlier versions with less params

      if (DoesStateChunks() && fxbMagic == 'FBCh')
      {
        int32_t chunkSize;
        pos = bnk.Get(&chunkSize, pos);
        chunkSize = WDL_bswap_if_le(chunkSize);

        GetIPlugVerFromChunk(bnk, pos);
        UnserializePresets(bnk, pos);
        //RestorePreset(currentPgm);
        InformHostOfProgramChange();
        return true;
      }
      else if (fxbMagic == 'FxBk')
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

          mParams_mutex.Enter();
          for (int j = 0; j< NParams(); j++)
          {
            WDL_EndianFloat v32;
            pos = bnk.Get(&v32.int32, pos);
            v32.int32 = WDL_bswap_if_le(v32.int32);
            mParams.Get(j)->SetNormalized((double) v32.f);
          }
          mParams_mutex.Leave();

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

void IPlugBase::InitChunkWithIPlugVer(IByteChunk& chunk)
{
  chunk.Clear();
  int magic = IPLUG_VERSION_MAGIC;
  chunk.Put(&magic);
  int ver = IPLUG_VERSION;
  chunk.Put(&ver);
}

int IPlugBase::GetIPlugVerFromChunk(IByteChunk& chunk, int& position)
{
  int magic = 0, ver = 0;
  int magicpos = chunk.Get(&magic, position);
  
  if (magicpos > position && magic == IPLUG_VERSION_MAGIC)
    position = chunk.Get(&ver, magicpos);
  
  return ver;
}

void IPlugBase::PrintDebugInfo()
{
  WDL_String buildInfo;
  GetBuildInfoStr(buildInfo);
  DBGMSG("%s\n NO_IGRAPHICS\n", buildInfo.Get());
}
