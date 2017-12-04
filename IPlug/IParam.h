#pragma once

#include <cstring>
#include "heapbuf.h"
#include "IPlugUtilities.h"

class IParam
{
public:
  enum EParamType { kTypeNone, kTypeBool, kTypeInt, kTypeEnum, kTypeDouble };

  IParam();
  ~IParam();

  EParamType Type() { return mType; }

  void InitBool(const char* name, bool defaultValue, const char* label = "", const char* group = ""); // LABEL not used here
  void InitEnum(const char* name, int defaultValue, int nEnums, const char* label = "", const char* group = ""); // LABEL not used here
  void InitInt(const char* name, int defaultValue, int minVal, int maxVal, const char* label = "", const char* group = "");
  void InitDouble(const char* name, double defaultVal, double minVal, double maxVal, double step, const char* label = "", const char* group = "", double shape = 1.);

  void Set(double value) { mValue = BOUNDED(value, mMin, mMax); }
  void SetDisplayText(int value, const char* text);
  void SetCanAutomate(bool canAutomate) { mCanAutomate = canAutomate; }
  // The higher the shape, the more resolution around host value zero.
  void SetShape(double shape);
  void SetIsMeta(bool meta) { mIsMeta = meta; }
  void SetToDefault() { mValue = mDefault; }

  /** Call this if your param is (x, y) but you want to always display (-x, -y) */
  void NegateDisplay() { mNegateDisplay = true; }
  bool GetDisplayIsNegated() const { return mNegateDisplay; }

  //call this to make sure the param display text allways has a sign
  void SignDisplay() { mSignDisplay = true; }

  // Accessors / converters.
  // These all return the readable value, not the VST (0,1).
  double Value() const { return mValue; }
  bool Bool() const { return (mValue >= 0.5); }
  int Int() const { return int(mValue); }
  double DBToAmp() const;

  void SetNormalized(double normalizedValue);
  double GetNormalized() const;
  double GetNormalized(double nonNormalizedValue) const;
  double GetNonNormalized(double normalizedValue) const;

  void GetDisplayForHost(char* rDisplay) { GetDisplayForHost(mValue, false, rDisplay); }
  void GetDisplayForHostNoDisplayText(char* rDisplay) { GetDisplayForHost(mValue, false, rDisplay, false); }
  void GetDisplayForHost(double value, bool normalized, char* rDisplay, bool withDisplayText = true);
  const char* GetNameForHost() const;
  const char* GetLabelForHost() const;
  const char* GetParamGroupForHost() const;
  
  int GetNDisplayTexts() const;
  const char* GetDisplayText(int value) const;
  const char* GetDisplayTextAtIdx(int idx, int* value = 0) const;
  bool MapDisplayText(const char* pStr, int* pValue) const;  // Reverse map back to value.
  
  double GetShape() const { return mShape; }
  double GetStep() const { return mStep; }
  double GetDefault() const { return mDefault; }
  double GetDefaultNormalized() const { return ToNormalizedParam(mDefault, mMin, mMax, mShape); }
  double GetMin() const { return mMin; }
  double GetMax() const { return mMax; }
  void GetBounds(double& lo, double& hi) const;
  double GetRange() const { return mMax - mMin; }
  int GetPrecision() const {return mDisplayPrecision;}
  bool GetCanAutomate() const { return mCanAutomate; }
  bool GetIsMeta() const { return mIsMeta; }

private:
  EParamType mType;
  double mValue, mMin, mMax, mStep, mShape, mDefault;
  int mDisplayPrecision;
  char mName[MAX_PARAM_NAME_LEN];
  char mLabel[MAX_PARAM_LABEL_LEN];
  char mParamGroup[MAX_PARAM_LABEL_LEN];
  bool mNegateDisplay;
  bool mSignDisplay;
  bool mCanAutomate;
  bool mIsMeta;
  
  struct DisplayText
  {
    int mValue;
    char mText[MAX_PARAM_DISPLAY_LEN];
  };
  
  WDL_TypedBuf<DisplayText> mDisplayTexts;
};
