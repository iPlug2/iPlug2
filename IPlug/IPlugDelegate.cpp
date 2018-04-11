#include "IPlugDelegate.h"


IDelegate::IDelegate(int nParams, int nPresets/*not used*/)
{
  for (int i = 0; i < nParams; ++i)
    mParams.Add(new IParam());
}

IDelegate::~IDelegate()
{
  mParams.Empty(true);
}

void IDelegate::OnParamChange(int paramIdx, EParamSource source)
{
  Trace(TRACELOC, "idx:%i src:%s\n", paramIdx, ParamSourceStrs[source]);
  OnParamChange(paramIdx);
}

bool IDelegate::SerializeParams(IByteChunk& chunk)
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

int IDelegate::UnserializeParams(const IByteChunk& chunk, int startPos)
{
  TRACE;
  int i, n = mParams.GetSize(), pos = startPos;
  ENTER_PARAMS_MUTEX;
  for (i = 0; i < n && pos >= 0; ++i)
  {
    IParam* pParam = mParams.Get(i);
    double v = 0.0;
    pos = chunk.Get(&v, pos);
    pParam->Set(v);
    Trace(TRACELOC, "%d %s %f", i, pParam->GetNameForHost(), pParam->Value());
  }
  //  OnParamReset(kPresetRecall); // TODO: fix this!
  LEAVE_PARAMS_MUTEX;
  return pos;
}

void IDelegate::InitFromDelegate(IDelegate& delegate)
{
  for (auto p = 0; p < delegate.NParams(); p++)
  {
    IParam* pParam = delegate.GetParam(p);
    GetParam(p)->Init(*pParam);
    GetParam(p)->Set(pParam->Value());
  }
}

void IDelegate::InitParamRange(int startIdx, int endIdx, int countStart, const char* nameFmtStr, double defaultVal, double minVal, double maxVal, double step, const char *label, int flags, const char *group, IParam::Shape *shape, IParam::EParamUnit unit, IParam::DisplayFunc displayFunc)
{
  WDL_String nameStr;
  for (auto p = startIdx; p <= endIdx; p++)
  {
    nameStr.SetFormatted(MAX_PARAM_NAME_LEN, nameFmtStr, countStart + (p-startIdx));
    GetParam(p)->InitDouble(nameStr.Get(), defaultVal, minVal, maxVal, step, label, flags, group, shape, unit, displayFunc);
  }
}

void IDelegate::CloneParamRange(int cloneStartIdx, int cloneEndIdx, int startIdx, const char* searchStr, const char* replaceStr, const char* newGroup)
{
  for (auto p = cloneStartIdx; p <= cloneEndIdx; p++)
  {
    IParam* pParam = GetParam(p);
    int outIdx = startIdx + (p - cloneStartIdx);
    GetParam(outIdx)->Init(*pParam, searchStr, replaceStr, newGroup);
    GetParam(outIdx)->Set(pParam->Value());
  }
}

void IDelegate::CopyParamValues(int startIdx, int destIdx, int nParams)
{
  assert((startIdx + nParams) < NParams());
  assert((destIdx + nParams) < NParams());
  assert((startIdx + nParams) < destIdx);
  
  for (auto p = startIdx; p < startIdx + nParams; p++)
  {
    GetParam(destIdx++)->Set(GetParam(p)->Value());
  }
}

void IDelegate::CopyParamValues(const char* inGroup, const char *outGroup)
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

void IDelegate::ModifyParamValues(int startIdx, int endIdx, std::function<void(IParam&)>func)
{
  for (auto p = startIdx; p <= endIdx; p++)
  {
    func(* GetParam(p));
  }
}

void IDelegate::ModifyParamValues(const char* paramGroup, std::function<void (IParam &)> func)
{
  for (auto p = 0; p < NParams(); p++)
  {
    IParam* pParam = GetParam(p);
    if(strcmp(pParam->GetGroupForHost(), paramGroup) == 0)
    {
      func(*pParam);
    }
  }
}

void IDelegate::DefaultParamValues(int startIdx, int endIdx)
{
  ModifyParamValues(startIdx, endIdx, [](IParam& param)
                    {
                      param.SetToDefault();
                    });
}

void IDelegate::DefaultParamValues(const char* paramGroup)
{
  ModifyParamValues(paramGroup, [](IParam& param)
                    {
                      param.SetToDefault();
                    });
}

void IDelegate::RandomiseParamValues(int startIdx, int endIdx)
{
  std::random_device rd;
  std::default_random_engine gen(rd());
  std::uniform_real_distribution<> dis(0., 1.);
  
  ModifyParamValues(startIdx, endIdx, [&gen, &dis](IParam& param)
                    {
                      param.SetNormalized(dis(gen));
                    });
}

void IDelegate::RandomiseParamValues(const char *paramGroup)
{
  std::random_device rd;
  std::default_random_engine gen(rd());
  std::uniform_real_distribution<> dis(0., 1.);
  
  ModifyParamValues(paramGroup, [&gen, &dis](IParam& param)
                    {
                      param.SetNormalized(dis(gen));
                    });
}
