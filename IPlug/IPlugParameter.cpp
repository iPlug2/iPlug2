#include <cstdio>
#include <algorithm>

#include "IPlugParameter.h"

IParam::IParam()
{
  memset(mName, 0, MAX_PARAM_NAME_LEN * sizeof(char));
  memset(mLabel, 0, MAX_PARAM_LABEL_LEN * sizeof(char));
  memset(mParamGroup, 0, MAX_PARAM_LABEL_LEN * sizeof(char));
};

void IParam::InitBool(const char* name, bool defaultVal, const char* label, const char* group)
{
  if (mType == kTypeNone) mType = kTypeBool;
  
  InitEnum(name, (defaultVal ? 1 : 0), 2, label, group);

  SetDisplayText(0, "off");
  SetDisplayText(1, "on");
}

void IParam::InitEnum(const char* name, int defaultVal, int nEnums, const char* label, const char* group)
{
  if (mType == kTypeNone) mType = kTypeEnum;
  
  InitInt(name, defaultVal, 0, nEnums - 1, label, group);
}

void IParam::InitInt(const char* name, int defaultVal, int minVal, int maxVal, const char* label, const char* group)
{
  if (mType == kTypeNone) mType = kTypeInt;
  
  InitDouble(name, (double) defaultVal, (double) minVal, (double) maxVal, 1.0, label, group);
}

void IParam::InitDouble(const char* name, double defaultVal, double minVal, double maxVal, double step, const char* label, const char* group, double shape)
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

  for (mDisplayPrecision = 0;
       mDisplayPrecision < MAX_PARAM_DISPLAY_PRECISION && step != floor(step);
       ++mDisplayPrecision, step *= 10.0)
  {
    ;
  }
  
  SetShape(shape);
}

void IParam::SetShape(double shape)
{
  if(shape != 0.0)
    mShape = shape;
}

void IParam::SetDisplayText(double value, const char* text)
{
  int n = mDisplayTexts.GetSize();
  mDisplayTexts.Resize(n + 1);
  DisplayText* pDT = mDisplayTexts.Get() + n;
  pDT->mValue = value;
  strcpy(pDT->mText, text);
}

double IParam::DBToAmp() const
{
  return ::DBToAmp(mValue);
}

void IParam::SetNormalized(double normalizedValue)
{
  mValue = FromNormalizedParam(normalizedValue, mMin, mMax, mShape);
  
  if (mType != kTypeDouble)
  {
    mValue = round(mValue / mStep) * mStep;
  }
  
  mValue = std::min(mValue, mMax);
}

double IParam::GetNormalized() const
{
  return GetNormalized(mValue);
}

double IParam::GetNormalized(double nonNormalizedValue) const
{
  nonNormalizedValue = BOUNDED(nonNormalizedValue, mMin, mMax);
  return ToNormalizedParam(nonNormalizedValue, mMin, mMax, mShape);
}

double IParam::GetNonNormalized(double normalizedValue) const
{
  return FromNormalizedParam(normalizedValue, mMin, mMax, mShape);
}

void IParam::GetDisplayForHost(double value, bool normalized, WDL_String& str, bool withDisplayText)
{
  if (normalized) value = FromNormalizedParam(value, mMin, mMax, mShape);

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
  {
    *pValue = pDT->mValue;
  }
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

void IParam::GetBounds(double& lo, double& hi) const
{
  lo = mMin;
  hi = mMax;
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
