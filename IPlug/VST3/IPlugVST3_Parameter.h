/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

class IPlugVST3Parameter : public Parameter
{
public:
  IPlugVST3Parameter(IParam* pParam, ParamID tag, UnitID unitID)
  : mIPlugParam(pParam)
  {
    UString(info.title, str16BufferSize(String128)).assign(pParam->GetNameForHost());
    UString(info.units, str16BufferSize(String128)).assign(pParam->GetLabelForHost());

    precision = pParam->GetDisplayPrecision();

    if (pParam->Type() != IParam::kTypeDouble)
      info.stepCount = pParam->GetRange();
    else
      info.stepCount = 0; // continuous

    int32 flags = 0;

    if (pParam->GetCanAutomate()) flags |= ParameterInfo::kCanAutomate;
    if (pParam->Type() == IParam::kTypeEnum) flags |= ParameterInfo::kIsList;

    info.defaultNormalizedValue = valueNormalized = pParam->ToNormalized(pParam->GetDefault());
    info.flags = flags;
    info.id = tag;
    info.unitId = unitID;
  }

  virtual void toString(ParamValue valueNormalized, String128 string) const override
  {
    WDL_String display;
    mIPlugParam->GetDisplayForHost(valueNormalized, true, display);
    Steinberg::UString(string, 128).fromAscii(display.Get());
  }

  virtual bool fromString(const TChar* string, ParamValue& valueNormalized) const override
  {
    String str((TChar*)string);
    valueNormalized = mIPlugParam->ToNormalized(atof(str.text8()));

    return true;
  }

  virtual Steinberg::Vst::ParamValue toPlain(ParamValue valueNormalized) const override
  {
    return mIPlugParam->FromNormalized(valueNormalized);
  }

  virtual Steinberg::Vst::ParamValue toNormalized(ParamValue plainValue) const override
  {
    return mIPlugParam->ToNormalized(valueNormalized);
  }

  OBJ_METHODS(IPlugVST3Parameter, Parameter)

protected:
  IParam* mIPlugParam = nullptr;
};
