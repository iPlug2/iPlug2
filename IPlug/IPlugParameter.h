/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc IParam
 */

#include <atomic>
#include <cstring>
#include <functional>
#include <memory>

#include "wdlstring.h"

#include "IPlugUtilities.h"

BEGIN_IPLUG_NAMESPACE

/** IPlug's parameter class */
class IParam
{
public:

  /** Defines types or parameter. */
  enum EParamType { kTypeNone, kTypeBool, kTypeInt, kTypeEnum, kTypeDouble };

  /** Used by AudioUnit plugins to determine the appearance of parameters, based on the kind of data they represent */
  enum EParamUnit { kUnitPercentage, kUnitSeconds, kUnitMilliseconds, kUnitSamples, kUnitDB, kUnitLinearGain, kUnitPan, kUnitPhase, kUnitDegrees, kUnitMeters, kUnitRate, kUnitRatio, kUnitFrequency, kUnitOctaves, kUnitCents, kUnitAbsCents, kUnitSemitones, kUnitMIDINote, kUnitMIDICtrlNum, kUnitBPM, kUnitBeats, kUnitCustom };

  /** Used by AudioUnit plugins to determine the mapping of parameters */
  enum EDisplayType { kDisplayLinear, kDisplayLog, kDisplayExp, kDisplaySquared, kDisplaySquareRoot, kDisplayCubed, kDisplayCubeRoot };

  /** Flags to determine characteristics of the parameter */
  enum EFlags
  {
    kFlagsNone            = 0,
    kFlagCannotAutomate   = 0x1, /** Indicates that the parameter is not automatable */
    kFlagStepped          = 0x2, /** Indicates that the parameter ???  */
    kFlagNegateDisplay    = 0x4, /** Indicates that the parameter should be displayed as a negative value */
    kFlagSignDisplay      = 0x8, /** Indicates that the parameter should be displayed as a signed value */
    kFlagMeta             = 0x10, /** Indicates that the parameter may influence the state of other parameters */
  };
  
  using DisplayFunc = std::function<void(double, WDL_String&)>;

#pragma mark - Shape

  /** Base struct for parameter shaping */
  struct Shape
  {
    virtual ~Shape() {}

    /** /todo 
     * @return Shape* /todo */
    virtual Shape* Clone() const = 0;

    /** /todo 
     * @param param /todo */
    virtual void Init(const IParam& param) {}

    /** /todo 
     * @return EDisplayType /todo */
    virtual EDisplayType GetDisplayType() const = 0;

    /** /todo 
     * @param value /todo
     * @param param /todo
     * @return double /todo */
    virtual double NormalizedToValue(double value, const IParam& param) const = 0;

    /** /todo 
     * @param value /todo
     * @param param /todo
     * @return double /todo */
    virtual double ValueToNormalized(double value, const IParam& param) const = 0;
  };

  /** Linear parameter shaping */
  struct ShapeLinear : public Shape
  {
    Shape* Clone() const override { return new ShapeLinear(*this); };
    IParam::EDisplayType GetDisplayType() const override { return kDisplayLinear; }
    double NormalizedToValue(double value, const IParam& param) const override;
    double ValueToNormalized(double value, const IParam& param) const override;
  
    double mShape;
  };
  
  /** PowCurve parameter shaping */
  struct ShapePowCurve : public Shape
  {
    ShapePowCurve(double shape);
    Shape* Clone() const override { return new ShapePowCurve(*this); };
    IParam::EDisplayType GetDisplayType() const override;
    double NormalizedToValue(double value, const IParam& param) const override;
    double ValueToNormalized(double value, const IParam& param) const override;
    
    double mShape;
  };
  
  /** Exponential parameter shaping */
  struct ShapeExp : public Shape
  {
    void Init(const IParam& param) override;
    Shape* Clone() const override { return new ShapeExp(*this); };
    IParam::EDisplayType GetDisplayType() const override { return kDisplayLog; }
    double NormalizedToValue(double value, const IParam& param) const override;
    double ValueToNormalized(double value, const IParam& param) const override;
    
    double mMul = 1.0;
    double mAdd = 1.0;
  };

#pragma mark -

  IParam();

  IParam(const IParam&) = delete;
  IParam& operator=(const IParam&) = delete;

  /** Initialize the parameter as boolean
   * @param name The parameter's name
   * @param defaultValue The default value of the parameter
   * @param label The parameter's label
   * @param flags The parameter's flags \see IParam::EFlags
   * @param group The parameter's group
   * @param offText The display text when the parameter value == 0.
   * @param onText The display text when the parameter value == 1. */
  void InitBool(const char* name, bool defaultValue, const char* label = "", int flags = 0, const char* group = "", const char* offText = "off", const char* onText = "on"); // TODO: LABEL not used here TODO: so why have it?
  
  /** /todo 
   * @param name /todo
   * @param defaultValue /todo
   * @param nEnums /todo
   * @param label /todo
   * @param flags /todo
   * @param group /todo
   * @param listItems /todo
   * @param ... /todo */
  void InitEnum(const char* name, int defaultValue, int nEnums, const char* label = "", int flags = 0, const char* group = "", const char* listItems = 0, ...); // TODO: LABEL not used here TODO: so why have it?

  /** /todo
   * @param name /todo
   * @param defaultValue /todo
   * @param listItems /todo
   * @param label /todo
   * @param flags /todo
   * @param group /todo*/
  void InitEnum(const char* name, int defaultValue, const std::initializer_list<const char*>& listItems, int flags = 0, const char* group = "");

  /** /todo 
   * @param name /todo
   * @param defaultValue /todo
   * @param minVal /todo
   * @param maxVal /todo
   * @param label /todo
   * @param flags /todo
   * @param group /todo */
  void InitInt(const char* name, int defaultValue, int minVal, int maxVal, const char* label = "", int flags = 0, const char* group = "");
  
  /** /todo 
   * @param name /todo
   * @param defaultVal /todo
   * @param minVal /todo
   * @param maxVal /todo
   * @param step /todo
   * @param label /todo
   * @param flags /todo
   * @param group /todo
   * @param shape /todo
   * @param unit /todo
   * @param displayFunc /todo */
  void InitDouble(const char* name, double defaultVal, double minVal, double maxVal, double step, const char* label = "", int flags = 0, const char* group = "", const Shape& shape = ShapeLinear(), EParamUnit unit = kUnitCustom, DisplayFunc displayFunc = nullptr);

  /** /todo 
   * @param name /todo
   * @param defaultVal /todo
   * @param minVal /todo
   * @param maxVal /todo
   * @param step /todo
   * @param flags /todo
   * @param group /todo */
  void InitSeconds(const char* name, double defaultVal = 1., double minVal = 0., double maxVal = 10., double step = 0.1, int flags = 0, const char* group = "");
  
  /** /todo
   * @param name /todo
   * @param defaultVal /todo
   * @param minVal /todo
   * @param maxVal /todo
   * @param step /todo
   * @param flags /todo
   * @param group /todo */
  void InitMilliseconds(const char* name, double defaultVal = 1., double minVal = 0., double maxVal = 100., int flags = 0, const char* group = "");
  /** /todo 
   * @param name /todo
   * @param defaultVal /todo
   * @param minVal /todo
   * @param maxVal /todo
   * @param step /todo
   * @param flags /todo
   * @param group /todo */
  void InitFrequency(const char* name, double defaultVal = 1000., double minVal = 0.1, double maxVal = 10000., double step = 0.1, int flags = 0, const char* group = "");
  
  /** /todo 
   * @param name /todo
   * @param defaultVal /todo
   * @param minVal /todo
   * @param maxVal /todo
   * @param flags /todo
   * @param group /todo */
  void InitPitch(const char* name, int defaultVal = 60, int minVal = 0, int maxVal = 128, int flags = 0, const char* group = "", bool middleCisC4 = false);
  
  /** /todo 
   * @param name /todo
   * @param defaultVal /todo
   * @param minVal /todo
   * @param maxVal /todo
   * @param step /todo
   * @param flags /todo
   * @param group /todo */
  void InitGain(const char* name, double defaultVal = 0., double minVal = -70., double maxVal = 24., double step = 0.5, int flags = 0, const char* group = "");
  
  /** /todo 
   * @param name /todo
   * @param defaultVal /todo
   * @param minVal /todo
   * @param maxVal /todo
   * @param flags /todo
   * @param group /todo */
  void InitPercentage(const char* name, double defaultVal = 0., double minVal = 0., double maxVal = 100., int flags = 0, const char* group = "");

  /** /todo 
   * @param name /todo
   * @param defaultVal /todo
   * @param minVal /todo
   * @param maxVal /todo
   * @param flags /todo
   * @param group /todo */
  void InitAngleDegrees(const char* name, double defaultVal = 0., double minVal = 0., double maxVal = 360., int flags = 0, const char* group = "");

  /** /todo 
   * @param p /todo
   * @param searchStr /todo
   * @param replaceStr /todo
   * @param newGroup /todo */
  void Init(const IParam& p, const char* searchStr = "", const char* replaceStr = "", const char* newGroup = "");
  
  /** /todo 
   * @param str /todo
   * @return double /todo */
  double StringToValue(const char* str) const;

  /** Constrains the input value between \c mMin and \c mMax
   * @param value The input value to constrain
   * @return double The resulting constrained value */
  inline double Constrain(double value) const { return Clip((mFlags & kFlagStepped ? round(value / mStep) * mStep : value), mMin, mMax); }

  /** Convert a real value to normalized value for this parameter
   * @param nonNormalizedValue The real input value
   * @return The corresponding normalized value, for this parameter */
  inline double ToNormalized(double nonNormalizedValue) const
  {
    return Clip(mShape->ValueToNormalized(Constrain(nonNormalizedValue), *this), 0., 1.);
  }

  /** Convert a normalized value to real value for this parameter
   * @param normalizedValue The normalized input value in the range 0. to 1.
   * @return The corresponding real value, for this parameter */
  inline double FromNormalized(double normalizedValue) const
  {
    return Constrain(mShape->NormalizedToValue(normalizedValue, *this));
  }

  /** Sets the parameter value
   * @param value Value to be set. Will be stepped and clamped between \c mMin and \c mMax */
  void Set(double value) { mValue.store(Constrain(value)); }

  /** Sets the parameter value from a normalized range (usually coming from the linked IControl)
   * @param normalizedValue The expected normalized value between 0. and 1. */
  void SetNormalized(double normalizedValue) { Set(FromNormalized(normalizedValue)); }

  /** /todo 
   * @param str /todo */
  void SetString(const char* str) { mValue.store(StringToValue(str)); }

  /** /todo  */
  void SetToDefault() { mValue.store(mDefault); }

  /** /todo 
   * @param value /todo */
  void SetDefault(double value) { mDefault = value; SetToDefault(); }

  /** /todo 
   * @param value /todo
   * @param str /todo */
  void SetDisplayText(double value, const char* str);
  
  /** Set the parameters label after creation. WARNING: if this is called after the host has queried plugin parameters, the host may display the label as it was previously
   * @param label /todo */
  void SetLabel(const char* label) { strcpy(mLabel, label); }
  
  /** Set the function to translate display values
   * @param func  /todo */
  void SetDisplayFunc(DisplayFunc func) { mDisplayFunction = func; }

  /** Gets a readable value of the parameter
   * @return Current value of the parameter */
  double Value() const { return mValue.load(); }

  /** Returns the parameter's value as a boolean
   * @return \c true if value >= 0.5, else otherwise */
  bool Bool() const { return (mValue.load() >= 0.5); }

  /** @return Current value of the parameter as an integer */
  int Int() const { return static_cast<int>(mValue.load()); }
  
  /** /todo 
   * @return double /todo */
  double DBToAmp() const { return iplug::DBToAmp(mValue.load()); }

  /** /todo 
   * @return double /todo */
  double GetNormalized() const { return ToNormalized(mValue.load()); }

  /** /todo 
   * @param display /todo
   * @param withDisplayText /todo */
  void GetDisplayForHost(WDL_String& display, bool withDisplayText = true) const { GetDisplayForHost(mValue.load(), false, display, withDisplayText); }

  void GetDisplayForHostWithLabel(WDL_String& display, bool withDisplayText = true) const
  {
    GetDisplayForHost(mValue.load(), false, display, withDisplayText);
    display.Append(" ");
    display.Append(GetLabelForHost());
  }
  
  /** /todo 
   * @param value /todo
   * @param normalized /todo
   * @param display /todo
   * @param withDisplayText /todo */
  void GetDisplayForHost(double value, bool normalized, WDL_String& display, bool withDisplayText = true) const;

  /** /todo 
   * @return const char* /todo */
  const char* GetNameForHost() const;

  /** /todo 
   * @return const char* /todo */
  const char* GetLabelForHost() const;

  /** /todo 
   * @return const char* /todo */
  const char* GetGroupForHost() const;

  /** /todo 
   * @return const char* /todo */
  const char* GetCustomUnit() const { return mUnit == kUnitCustom ? mLabel : nullptr; }
  
  /** /todo 
   * @return int /todo */
  int NDisplayTexts() const;

  /** /todo 
   * @param value /todo
   * @return const char* /todo */
  const char* GetDisplayText(double value) const;

  /** /todo 
   * @param idx /todo
   * @param pValue /todo
   * @return const char* /todo  */
  const char* GetDisplayTextAtIdx(int idx, double* pValue = nullptr) const;

  /** /todo 
   * @param str /todo
   * @param pValue /todo
   * @return true /todo
   * @return false /todo */
  bool MapDisplayText(const char* str, double* pValue) const;  // Reverse map back to value.
  
  /** /todo 
   * @return EParamType /todo */
  EParamType Type() const { return mType; }

  /** /todo 
   * @return EParamUnit /todo */
  EParamUnit Unit() const { return mUnit; }

  /** /todo 
   * @return EDisplayType /todo */
  EDisplayType DisplayType() const { return mShape->GetDisplayType(); }
  
  /** /todo 
   * @param normalized /todo
   * @return double /todo  */
  double GetDefault(bool normalized = false) const { return normalized ? ToNormalized(GetDefault()) : mDefault; }
  
  /**  @return double /todo */
  double GetMin() const { return mMin; }

  /**  @return double /todo */
  double GetMax() const { return mMax; }
  
  /** /todo 
   * @param lo /todo
   * @param hi /todo */
  void GetBounds(double& lo, double& hi) const;

  /** /todo 
   * @return double /todo  */
  double GetRange() const { return mMax - mMin; }

  /** /todo 
   * @return double /todo */
  double GetStep() const { return mStep; }

  /** /todo 
   * @return int /todo */
  int GetDisplayPrecision() const {return mDisplayPrecision;}
  
  /** /todo 
   * @return int /todo */
  int GetFlags() const { return mFlags; }

  /** /todo 
   * @return true /todo  */
  bool GetCanAutomate() const { return !(mFlags & kFlagCannotAutomate); }

  /** /todo 
   * @return true /todo */
  bool GetStepped() const { return mFlags & kFlagStepped; }

  /** /todo 
   * @return false /todo */
  bool GetNegateDisplay() const { return mFlags & kFlagNegateDisplay; }

  /** /todo 
   * @return false /todo */
  bool GetSignDisplay() const { return mFlags & kFlagSignDisplay; }

  /** /todo 
   * @return false /todo */
  bool GetMeta() const { return mFlags & kFlagMeta; }
 
  /** /todo 
   * @param json /todo
   * @param idx /todo */
  void GetJSON(WDL_String& json, int idx) const;

  /** /todo */
  void PrintDetails() const;
private:
  /** /todo */
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
  
  std::unique_ptr<Shape> mShape;
  DisplayFunc mDisplayFunction = nullptr;

  WDL_TypedBuf<DisplayText> mDisplayTexts;
} WDL_FIXALIGN;

END_IPLUG_NAMESPACE
