/*
 ==============================================================================
 
 This file is part of the iPlug 2 library
 
 Oli Larkin et al. 2018 - https://www.olilarkin.co.uk
 
 iPlug 2 is an open source library subject to commercial or open-source
 licensing.
 
 The code included in this file is provided under the terms of the WDL license
 - https://www.cockos.com/wdl/
 
 ==============================================================================
 */

#pragma once

#include <atomic>
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
    kFlagsNone            = 0,
    kFlagCannotAutomate   = 0x1,
    kFlagStepped          = 0x2,
    kFlagNegateDisplay    = 0x4,
    kFlagSignDisplay      = 0x8,
    kFlagMeta             = 0x10,
  };
  
  typedef std::function<void(double, WDL_String&)> DisplayFunc;

#pragma mark - Shape

  struct Shape
  {
    virtual ~Shape() {}
    virtual Shape* Clone() const = 0;
    virtual void Init(const IParam& param) {}
    virtual EDisplayType GetDisplayType() const = 0;
    virtual double NormalizedToValue(double value, const IParam& param) const = 0;
    virtual double ValueToNormalized(double value, const IParam& param) const = 0;
  };

  // Linear shape structs
  struct ShapeLinear : public Shape
  {
      Shape* Clone() const override { return new ShapeLinear(); };
      IParam::EDisplayType GetDisplayType() const override { return kDisplayLinear; }
      double NormalizedToValue(double value, const IParam& param) const override;
      double ValueToNormalized(double value, const IParam& param) const override;
    
      double mShape;
  };
  
  // Non-linear shape structs
  struct ShapePowCurve : public Shape
  {
    ShapePowCurve(double shape);
    Shape* Clone() const override { return new ShapePowCurve(mShape); };
    IParam::EDisplayType GetDisplayType() const override;
    double NormalizedToValue(double value, const IParam& param) const override;
    double ValueToNormalized(double value, const IParam& param) const override;
    
    double mShape;
  };
  
  struct ShapeExp : public Shape
  {
    void Init(const IParam& param) override;
    Shape* Clone() const override { return new ShapeExp(); };
    IParam::EDisplayType GetDisplayType() const override { return kDisplayLog; }
    double NormalizedToValue(double value, const IParam& param) const override;
    double ValueToNormalized(double value, const IParam& param) const override;
    
    double mMul = 1.0;
    double mAdd = 1.0;
  };

#pragma mark -

  IParam();

  ~IParam()
  {
    delete mShape;
  };

  void InitBool(const char* name, bool defaultValue, const char* label = "", int flags = 0, const char* group = "", const char* offText = "off", const char* onText = "on"); // // LABEL not used here TODO: so why have it?
  void InitEnum(const char* name, int defaultValue, int nEnums, const char* label = "", int flags = 0, const char* group = "", const char* listItems = 0, ...); // LABEL not used here TODO: so why have it?
  void InitInt(const char* name, int defaultValue, int minVal, int maxVal, const char* label = "", int flags = 0, const char* group = "");
  void InitDouble(const char* name, double defaultVal, double minVal, double maxVal, double step, const char* label = "", int flags = 0, const char* group = "", Shape* shape = nullptr, EParamUnit unit = kUnitCustom, DisplayFunc displayFunc = nullptr);

  void InitSeconds(const char* name, double defaultVal = 1., double minVal = 0., double maxVal = 10., double step = 0.1, int flags = 0, const char* group = "");
  void InitFrequency(const char* name, double defaultVal = 1000., double minVal = 0.1, double maxVal = 10000., double step = 0.1, int flags = 0, const char* group = "");
  void InitPitch(const char* name, int defaultVal = 60, int minVal = 0, int maxVal = 128, int flags = 0, const char* group = "");
  void InitGain(const char* name, double defaultVal = 0., double minVal = -70., double maxVal = 24., double step = 0.5, int flags = 0, const char* group = "");
  void InitPercentage(const char* name, double defaultVal = 0., double minVal = 0., double maxVal = 100., int flags = 0, const char* group = "");
  void InitAngleDegrees(const char* name, double defaultVal = 0., double minVal = 0., double maxVal = 360., int flags = 0, const char* group = "");

  void Init(const IParam& p, const char* searchStr = "", const char* replaceStr = "", const char* newGroup = "");
  
  double StringToValue(const char* str) const;

  inline double Constrain(double value) const { return Clip((mFlags & kFlagStepped ? round(value / mStep) * mStep : value), mMin, mMax); }

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
  void Set(double value) { mValue.store(Constrain(value)); }
  void SetNormalized(double normalizedValue) { Set(FromNormalized(normalizedValue)); }
  void SetString(const char* str) { mValue.store(StringToValue(str)); }
  void SetToDefault() { mValue.store(mDefault); }
  void SetDefault(double value) { mDefault = value; SetToDefault(); }

  void SetDisplayText(double value, const char* str);

  // Accessors / converters.
  // These all return the readable value, not the VST (0,1).
  /** Gets a readable value of the parameter
   * @return Current value of the parameter */
  double Value() const { return mValue.load(); }
  /** Returns the parameter's value as a boolean
   * @return \c true if value >= 0.5, else otherwise */
  bool Bool() const { return (mValue.load() >= 0.5); }
  /** Returns the parameter's value as an integer
   * @return Current value of the parameter */
  int Int() const { return static_cast<int>(mValue.load()); }
  double DBToAmp() const { return ::DBToAmp(mValue.load()); }
  double GetNormalized() const { return ToNormalized(mValue.load()); }

  void GetDisplayForHost(WDL_String& display, bool withDisplayText = true) const { GetDisplayForHost(mValue.load(), false, display, withDisplayText); }
  void GetDisplayForHost(double value, bool normalized, WDL_String& display, bool withDisplayText = true) const;

  const char* GetNameForHost() const;
  const char* GetLabelForHost() const;
  const char* GetGroupForHost() const;
  const char* GetCustomUnit() const { return mUnit == kUnitCustom ? mLabel : nullptr; }
  
  int NDisplayTexts() const;
  const char* GetDisplayText(double value) const;
  const char* GetDisplayTextAtIdx(int idx, double* pValue = nullptr) const;
  bool MapDisplayText(const char* str, double* pValue) const;  // Reverse map back to value.
  
  EParamType Type() const { return mType; }
  EParamUnit Unit() const { return mUnit; }
  EDisplayType DisplayType() const { return mShape->GetDisplayType(); }
  
  double GetDefault(bool normalized = false) const { return normalized ? ToNormalized(GetDefault()) : mDefault; }
  
  double GetMin() const { return mMin; }
  double GetMax() const { return mMax; }
  void GetBounds(double& lo, double& hi) const;
  double GetRange() const { return mMax - mMin; }
  double GetStep() const { return mStep; }
  int GetDisplayPrecision() const {return mDisplayPrecision;}
  
  int GetFlags() const { return mFlags; }
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
  std::atomic<double> mValue{0.0};
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
