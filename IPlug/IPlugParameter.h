#pragma once

#include <cstring>

#include "wdlstring.h"

#include "IPlugUtilities.h"

/** IPlug's parameter class */
class IParam
{
public:
  enum EParamType { kTypeNone, kTypeBool, kTypeInt, kTypeEnum, kTypeDouble };

  IParam();
  ~IParam() {};

  EParamType Type() const { return mType; }

  void InitBool(const char* name, bool defaultValue, const char* label = "", const char* group = ""); // LABEL not used here
  void InitEnum(const char* name, int defaultValue, int nEnums, const char* label = "", const char* group = ""); // LABEL not used here
  void InitInt(const char* name, int defaultValue, int minVal, int maxVal, const char* label = "", const char* group = "");
  void InitDouble(const char* name, double defaultVal, double minVal, double maxVal, double step, const char* label = "", const char* group = "", double shape = 1.);

  /** Sets the parameter value
   * @param value Value to be set. Will be clamped between \c mMin and \c mMax
   */
  void Set(double value) { mValue = BOUNDED(value, mMin, mMax); }
  void SetDisplayText(double value, const char* text);
  void SetCanAutomate(bool canAutomate) { mCanAutomate = canAutomate; }
  // The higher the shape, the more resolution around host value zero.
  void SetShape(double shape);
  void SetIsMeta(bool meta) { mIsMeta = meta; }
  void SetToDefault() { mValue = mDefault; }

  // Call this if your param is (x, y) but you want to always display (-x, -y)
  void NegateDisplay() { mNegateDisplay = true; }
  bool GetDisplayIsNegated() const { return mNegateDisplay; }

  //call this to make sure the param display text allways has a sign
  void SignDisplay() { mSignDisplay = true; }

  // Accessors / converters.
  // These all return the readable value, not the VST (0,1).
  /** Gets a readable value of the parameter
   * @return Current value of the parameter
   */
  double Value() const { return mValue; }
  /** Returns the parameter's value as a boolean
   * @return \c True if value >= 0.5, else otherwise
   */
  bool Bool() const { return (mValue >= 0.5); }
  /** Returns the parameter's value as an integer
   * @return Current value of the parameter
   */
  int Int() const { return int(mValue); }
  double DBToAmp() const;
  double Clamp(double value) const { return BOUNDED(value, mMin, mMax); }
    
  void SetNormalized(double normalizedValue);
  double GetNormalized() const;
  double GetNormalized(double nonNormalizedValue) const;
  double GetNonNormalized(double normalizedValue) const;

  void GetDisplayForHost(WDL_String& display, bool withDisplayText = true) { GetDisplayForHost(mValue, false, display, withDisplayText); }
  void GetDisplayForHost(double value, bool normalized, WDL_String& display, bool withDisplayText = true);
  
  double StringToValue(const char* str);
  
  const char* GetNameForHost() const;
  const char* GetLabelForHost() const;
  const char* GetParamGroupForHost() const;
  
  int NDisplayTexts() const;
  const char* GetDisplayText(int value) const;
  const char* GetDisplayTextAtIdx(int idx, double* pValue = nullptr) const;
  bool MapDisplayText(const char* pStr, double* pValue) const;  // Reverse map back to value.
  
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
  
  void GetJSON(WDL_String& json, int idx) const;

private:
  EParamType mType = kTypeNone;
  double mValue = 0.0;
  double mMin = 0.0;
  double mMax = 1.0;
  double mStep = 1.0;
  double mShape = 1.0;
  double mDefault = 0.0;
  int mDisplayPrecision = 0;
  bool mNegateDisplay = false;
  bool mSignDisplay = false;
  bool mCanAutomate = true;
  bool mIsMeta = false;
  char mName[MAX_PARAM_NAME_LEN];
  char mLabel[MAX_PARAM_LABEL_LEN];
  char mParamGroup[MAX_PARAM_GROUP_LEN];
  
  struct DisplayText
  {
    double mValue;
    char mText[MAX_PARAM_DISPLAY_LEN];
  };
  
  WDL_TypedBuf<DisplayText> mDisplayTexts;
} WDL_FIXALIGN;
