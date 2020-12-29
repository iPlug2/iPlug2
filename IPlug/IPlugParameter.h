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
    /** No flags */
    kFlagsNone            = 0,
     /** Indicates that the parameter is not automatable */
    kFlagCannotAutomate   = 0x1,
    /** Indicates that the parameter is stepped  */
    kFlagStepped          = 0x2,
    /** Indicates that the parameter should be displayed as a negative value */
    kFlagNegateDisplay    = 0x4,
    /** Indicates that the parameter should be displayed as a signed value */
    kFlagSignDisplay      = 0x8,
    /** Indicates that the parameter may influence the state of other parameters */
    kFlagMeta             = 0x10,
  };
  
  /** DisplayFunc allows custom parameter display functions, defined by a lambda matching this signature */
  using DisplayFunc = std::function<void(double, WDL_String&)>;

#pragma mark - Shape

  /** Base struct for parameter shaping */
  struct Shape
  {
    virtual ~Shape() {}

    /** @return A new instance of this Shape struct */
    virtual Shape* Clone() const = 0;

    /** Initializes the shape instance
     * @param param The parent parameter */
    virtual void Init(const IParam& param) {}

    /** @return EDisplayType, used by AudioUnit plugins to determine the mapping of parameters */
    virtual EDisplayType GetDisplayType() const = 0;

    /** Returns the real value from a normalized input, based on an IParam's settings
     * @param value The normalized value as a \c double to be converted
     * @param param The IParam to do the calculation against
     * @return double The real value */
    virtual double NormalizedToValue(double value, const IParam& param) const = 0;

    /** Returns the normalized value from a real value, based on an IParam's settings
     * @param value The real value as a \c double to be converted
     * @param param The IParam to do the calculation against
     * @return double The normalized value */
    virtual double ValueToNormalized(double value, const IParam& param) const = 0;
  };

  /** Linear parameter shaping */
  struct ShapeLinear : public Shape
  {
    Shape* Clone() const override { return new ShapeLinear(*this); }
    IParam::EDisplayType GetDisplayType() const override { return kDisplayLinear; }
    double NormalizedToValue(double value, const IParam& param) const override;
    double ValueToNormalized(double value, const IParam& param) const override;
  
    double mShape;
  };
  
  /** PowCurve parameter shaping */
  struct ShapePowCurve : public Shape
  {
    ShapePowCurve(double shape);
    Shape* Clone() const override { return new ShapePowCurve(*this); }
    IParam::EDisplayType GetDisplayType() const override;
    double NormalizedToValue(double value, const IParam& param) const override;
    double ValueToNormalized(double value, const IParam& param) const override;
    
    double mShape;
  };
  
  /** Exponential parameter shaping */
  struct ShapeExp : public Shape
  {
    void Init(const IParam& param) override;
    Shape* Clone() const override { return new ShapeExp(*this); }
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
   * @param label The parameter's unit suffix. Has no effect for this type of parameter
   * @param flags The parameter's flags \see IParam::EFlags
   * @param group The parameter's group
   * @param offText The display text when the parameter value == 0.
   * @param onText The display text when the parameter value == 1. */
  void InitBool(const char* name, bool defaultValue, const char* label = "", int flags = 0, const char* group = "", const char* offText = "off", const char* onText = "on");
  
  /** Initialize the parameter as an enumerated list
   * @param name The parameter's name
   * @param defaultValue The default value of the parameter
   * @param nEnums The number of elements in the enumerated list
   * @param label The parameter's unit suffix. Has no effect for this type of parameter
   * @param flags The parameter's flags \see IParam::EFlags
   * @param group The parameter's group
   * @param listItems VARARG list of enum items, the length of which must match nEnums */
  void InitEnum(const char* name, int defaultValue, int nEnums, const char* label = "", int flags = 0, const char* group = "", const char* listItems = 0, ...);

  /** Initialize the parameter as enum
   * @param name The parameter's name
   * @param defaultValue The default value of the parameter
   * @param listItems An initializer list of CStrings for the list items
   * @param flags The parameter's flags \see IParam::EFlags
   * @param group The parameter's group */
  void InitEnum(const char* name, int defaultValue, const std::initializer_list<const char*>& listItems, int flags = 0, const char* group = "");

  /** Initialize the parameter as integer
   * @param name The parameter's name
   * @param defaultValue The default value of the parameter
   * @param minVal The minimum value of the parameter
   * @param maxVal The maximum value of the parameter
   * @param label The parameter's unit suffix (eg. dB, %)
   * @param flags The parameter's flags \see IParam::EFlags
   * @param group The parameter's group */
  void InitInt(const char* name, int defaultValue, int minVal, int maxVal, const char* label = "", int flags = 0, const char* group = "");
  
  /** Initialize the parameter as double
   * @param name The parameter's name
   * @param defaultVal The default value of the parameter
   * @param minVal The minimum value of the parameter
   * @param maxVal The maximum value of the parameter
   * @param step The step size of the parameter
   * @param label The parameter's unit suffix (eg. dB, %)
   * @param flags The parameter's flags \see IParam::EFlags
   * @param group The parameter's group
   * @param shape A Parameter::Shape struct that determines the skewing of the parameters values across its range
   * @param unit Used by AudioUnit plugins to determine the appearance of parameters, based on the kind of data they represent
   * @param displayFunc Custom display function, conforming to DisplayFunc */
  void InitDouble(const char* name, double defaultVal, double minVal, double maxVal, double step, const char* label = "", int flags = 0, const char* group = "", const Shape& shape = ShapeLinear(), EParamUnit unit = kUnitCustom, DisplayFunc displayFunc = nullptr);

  /** Initialize the parameter as seconds
   * @param name The parameter's name
   * @param defaultVal The default value of the parameter
   * @param minVal The minimum value of the parameter
   * @param maxVal The maximum value of the parameter
   * @param step The step size of the parameter
   * @param flags The parameter's flags \see IParam::EFlags
   * @param group The parameter's group */
  void InitSeconds(const char* name, double defaultVal = 1., double minVal = 0., double maxVal = 10., double step = 0.1, int flags = 0, const char* group = "");
  
  /** Initialize the parameter as milliseconds
   * @param name The parameter's name
   * @param defaultVal The default value of the parameter
   * @param minVal The minimum value of the parameter
   * @param maxVal The maximum value of the parameter
   * @param step The step size of the parameter
   * @param flags The parameter's flags \see IParam::EFlags
   * @param group The parameter's group */
  void InitMilliseconds(const char* name, double defaultVal = 1., double minVal = 0., double maxVal = 100., int flags = 0, const char* group = "");

  /** Initialize the parameter as frequency
   * @param name The parameter's name
   * @param defaultVal The default value of the parameter
   * @param minVal The minimum value of the parameter
   * @param maxVal The maximum value of the parameter
   * @param step The step size of the parameter
   * @param flags The parameter's flags \see IParam::EFlags
   * @param group The parameter's group */
  void InitFrequency(const char* name, double defaultVal = 1000., double minVal = 0.1, double maxVal = 10000., double step = 0.1, int flags = 0, const char* group = "");
  
  /** Initialize the parameter as pitch
   * @param name The parameter's name
   * @param defaultVal The default value of the parameter
   * @param minVal The minimum value of the parameter
   * @param maxVal The maximum value of the parameter
   * @param flags The parameter's flags \see IParam::EFlags
   * @param group The parameter's group */
  void InitPitch(const char* name, int defaultVal = 60, int minVal = 0, int maxVal = 128, int flags = 0, const char* group = "", bool middleCisC4 = false);
  
  /** Initialize the parameter as gain (units in decibels)
   * @param name The parameter's name
   * @param defaultVal The default value of the parameter
   * @param minVal The minimum value of the parameter
   * @param maxVal The maximum value of the parameter
   * @param step The step size of the parameter
   * @param flags The parameter's flags \see IParam::EFlags
   * @param group The parameter's group */
  void InitGain(const char* name, double defaultVal = 0., double minVal = -70., double maxVal = 24., double step = 0.5, int flags = 0, const char* group = "");
  
  /** Initialize the parameter as percentage
   * @param name The parameter's name
   * @param defaultVal The default value of the parameter
   * @param minVal The minimum value of the parameter
   * @param maxVal The maximum value of the parameter
   * @param flags The parameter's flags \see IParam::EFlags
   * @param group The parameter's group */
  void InitPercentage(const char* name, double defaultVal = 0., double minVal = 0., double maxVal = 100., int flags = 0, const char* group = "");

  /** Initialize the parameter as angle in degrees
   * @param name The parameter's name
   * @param defaultVal The default value of the parameter
   * @param minVal The minimum value of the parameter
   * @param maxVal The maximum value of the parameter
   * @param flags The parameter's flags \see IParam::EFlags
   * @param group The parameter's group */
  void InitAngleDegrees(const char* name, double defaultVal = 0., double minVal = 0., double maxVal = 360., int flags = 0, const char* group = "");

  /** Initialize the parameter based on another parameter, replacing a CString in the name
   * @param p The existing parameter
   * @param searchStr Search string for modifying the parameter name
   * @param replaceStr Replace string for modifying the parameter name
   * @param newGroup Group for the new parameter */
  void Init(const IParam& p, const char* searchStr = "", const char* replaceStr = "", const char* newGroup = "");
  
  /** Convert a textual representation of the parameter value to a double (real value)
   * @param str CString textual representation of the parameter value 
   * @return double The real value */
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

  /** Set the parameter value using a textual representation
   * @param str The textual representations as a CString */
  void SetString(const char* str) { mValue.store(StringToValue(str)); }

  /** Replaces the parameter's current value with the default one  */
  void SetToDefault() { mValue.store(mDefault); }

  /** Set the parameter's default value, and set the parameter to that default
   * @param value The new default value */
  void SetDefault(double value) { mDefault = value; SetToDefault(); }

  /** Set some text to display for a particular value, e.g. -70dB could display "-inf"
   * @param value The value for which to display the text
   * @param str CString text to display at value */
  void SetDisplayText(double value, const char* str);

  /** Set the parameters display precision
 * @param precision The display precision in digits*/
  void SetDisplayPrecision(int precision);

  /** Set the parameters label after creation. WARNING: if this is called after the host has queried plugin parameters, the host may display the label as it was previously
   * @param label CString for the label */
  void SetLabel(const char* label) { strcpy(mLabel, label); }
  
  /** Set the function to translate display values
   * @param func A function conforming to DisplayFunc */
  void SetDisplayFunc(DisplayFunc func) { mDisplayFunction = func; }

  /** Gets a readable value of the parameter
   * @return double Current value of the parameter */
  double Value() const { return mValue.load(); }

  /** Returns the parameter's value as a boolean
   * @return \c true if value >= 0.5, else otherwise */
  bool Bool() const { return (mValue.load() >= 0.5); }

  /** Returns the parameter's value as an integer
  @return Current value of the parameter as an integer */
  int Int() const { return static_cast<int>(mValue.load()); }
  
  /** Gain based on parameter's current value in dB
   * @return double Gain calculated as an approximation of
   * \f$ 10^{\frac{x}{20}} \f$
   * @see #IAMP_DB */
  double DBToAmp() const { return iplug::DBToAmp(mValue.load()); }

  /** Returns the parameter's normalized value
   * @return double The resulting normalized value */
  double GetNormalized() const { return ToNormalized(mValue.load()); }

  /** Get the current textual display for the current parameter value
   * @param display \c WDL_String to fill with the results
   * @param withDisplayText Should the output include display texts */
  void GetDisplay(WDL_String& display, bool withDisplayText = true) const { GetDisplay(mValue.load(), false, display, withDisplayText); }

  /** Get the current textual display for a specified parameter value
   * @param value The value to get the display for
   * @param normalized Is value normalized or real
   * @param display \c WDL_String to fill with the results
   * @param withDisplayText Should the output include display texts */
  void GetDisplay(double value, bool normalized, WDL_String& display, bool withDisplayText = true) const;

  /** Fills the \c WDL_String the value of the parameter along with the label, e.g. units
   * @param display \c WDL_String to fill with the results
   * @param withDisplayText Should the output include display texts */
  void GetDisplayWithLabel(WDL_String& display, bool withDisplayText = true) const
  {
    GetDisplay(mValue.load(), false, display, withDisplayText);
    const char* hostlabel = GetLabel();
    if (CStringHasContents(hostlabel))
    {
      display.Append(" ");
      display.Append(hostlabel);
    }
  }
  
  /** Returns the parameter's name
   * @return CString with the parameter's name */
  const char* GetName() const;

  /** Returns the parameter's label
   * @return CString with the parameter's label */
  const char* GetLabel() const;

  /** Returns the parameter's group
   * @return CString with the parameter's group */
  const char* GetGroup() const;

  /** Get parameter's label (unit suffix)
   * @return CString Parameter's label (unit suffix) or \c nullptr if it is not set */
  const char* GetCustomUnit() const { return mUnit == kUnitCustom ? mLabel : nullptr; }
  
  /** Get the number of display texts for the parameter
   * @return The number of display texts for the parameter */
  int NDisplayTexts() const;

  /** Get the display text for a particular value 
   * @param value The value to get the display text for
   * @return CString The display text */
  const char* GetDisplayText(double value) const;

  /** Get the display text at a particular index
   * @param idx The index of the display text
   * @param pValue The value linked to the display text will be put here
   * @return CString The display text */
  const char* GetDisplayTextAtIdx(int idx, double* pValue = nullptr) const;

  /** Get the value of a particular display text 
   * @param str The display text to look up
   * @param pValue The value linked to the display text will be put here
   * @return \c true if str matched a display text */
  bool MapDisplayText(const char* str, double* pValue) const;
  
  /** Get the parameter's type
   * @return EParamType Type of the parameter, @e kTypeNone if not initialized
   * @see EParamType */
  EParamType Type() const { return mType; }

  /** Get the parameter's unit
   * @note This is only used for AU plugins to determine the appearance of parameters, based on the kind of data they represent 
   * @return EParamUnit */
  EParamUnit Unit() const { return mUnit; }

 /** Get the parameter's display type
   * @note This is only used for AU plugins to determine the mapping of parameters
   * @return EDisplayType */
  EDisplayType DisplayType() const { return mShape->GetDisplayType(); }
  
  /** Returns the parameter's default value
   * @param normalized Should the returned value be the default as a normalized or real value
   * @return double The parameter's default value  */
  double GetDefault(bool normalized = false) const { return normalized ? ToNormalized(GetDefault()) : mDefault; }
  
  /** Returns the parameter's minimum value
   * @return The minimum real value of the parameter's range */
  double GetMin() const { return mMin; }

  /** Returns the parameter's maximum value
   * @return The maximum real value of the parameter's range */
  double GetMax() const { return mMax; }
  
  /** Get the minimum and maximum real value of the parameter's range in one method call
   * @param lo The minimum value will be put here
   * @param hi The maximum value will be put here */
  void GetBounds(double& lo, double& hi) const;

  /** Returns the parameter's range
   * @return double The difference between the parameter's maximum and minimum bounds */
  double GetRange() const { return mMax - mMin; }

  /** Returns the parameter's step size
   * @return The parameter's step size */
  double GetStep() const { return mStep; }

  /** Returns the parameter's precision
   * @return int The number of decimal places that should be used to display the parameter's real value */
  int GetDisplayPrecision() const {return mDisplayPrecision;}
  
  /** Returns the parameter's flags
   * @return int The parameter's flags as an integer */
  int GetFlags() const { return mFlags; }

  /** @return \c true If the parameter should be automateable  */
  bool GetCanAutomate() const { return !(mFlags & kFlagCannotAutomate); }

  /** @return \c true If the parameter should be discrete (stepped)  */
  bool GetStepped() const { return mFlags & kFlagStepped; }

  /** @return \c true If the parameter should be displayed as a negative value */
  bool GetNegateDisplay() const { return mFlags & kFlagNegateDisplay; }

  /** @return \c true If the parameter should be displayed as a signed value */
  bool GetSignDisplay() const { return mFlags & kFlagSignDisplay; }

  /** @return \c true If the parameter is flagged as a "meta" parameter, e.g. one that could modify other parameters */
  bool GetMeta() const { return mFlags & kFlagMeta; }
 
  /** Get a JSON description of the parameter. 
   * @param json WDL_String to fill with the JSON
   * @param idx Index of the parameter, to place in the JSON */
  void GetJSON(WDL_String& json, int idx) const;

  /** Helper to print the parameter details to debug console in debug builds */
  void PrintDetails() const;
private:
  /** A DisplayText is used to link a certain real value of the parameter with a CString. For example -70 on a decibel gain parameter could instead read "-inf" */
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
