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

  enum EFlags
  {
    kFlagCannotAutomate   = 0x1,
    kFlagStepped          = 0x2,
    kFlagNegateDisplay    = 0x4,
    kFlagSignDisplay      = 0x8,
    kFlagMeta             = 0x10
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

  void InitBool(const char* name, bool defaultValue, int flags = 0, const char* label = "", const char* group = "", const char* offText = "off", const char* onText = "on"); // // LABEL not used here TODO: so why have it?
  void InitEnum(const char* name, int defaultValue, int nEnums, int flags = 0, const char* label = "", const char* group = "", const char* listItems = 0, ...); // LABEL not used here TODO: so why have it?
  void InitInt(const char* name, int defaultValue, int minVal, int maxVal, int flags = 0, const char* label = "", const char* group = "");
  void InitDouble(const char* name, double defaultVal, double minVal, double maxVal, double step, int flags = 0, const char* label = "", const char* group = "", Shape* shape = nullptr, EParamUnit unit = kUnitCustom, IDisplayFunc displayFunc);

  void InitSeconds(const char* name, double defaultVal = 1., double minVal = 0., double maxVal = 10., double step = 0.1, int flags = 0, const char* group = "");
  void InitFrequency(const char* name, double defaultVal = 1000., double minVal = 0.1, double maxVal = 10000., double step = 0.1, int flags = 0, const char* group = "");
  void InitPitch(const char* name, int defaultVal = 60, int minVal = 0, int maxVal = 128, int flags = 0, const char* group = "");
  void InitGain(const char* name, double defaultVal = 0., double minVal = -70., double maxVal = 24., double step = 0.5, int flags = 0, const char* group = "");
  void InitPercentage(const char* name, double defaultVal = 0., double minVal = 0., double maxVal = 100., int flags = 0, const char* group = "");

  double StringToValue(const char* str) const;

  inline double Constrain(double value) const { return Clip((mIsStepped ? round(value / mStep) * mStep : value), mMin, mMax); }

  inline double ToNormalized(double nonNormalizedValue) const
  {
    return Clip(mShape->ValueToNormalized(Constrain(nonNormalizedValue), *this), 0., 1.);
  }

  inline double FromNormalized(double normalizedValue) const
  {
    return Constrain(mShape->NormalizedToValue(normalizedValue, *this));
  }

  /** Sets the parameter value
   * @param value Value to be set. Will be stepped and clamped between \c mMin and \c mMax */
  void Set(double value) { mValue = Constrain(value); }
  void SetNormalized(double normalizedValue) { Set(FromNormalized(normalizedValue)); }
  void SetString(const char* str) { mValue = StringToValue(str); }
  void SetToDefault() { mValue = mDefault; }

  void SetDisplayText(double value, const char* str);

  // Accessors / converters.
  // These all return the readable value, not the VST (0,1).
  /** Gets a readable value of the parameter
   * @return Current value of the parameter */
  double Value() const { return mValue; }
  /** Returns the parameter's value as a boolean
   * @return \c true if value >= 0.5, else otherwise */
  bool Bool() const { return (mValue >= 0.5); }
  /** Returns the parameter's value as an integer
   * @return Current value of the parameter */
  int Int() const { return int(mValue); }
  double DBToAmp() const { return ::DBToAmp(mValue); }
  double GetNormalized() const { return ToNormalized(mValue); }

  void GetDisplayForHost(WDL_String& display, bool withDisplayText = true) const { GetDisplayForHost(mValue, false, display, withDisplayText); }
  void GetDisplayForHost(double value, bool normalized, WDL_String& display, bool withDisplayText = true) const;

  double StringToValue(const char* str) const;

  const char* GetNameForHost() const;
  const char* GetLabelForHost() const;
  const char* GetParamGroupForHost() const;
  const char* GetCustomUnit() const { return mUnit == kUnitCustom ? mLabel : nullptr; }
  
  int NDisplayTexts() const;
  const char* GetDisplayText(int value) const;
  const char* GetDisplayTextAtIdx(int idx, double* pValue = nullptr) const;
  bool MapDisplayText(const char* str, double* pValue) const;  // Reverse map back to value.
  
  EParamType Type() const { return mType; }
  EParamUnit Unit() const { return mUnit; }
  EDisplayType DisplayType() const { return mShape->GetDisplayType(); }
  
  double GetDefault() const { return mDefault; }
  double GetMin() const { return mMin; }
  double GetMax() const { return mMax; }
  void GetBounds(double& lo, double& hi) const;
  double GetRange() const { return mMax - mMin; }
  double GetStep() const { return mStep; }
  int GetDisplayPrecision() const {return mDisplayPrecision;}
  
  bool GetCanAutomate() const { return !(mFlags & kFlagCannotAutomate); }
  bool GetStepped() const { return mFlags & kFlagStepped; }
  bool GetNegateDisplay() const { return mFlags & kFlagNegateDisplay; }
  bool GetSignDisplay() const { return mFlags & kFlagSignDisplay; }
  bool GetMeta() const { return mFlags & kFlagMeta; }
  
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
  int mFlags = 0;

  char mName[MAX_PARAM_NAME_LEN];
  char mLabel[MAX_PARAM_LABEL_LEN];
  char mParamGroup[MAX_PARAM_GROUP_LEN];
  Shape* mShape = nullptr;
  DisplayFunc mDisplayFunction = nullptr;

  WDL_TypedBuf<DisplayText> mDisplayTexts;
} WDL_FIXALIGN;
