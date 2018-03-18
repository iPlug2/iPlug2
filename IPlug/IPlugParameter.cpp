#include <cstdio>
#include <algorithm>

#include "IPlugParameter.h"

IParam::IParam()
{
  memset(mName, 0, MAX_PARAM_NAME_LEN * sizeof(char));
  memset(mLabel, 0, MAX_PARAM_LABEL_LEN * sizeof(char));
  memset(mParamGroup, 0, MAX_PARAM_LABEL_LEN * sizeof(char));
};

void IParam::InitBool(const char* name, bool defaultVal, const char* label, const char* group, const char* offText, const char* onText)
{
  if (mType == kTypeNone) mType = kTypeBool;
  
  InitEnum(name, (defaultVal ? 1 : 0), 2, label, group);

  SetDisplayText(0, offText);
  SetDisplayText(1, onText);
}

void IParam::InitEnum(const char* name, int defaultVal, int nEnums, const char* label, const char* group, const char* listItems, ...)
{
  if (mType == kTypeNone) mType = kTypeEnum;
  
  InitInt(name, defaultVal, 0, nEnums - 1, label, group);
  
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

void IParam::InitInt(const char* name, int defaultVal, int minVal, int maxVal, const char* label, const char* group)
{
  if (mType == kTypeNone) mType = kTypeInt;
  
  InitDouble(name, (double) defaultVal, (double) minVal, (double) maxVal, 1.0, label, group);
}

void IParam::InitDouble(const char* name, double defaultVal, double minVal, double maxVal, double step, const char* label, const char* group, Shape* shape, EParamUnit unit)
{
  if (mType == kTypeNone) mType = kTypeDouble;
  
  strcpy(mName, name);
  strcpy(mLabel, label);
  strcpy(mParamGroup, group);
  mValue = defaultVal;
  mMin = minVal;
  mMax = std::max(maxVal, minVal + step);
  mStep = step;
  mDefault = defaultVal;
  mUnit = unit;

  for (mDisplayPrecision = 0;
       mDisplayPrecision < MAX_PARAM_DISPLAY_PRECISION && step != floor(step);
       ++mDisplayPrecision, step *= 10.0)
  {
    ;
  }
    
  assert (!mShape && "Parameter has already been initialised!");
  mShape = shape ? shape : new Shape;
  mShape->Init(*this);
}

void IParam::InitFrequency(const char *name, double defaultVal, double minVal, double maxVal, double step, const char *group)
{
  InitDouble(name, defaultVal, minVal, maxVal, step, "Hz", group, new ShapeExp, kUnitFrequency);
  //TODO: shape
}

void IParam::InitSeconds(const char *name, double defaultVal, double minVal, double maxVal, double step, const char *group)
{
  InitDouble(name, defaultVal, minVal, maxVal, step, "Seconds", group, nullptr, kUnitSeconds);
  //TODO: shape
}

void IParam::InitPitch(const char *name, int defaultVal, int minVal, int maxVal, const char *group)
{
  int nItems = maxVal - minVal;
  InitEnum(name, defaultVal, nItems, "", group);
  WDL_String displayText;
  for (auto i = 0; i < nItems; i++)
  {
    MidiNoteName(minVal + i, displayText);
    SetDisplayText(i, displayText.Get());
  }
}

void IParam::InitGain(const char *name, double defaultVal, double minVal, double maxVal, double step, const char *group)
{
  InitDouble(name, defaultVal, minVal, maxVal, step, "dB", group, nullptr, kUnitDB);
}

void IParam::InitPercentage(const char *name, double defaultVal, double minVal, double maxVal, const char *group)
{
  InitDouble(name, defaultVal, minVal, maxVal, 1, "%", group, nullptr, kUnitPercentage);
}

void IParam::SetDisplayText(double value, const char* str)
{
  int n = mDisplayTexts.GetSize();
  mDisplayTexts.Resize(n + 1);
  DisplayText* pDT = mDisplayTexts.Get() + n;
  pDT->mValue = value;
  strcpy(pDT->mText, str);
}

double IParam::DBToAmp() const
{
  return ::DBToAmp(mValue);
}

void IParam::SetNormalized(double normalizedValue)
{
  mValue = FromNormalized(normalizedValue);
  
  if (mType != kTypeDouble)
  {
    mValue = round(mValue / mStep) * mStep;
  }
  
  mValue = std::min(mValue, mMax);
}

double IParam::GetNormalized() const
{
  return ToNormalized(mValue);
}

void IParam::GetDisplayForHost(double value, bool normalized, WDL_String& str, bool withDisplayText) const
{
  if (normalized) value = FromNormalized(value);

  if (withDisplayText)
  {
    const char* displayText = GetDisplayText((int) value);

    if (CSTR_NOT_EMPTY(displayText))
    {
      str.Set(displayText, MAX_PARAM_DISPLAY_LEN);
      return;
    }
  }

  double displayValue = value;

  if (mNegateDisplay)
    displayValue = -displayValue;

  // Squash all zeros to positive
  
  if (!displayValue) displayValue = 0.0;

  if (mDisplayPrecision == 0)
  {
    str.SetFormatted(MAX_PARAM_DISPLAY_LEN, "%d", int(round(displayValue)));
  }
  else if (mSignDisplay && displayValue)
  {
    char fmt[16];
    sprintf(fmt, "%%+.%df", mDisplayPrecision);
    str.SetFormatted(MAX_PARAM_DISPLAY_LEN, fmt, displayValue);
  }
  else
  {
    str.SetFormatted(MAX_PARAM_DISPLAY_LEN, "%.*f", mDisplayPrecision, displayValue);
  }
}

const char* IParam::GetNameForHost() const
{
  return mName;
}

const char* IParam::GetLabelForHost() const
{
  const char* displayText = GetDisplayText((int) mValue);
  return (CSTR_NOT_EMPTY(displayText)) ? "" : mLabel;
}

const char* IParam::GetParamGroupForHost() const
{
  return mParamGroup;
}

int IParam::NDisplayTexts() const
{
  return mDisplayTexts.GetSize();
}

const char* IParam::GetDisplayText(int value) const
{
  int n = mDisplayTexts.GetSize();
  if (n)
  {
    DisplayText* pDT = mDisplayTexts.Get();
    for (int i = 0; i < n; ++i, ++pDT)
    {
      if (value == pDT->mValue)
      {
        return pDT->mText;
      }
    }
  }
  return "";
}

const char* IParam::GetDisplayTextAtIdx(int idx, double* pValue) const
{
  DisplayText* pDT = mDisplayTexts.Get()+idx;
  
  if (pValue)
    *pValue = pDT->mValue;

  return pDT->mText;
}

bool IParam::MapDisplayText(const char* str, double* pValue) const
{
  int n = mDisplayTexts.GetSize();
  
  if (n)
  {
    DisplayText* pDT = mDisplayTexts.Get();
    for (int i = 0; i < n; ++i, ++pDT)
    {
      if (!strcmp(str, pDT->mText))
      {
        *pValue = pDT->mValue;
        return true;
      }
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
    
    if (GetDisplayIsNegated())
      v = -v;
    
    v = Clamp(v);
    mapped = true;
  }
  
  return v;
}

void IParam::GetBounds(double& lo, double& hi) const
{
  lo = mMin;
  hi = mMax;
}

IParam::MetaData IParam::GetMetaData() const
{
    MetaData data;
    
    data.mParamUnit = mUnit;
    data.mDisplayType = mShape->GetDisplayType();
    data.mCustomUnit = mUnit == kUnitCustom ? mLabel : nullptr;
    data.mMeta = mIsMeta;
    
    return data;
}

void IParam::GetJSON(WDL_String& json, int idx) const
{
  json.AppendFormatted(8192, "{");
  json.AppendFormatted(8192, "\"id\":%i, ", idx);
  json.AppendFormatted(8192, "\"name\":\"%s\", ", GetNameForHost());
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
  json.AppendFormatted(8192, "\"rate\":\"audio\"");
  json.AppendFormatted(8192, "}");
}


