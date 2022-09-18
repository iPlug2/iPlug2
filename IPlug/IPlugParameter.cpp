/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

/**
 * @file
 * @brief IParam implementation
 */

#include <cstdio>
#include <algorithm>

#include "IPlugParameter.h"
#include "IPlugLogger.h"

using namespace iplug;

#pragma mark - Shape

double IParam::ShapeLinear::NormalizedToValue(double value, const IParam& param) const
{
  return param.mMin + value * (param.mMax - param.mMin);
}

double IParam::ShapeLinear::ValueToNormalized(double value, const IParam& param) const
{
  return (value - param.mMin) / (param.mMax - param.mMin);
}

IParam::ShapePowCurve::ShapePowCurve(double shape)
: mShape(shape)
{
}

IParam::EDisplayType IParam::ShapePowCurve::GetDisplayType() const
{
  if (mShape > 2.5) return kDisplayCubeRoot;
  if (mShape > 1.5) return kDisplaySquareRoot;
  if (mShape < (2.0 / 5.0)) return kDisplayCubed;
  if (mShape < (2.0 / 3.0)) return kDisplaySquared;
  
  return IParam::kDisplayLinear;
}

double IParam::ShapePowCurve::NormalizedToValue(double value, const IParam& param) const
{
  return param.GetMin() + std::pow(value, mShape) * (param.GetMax() - param.GetMin());
}

double IParam::ShapePowCurve::ValueToNormalized(double value, const IParam& param) const
{
  return std::pow((value - param.GetMin()) / (param.GetMax() - param.GetMin()), 1.0 / mShape);
}

void IParam::ShapeExp::Init(const IParam& param)
{
  double min = param.GetMin();
  
  if(min <= 0.)
    min = 0.00000001;
  
  mAdd = std::log(min);
  mMul = std::log(param.GetMax() / min);
}

double IParam::ShapeExp::NormalizedToValue(double value, const IParam& param) const
{
  return std::exp(mAdd + value * mMul);
}

double IParam::ShapeExp::ValueToNormalized(double value, const IParam& param) const
{
  return (std::log(value) - mAdd) / mMul;
}

#pragma mark -

IParam::IParam()
{
  mShape = std::make_unique<ShapeLinear>();
  memset(mName, 0, MAX_PARAM_NAME_LEN * sizeof(char));
  memset(mLabel, 0, MAX_PARAM_LABEL_LEN * sizeof(char));
  memset(mParamGroup, 0, MAX_PARAM_LABEL_LEN * sizeof(char));
};

void IParam::InitBool(const char* name, bool defaultVal, const char* label, int flags, const char* group, const char* offText, const char* onText)
{
  if (mType == kTypeNone) mType = kTypeBool;
  
  InitEnum(name, (defaultVal ? 1 : 0), 2, label, flags | kFlagStepped, group);

  SetDisplayText(0, offText);
  SetDisplayText(1, onText);
}

void IParam::InitEnum(const char* name, int defaultVal, int nEnums, const char* label, int flags, const char* group, const char* listItems, ...)
{
  if (mType == kTypeNone) mType = kTypeEnum;
  
  InitInt(name, defaultVal, 0, nEnums - 1, label, flags | kFlagStepped, group);
  
  if(listItems)
  {
    SetDisplayText(0, listItems);

    va_list args;
    va_start(args, listItems);
    for (auto i = 1; i < nEnums; ++i)
      SetDisplayText(i, va_arg(args, const char*));
    va_end(args);
  }
}

void IParam::InitEnum(const char* name, int defaultVal, const std::initializer_list<const char*>& listItems, int flags, const char* group)
{
  if (mType == kTypeNone) mType = kTypeEnum;

  InitInt(name, defaultVal, 0, static_cast<int>(listItems.size()) - 1, "", flags | kFlagStepped, group);

  int idx = 0;
  for (auto& item : listItems)
  {
    SetDisplayText(idx++, item);
  }
}

void IParam::InitInt(const char* name, int defaultVal, int minVal, int maxVal, const char* label, int flags, const char* group)
{
  if (mType == kTypeNone) mType = kTypeInt;
  
  InitDouble(name, (double) defaultVal, (double) minVal, (double) maxVal, 1.0, label, flags | kFlagStepped, group);
}

void IParam::InitDouble(const char* name, double defaultVal, double minVal, double maxVal, double step, const char* label, int flags, const char* group, const Shape& shape, EParamUnit unit, DisplayFunc displayFunc)
{
  if (mType == kTypeNone) mType = kTypeDouble;
  
//  assert(CStringHasContents(mName) && "Parameter already initialised!");
//  assert(CStringHasContents(name) && "Parameter must be given a name!");

  strcpy(mName, name);
  strcpy(mLabel, label);
  strcpy(mParamGroup, group);
  
  // N.B. apply stepping and constraints to the default value (and store the result)
  mMin = minVal;
  mMax = std::max(maxVal, minVal + step);
  mStep = step;
  mDefault = defaultVal;
  mUnit = unit;
  mFlags = flags;
  mDisplayFunction = displayFunc;

  Set(defaultVal);
  
  for (mDisplayPrecision = 0;
       mDisplayPrecision < MAX_PARAM_DISPLAY_PRECISION && step != floor(step);
       ++mDisplayPrecision, step *= 10.0)
  {
    ;
  }
    
  mShape = std::unique_ptr<Shape>(shape.Clone());
  mShape->Init(*this);
}

void IParam::InitFrequency(const char *name, double defaultVal, double minVal, double maxVal, double step, int flags, const char *group)
{
  InitDouble(name, defaultVal, minVal, maxVal, step, "Hz", flags, group, ShapeExp(), kUnitFrequency);
}

void IParam::InitSeconds(const char *name, double defaultVal, double minVal, double maxVal, double step, int flags, const char *group)
{
  InitDouble(name, defaultVal, minVal, maxVal, step, "Seconds", flags, group, ShapeLinear(), kUnitSeconds);
}

void IParam::InitMilliseconds(const char *name, double defaultVal, double minVal, double maxVal, int flags, const char *group)
{
  InitDouble(name, defaultVal, minVal, maxVal, 1, "ms", flags, group, ShapeLinear(), kUnitMilliseconds);
}

void IParam::InitPitch(const char *name, int defaultVal, int minVal, int maxVal, int flags, const char *group, bool middleCisC)
{
  InitEnum(name, defaultVal, (maxVal - minVal) + 1, "", flags, group);
  WDL_String displayText;
  for (auto i = minVal; i <= maxVal; i++)
  {
    MidiNoteName(i, displayText, /*cents*/false, middleCisC);
    SetDisplayText(i - minVal, displayText.Get());
  }
}

void IParam::InitGain(const char *name, double defaultVal, double minVal, double maxVal, double step, int flags, const char *group)
{
  InitDouble(name, defaultVal, minVal, maxVal, step, "dB", flags, group, ShapeLinear(), kUnitDB);
}

void IParam::InitPercentage(const char *name, double defaultVal, double minVal, double maxVal, int flags, const char *group)
{
  InitDouble(name, defaultVal, minVal, maxVal, 1, "%", flags, group, ShapeLinear(), kUnitPercentage);
}

void IParam::InitAngleDegrees(const char *name, double defaultVal, double minVal, double maxVal, int flags, const char *group)
{
  InitDouble(name, defaultVal, minVal, maxVal, 1, "degrees", flags, group, ShapeLinear(), kUnitDegrees);
}

void IParam::Init(const IParam& p, const char* searchStr, const char* replaceStr, const char* newGroup)
{
  if (mType == kTypeNone) mType = p.Type();

  WDL_String str(p.mName);
  WDL_String group(p.mParamGroup);
  
  if (CStringHasContents(searchStr))
  {
    char* pos = strstr(str.Get(), searchStr);
    
    if(pos)
    {
      int insertionPos = static_cast<int>(str.Get() - pos);
      str.DeleteSub(insertionPos, static_cast<int>(strlen(searchStr)));
      str.Insert(replaceStr, insertionPos);
    }
  }
  
  if (CStringHasContents(newGroup))
  {
    group.Set(newGroup);
  }
  
  InitDouble(str.Get(), p.mDefault, p.mMin, p.mMax, p.mStep, p.mLabel, p.mFlags, group.Get(), *p.mShape, p.mUnit, p.mDisplayFunction);
  
  for (auto i=0; i<p.NDisplayTexts(); i++)
  {
    double val;
    const char* str = p.GetDisplayTextAtIdx(i, &val);
    SetDisplayText(val, str);
  }
}

void IParam::SetDisplayText(double value, const char* str)
{
  int n = mDisplayTexts.GetSize();
  mDisplayTexts.Resize(n + 1);
  DisplayText* pDT = mDisplayTexts.Get() + n;
  pDT->mValue = value;
  strcpy(pDT->mText, str);
}

void IParam::SetDisplayPrecision(int precision)
{
  mDisplayPrecision = precision;
}

void IParam::GetDisplay(double value, bool normalized, WDL_String& str, bool withDisplayText) const
{
  if (normalized) value = FromNormalized(value);

  if (mDisplayFunction != nullptr)
  {
    mDisplayFunction(value, str);
    return;
  }

  if (withDisplayText)
  {
    const char* displayText = GetDisplayText(value);

    if (CStringHasContents(displayText))
    {
      str.Set(displayText, MAX_PARAM_DISPLAY_LEN);
      return;
    }
  }

  double displayValue = value;

  if (mFlags & kFlagNegateDisplay)
    displayValue = -displayValue;

  // Squash all zeros to positive
  if (!displayValue) displayValue = 0.0;

  if (mDisplayPrecision == 0)
  {
    str.SetFormatted(MAX_PARAM_DISPLAY_LEN, "%d", static_cast<int>(round(displayValue)));
  }
  else if ((mFlags & kFlagSignDisplay) && displayValue)
  {
    char fmt[16];
    snprintf(fmt, 16, "%%+.%df", mDisplayPrecision);
    str.SetFormatted(MAX_PARAM_DISPLAY_LEN, fmt, displayValue);
  }
  else
  {
    str.SetFormatted(MAX_PARAM_DISPLAY_LEN, "%.*f", mDisplayPrecision, displayValue);
  }
}

const char* IParam::GetName() const
{
  return mName;
}

const char* IParam::GetLabel() const
{
  return (CStringHasContents(GetDisplayText(static_cast<int>(mValue.load())))) ? "" : mLabel;
}

const char* IParam::GetGroup() const
{
  return mParamGroup;
}

int IParam::NDisplayTexts() const
{
  return mDisplayTexts.GetSize();
}

const char* IParam::GetDisplayText(double value) const
{
  int n = mDisplayTexts.GetSize();
  for (DisplayText* pDT = mDisplayTexts.Get(); n; --n, ++pDT)
  {
    if (value == pDT->mValue) return pDT->mText;
  }
  return "";
}

const char* IParam::GetDisplayTextAtIdx(int idx, double* pValue) const
{
  DisplayText* pDT = mDisplayTexts.Get()+idx;
  if (pValue) *pValue = pDT->mValue;
  return pDT->mText;
}

bool IParam::MapDisplayText(const char* str, double* pValue) const
{
  int n = mDisplayTexts.GetSize();
  for (DisplayText* pDT = mDisplayTexts.Get(); n; --n, ++pDT)
  {
    if (!strcmp(str, pDT->mText))
    {
      *pValue = pDT->mValue;
      return true;
    }
  }
  return false;
}

double IParam::StringToValue(const char* str) const
{
  double v = 0.;
  bool mapped = (bool) NDisplayTexts();

  if (mapped)
    mapped = MapDisplayText(str, &v);

  if (!mapped && Type() != kTypeEnum && Type() != kTypeBool)
  {
    v = atof(str);

    if (mFlags & kFlagNegateDisplay)
      v = -v;

    v = Constrain(v);
    mapped = true;
  }

  return v;
}

void IParam::GetBounds(double& lo, double& hi) const
{
  lo = mMin;
  hi = mMax;
}

void IParam::GetJSON(WDL_String& json, int idx) const
{
  json.AppendFormatted(8192, "{");
  json.AppendFormatted(8192, "\"id\":%i, ", idx);
  json.AppendFormatted(8192, "\"name\":\"%s\", ", GetName());
  switch (Type())
  {
    case IParam::kTypeNone:
      break;
    case IParam::kTypeBool:
      json.AppendFormatted(8192, "\"type\":\"%s\", ", "bool");
      break;
    case IParam::kTypeInt:
      json.AppendFormatted(8192, "\"type\":\"%s\", ", "int");
      break;
    case IParam::kTypeEnum:
      json.AppendFormatted(8192, "\"type\":\"%s\", ", "enum");
      break;
    case IParam::kTypeDouble:
      json.AppendFormatted(8192, "\"type\":\"%s\", ", "float");
      break;
    default:
      break;
  }
  json.AppendFormatted(8192, "\"min\":%f, ", GetMin());
  json.AppendFormatted(8192, "\"max\":%f, ", GetMax());
  json.AppendFormatted(8192, "\"default\":%f, ", GetDefault());
  json.AppendFormatted(8192, "\"rate\":\"control\"");
  json.AppendFormatted(8192, "}");
}

void IParam::PrintDetails() const
{
  DBGMSG("%s %f", GetName(), Value());
}
