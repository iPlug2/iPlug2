#pragma once

#include <cstring>
#include <functional>

#include "wdlstring.h"

#include "IPlugUtilities.h"

/** IPlug's parameter class */
class IParam
{
public:

  enum EParamType { kTypeNone, kTypeBool, kTypeInt, kTypeEnum, kTypeDouble };
  enum EParamUnit { kUnitPercentage, kUnitSeconds, kUnitMilliseconds, kUnitSamples, kUnitDB, kUnitLinearGain, kUnitPan, kUnitPhase, kUnitDegrees, kUnitMeters, kUnitRate, kUnitRatio, kUnitFrequency, kUnitOctaves, kUnitCents, kUnitAbsCents, kUnitSemitones, kUnitMIDINote, kUnitMIDICtrlNum, kUnitBPM, kUnitBeats, kUnitCustom };
  enum EDisplayType { kDisplayLinear, kDisplayLog, kDisplayExp, kDisplaySquared, kDisplaySquareRoot, kDisplayCubed, kDisplayCubeRoot };
  
  struct MetaData
  {
    EDisplayType mDisplayType;
    EParamUnit mParamUnit;
    const char* mCustomUnit = nullptr;
    bool mMeta;
  };
  
  typedef std::function<void(double, WDL_String&)> DisplayFunc;
  
#pragma mark - Shape
  
  struct Shape
  {
    virtual ~Shape() {}
    
    virtual void Init(const IParam& param) {}
    
    virtual EDisplayType GetDisplayType() const { return kDisplayLinear; }
    
    virtual double NormalizedToValue(double value, const IParam& param) const
    {
      return param.mMin + value * (param.mMax - param.mMin);
    }
    
    virtual double ValueToNormalized(double value, const IParam& param) const
    {
      return (value - param.mMin) / (param.mMax - param.mMin);
    }
  };
  
  // Non-linear shape structs
  struct ShapePowCurve : public IParam::Shape
  {
    ShapePowCurve(double shape)
    : mShape(shape)
    {
    }
    
    IParam::EDisplayType GetDisplayType() const override
    {
      if (mShape > 2.5)
        return IParam::kDisplayCubeRoot;
      if (mShape > 1.5)
        return IParam::kDisplaySquareRoot;
      if (mShape < (2.0 / 5.0))
        return IParam::kDisplayCubed;
      if (mShape < (2.0 / 3.0))
        return IParam::kDisplaySquared;
      
      return IParam::kDisplayLinear;
    }
    
    double NormalizedToValue(double value, const IParam& param) const override
    {
      return param.GetMin() + std::pow(value, mShape) * (param.GetMax() - param.GetMin());
    }
    
    virtual double ValueToNormalized(double value, const IParam& param) const override
    {
      return std::pow((value - param.GetMin()) / (param.GetMax() - param.GetMin()), 1.0 / mShape);
    }
    
    double mShape;
  };
  
  struct ShapeExp : public IParam::Shape
  {
    void Init(const IParam& param) override
    {
      mAdd = std::log(param.GetMin());
      mMul = std::log(param.GetMax() / param.GetMin());
    }
    
    IParam::EDisplayType GetDisplayType() const override
    {
      return IParam::kDisplayLog;
    }
    
    double NormalizedToValue(double value, const IParam& param) const override
    {
      return std::exp(mAdd + value * mMul);
    }
    
    virtual double ValueToNormalized(double value, const IParam& param) const override
    {
      return (std::log(value) - mAdd) / mMul;
    }
    
    double mMul = 1.0;
    double mAdd = 1.0;
  };
  
#pragma mark -

  IParam();
  
  ~IParam()
  {
    delete mShape;
  };

  EParamType Type() const { return mType; }

  void InitBool(const char* name, bool defaultValue, const char* label = "", const char* group = "", const char* offText = "off", const char* onText = "on"); // // LABEL not used here TODO: so why have it?
  void InitEnum(const char* name, int defaultValue, int nEnums, const char* label = "", const char* group = "", const char* listItems = 0, ...); // LABEL not used here TODO: so why have it?
  void InitInt(const char* name, int defaultValue, int minVal, int maxVal, const char* label = "", const char* group = "");
  void InitDouble(const char* name, double defaultVal, double minVal, double maxVal, double step, const char* label = "", const char* group = "", Shape* shape = nullptr, EParamUnit unit = kUnitCustom, DisplayFunc displayFunc = nullptr);

  void InitSeconds(const char* name, double defaultVal = 1., double minVal = 0., double maxVal = 10., double step = 0.1, const char* group = "");
  void InitFrequency(const char* name, double defaultVal = 1000., double minVal = 0.1, double maxVal = 10000., double step = 0.1, const char* group = "");
  void InitPitch(const char* name, int defaultVal = 60, int minVal = 0, int maxVal = 128, const char* group = "");
  void InitGain(const char* name, double defaultVal = 0., double minVal = -70., double maxVal = 24., double step = 0.5, const char* group = "");
  void InitPercentage(const char* name, double defaultVal = 0., double minVal = 0., double maxVal = 100., const char* group = "");

  /** Sets the parameter value
   * @param value Value to be set. Will be clamped between \c mMin and \c mMax */
  void Set(double value) { mValue = BOUNDED(value, mMin, mMax); }
  void SetToDefault() { mValue = mDefault; }
  void SetDisplayText(double value, const char* str);
  void SetCanAutomate(bool canAutomate) { mCanAutomate = canAutomate; }
  // The higher the shape, the more resolution around host value zero.
  void SetIsMeta(bool meta) { mIsMeta = meta; }

  // Call this if your param is (x, y) but you want to always display (-x, -y)
  void NegateDisplay() { mNegateDisplay = true; }
  bool GetDisplayIsNegated() const { return mNegateDisplay; }

  //call this to make sure the param display text allways has a sign
  void SignDisplay() { mSignDisplay = true; }

  /** Gets a readable value of the parameter
   * @return Current value of the parameter */
  double Value() const { return mValue; }
  /** Returns the parameter's value as a boolean
   * @return \c true if value >= 0.5, else otherwise */
  bool Bool() const { return (mValue >= 0.5); }
  /** Returns the parameter's value as an integer
   * @return Current value of the parameter */
  int Int() const { return int(mValue); }
  double DBToAmp() const;
  double Clamp(double value) const { return BOUNDED(value, mMin, mMax); }
  
  void SetNormalized(double normalizedValue);
  double GetNormalized() const;

  inline double ToNormalized(double nonNormalizedValue) const
  {
    return BOUNDED(mShape->ValueToNormalized(nonNormalizedValue, *this), 0, 1);
  }
  
  inline double FromNormalized(double normalizedValue) const
  {
    return BOUNDED(mShape->NormalizedToValue(normalizedValue, *this), mMin, mMax);
  }
  
  void GetDisplayForHost(WDL_String& display, bool withDisplayText = true) const { GetDisplayForHost(mValue, false, display, withDisplayText); }
  void GetDisplayForHost(double value, bool normalized, WDL_String& display, bool withDisplayText = true) const;
  
  double StringToValue(const char* str) const;
  
  const char* GetNameForHost() const;
  const char* GetLabelForHost() const;
  const char* GetParamGroupForHost() const;
  
  int NDisplayTexts() const;
  const char* GetDisplayText(int value) const;
  const char* GetDisplayTextAtIdx(int idx, double* pValue = nullptr) const;
  bool MapDisplayText(const char* str, double* pValue) const;  // Reverse map back to value.
  
  double GetDefault() const { return mDefault; }
  double GetMin() const { return mMin; }
  double GetMax() const { return mMax; }
  void GetBounds(double& lo, double& hi) const;
  double GetRange() const { return mMax - mMin; }
  double GetStep() const { return mStep; }
  int GetDisplayPrecision() const {return mDisplayPrecision;}
  bool GetCanAutomate() const { return mCanAutomate; }
  MetaData GetMetaData() const;
  
  void GetJSON(WDL_String& json, int idx) const;
private:
  struct DisplayText
  {
    double mValue;
    char mText[MAX_PARAM_DISPLAY_LEN];
  };
  
  EParamType mType = kTypeNone;
  EParamUnit mUnit = kUnitCustom;
  double mValue = 0.0;
  double mMin = 0.0;
  double mMax = 1.0;
  double mStep = 1.0;
  double mDefault = 0.0;
  int mDisplayPrecision = 0;
  bool mNegateDisplay = false;
  bool mSignDisplay = false;
  bool mCanAutomate = true;
  bool mIsMeta = false;
  char mName[MAX_PARAM_NAME_LEN];
  char mLabel[MAX_PARAM_LABEL_LEN];
  char mParamGroup[MAX_PARAM_GROUP_LEN];
  Shape* mShape = nullptr;
  DisplayFunc mDisplayFunction = nullptr;
  
  WDL_TypedBuf<DisplayText> mDisplayTexts;
} WDL_FIXALIGN;

