/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

#include "pluginterfaces/base/ustring.h"
#include "public.sdk/source/vst/vstparameters.h"
#include "base/source/fstring.h"

#include "IPlugParameter.h"

BEGIN_IPLUG_NAMESPACE

using namespace Steinberg;
using namespace Vst;

/** VST3 parameter helper */
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

  void toString(ParamValue valueNormalized, String128 string) const override
  {
    WDL_String display;
    mIPlugParam->GetDisplayForHost(valueNormalized, true, display);
    Steinberg::UString(string, 128).fromAscii(display.Get());
  }

  bool fromString(const TChar* string, ParamValue& valueNormalized) const override
  {
    String str((TChar*)string);
    valueNormalized = mIPlugParam->ToNormalized(mIPlugParam->StringToValue(str.text8()));

    return true;
  }

  Steinberg::Vst::ParamValue toPlain(ParamValue valueNormalized) const override
  {
    return mIPlugParam->FromNormalized(valueNormalized);
  }

  Steinberg::Vst::ParamValue toNormalized(ParamValue plainValue) const override
  {
    return mIPlugParam->ToNormalized(valueNormalized);
  }

  OBJ_METHODS(IPlugVST3Parameter, Parameter)

protected:
  IParam* mIPlugParam = nullptr;
};

/** VST3 preset parameter helper */
class IPlugVST3PresetParameter : public Parameter
{
public:
    IPlugVST3PresetParameter(int nPresets)
    : Parameter(STR16("Preset"), kPresetParam, STR16(""), 0, nPresets, ParameterInfo::kIsProgramChange)
    {}
    
    OBJ_METHODS(IPlugVST3PresetParameter, Parameter)
};

/** VST3 bypass parameter helper */
class IPlugVST3BypassParameter : public StringListParameter
{
public:
  IPlugVST3BypassParameter()
  : StringListParameter(STR16("Bypass"), kBypassParam, 0, ParameterInfo::kCanAutomate | ParameterInfo::kIsBypass | ParameterInfo::kIsList)
  {
    appendString(STR16("off"));
    appendString(STR16("on"));
  }
  
  OBJ_METHODS(IPlugVST3BypassParameter, StringListParameter)
};

END_IPLUG_NAMESPACE

