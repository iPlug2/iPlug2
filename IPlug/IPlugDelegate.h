#pragma once

#include <random>

#include "ptrlist.h"

#include "IPlugParameter.h"

/** This pure virtual interface delegates communication in both directions between the UI and the plug-in's main class/API class.
 *  It needn't be a "plug-in" that implements this interface, it can also be used for other things
 *  An example use case: you would like to pop up a custom preferences window with a few simple checkboxes.
 *  You should be able to do that with a new graphics context and something implementing this interface in order to send/receive values
 *  to/from your new UI.
 *
 *  Note on method names: "FromUI" in a method name, means that that method is called by the UI class. Likewise "ToUI" means 
 *  that the method is delivering something wait for it... to the UI.
 *  The words "FromDelegate" in a method name mean that method is called from the class that implements the IDelegate interface,
 *  which is usually your plug-in base class. A parameter value is a floating point number linked to an integer parameter index.
 *  A parameter object is an instance of the IParam class as defined in IPlugParameter.h, owned by IPlugBase.
 *  A parameter object is also referred to as a "param", in method names such as IPlugBase::GetParam(int paramIdx) and IControl::GetParam().
 */
class IDelegate
{
public:
  IDelegate(int nParams);
  virtual ~IDelegate();
  
#pragma mark -
  IParam* GetParam(int paramIdx) { return mParams.Get(paramIdx); }

  /** @return Returns the number of parameters that belong to the plug-in. */
  int NParams() const { return mParams.GetSize(); }
  
  /** @return The number of unique parameter groups identified */
  int NParamGroups() { return mParamGroups.GetSize(); }
  
  /** Called to add a parameter group name, when a unique group name is discovered
   * @param name CString for the unique group name
   * @return Number of parameter groups */
  int AddParamGroup(const char* name) { mParamGroups.Add(name); return NParamGroups(); }
  
  /** Get the parameter group name as a particular index
   * @param idx The index to return
   * @return CString for the unique group name */
  const char* GetParamGroupName(int idx) { return mParamGroups.Get(idx); }
  
  /** Initialise a range of parameters simultaneously. This mirrors the arguments available in IParam::InitDouble, for maximum flexibility
   * @param startIdx The index of the first parameter to initialise
   * @param endIdx The index of the last parameter to initialise
   * @param countStart An integer representing the start of the count in the format string. If the first parameter should have "0" in its name, set this to 0
   * @param nameFmtStr A limited format string where %i can be used to get the index + countStart, in the range of parameters specified
   * @param defaultVal A default real value for the parameter
   * @param minVal A minimum real value for the parameter
   * @param maxVal A Maximum real value for the parameter
   * @param step The parameter step
   * @param label A CString label for the parameter e.g. "decibels"
   * @param flags Any flags, see IParam::EFlags
   * @param group A CString group name for the parameter, e.g. "envelope"
   * @param shape A IParam::Shape class to determine how the parameter shape should be skewed
   * @param unit An IParam::EParamUnit which can be used in audiounit plug-ins to specify certain kinds of parameter
   * @param displayFunc An IParam::DisplayFunc lambda function to specify a custom display function */
  void InitParamRange(int startIdx, int endIdx, int countStart, const char* nameFmtStr, double defaultVal, double minVal, double maxVal, double step, const char* label = "", int flags = 0, const char* group = "", IParam::Shape* shape = nullptr, IParam::EParamUnit unit = IParam::kUnitCustom, IParam::DisplayFunc displayFunc = nullptr);
  
  /** Clone a range of parameters, optionally doing a string substitution on the parameter name.
   * @param cloneStartIdx The index of the first parameter to clone
   * @param cloneEndIdx The index of the last parameter to clone
   * @param startIdx The start of the cloned range
   * @param searchStr A CString to search for in the input parameter name
   * @param replaceStr A CString to replace searchStr in the output parameter name
   * @param newGroup If the new parameter should have a different group, update here */
  void CloneParamRange(int cloneStartIdx, int cloneEndIdx, int startIdx, const char* searchStr = "", const char* replaceStr = "", const char* newGroup = "");
  
  /** Modify a range of parameters with a lamda function
   * @param startIdx The index of the first parameter to modify
   * @param endIdx The index of the last parameter to modify
   * @param func A lambda function to modify the parameter. Ideas: you could randomise the parameter value or reset to default, modify certain params based on their group */
  void ModifyParamValues(int startIdx, int endIdx, std::function<void(IParam& param)> func);
  
  /** Modify a parameter group simulataneously
   * @param paramGroup The name of the group to modify
   * @param param func A lambda function to modify the parameter. Ideas: you could randomise the parameter value or reset to default*/
  void ModifyParamValues(const char* paramGroup, std::function<void(IParam& param)> func);
  
  /** Copy a range of parameter values
   * @param startIdx The index of the first parameter value to copy
   * @param destIdx The index of the first destination parameter
   * @param nParams The number of parameters to copy */
  void CopyParamValues(int startIdx, int destIdx, int nParams);
  
  /** Copy a range of parameter values for a parameter group
   * @param inGroup The name of the group to copy from
   * @param outGroup The name of the group to copy to */
  void CopyParamValues(const char* inGroup, const char* outGroup);
  
  /** Randomise parameter values within a range. NOTE for more flexibility in terms of RNG etc, use ModifyParamValues()
   * @param startIdx The index of the first parameter to modify
   * @param endIdx The index of the last parameter to modify */
  void RandomiseParamValues(int startIdx, int endIdx);
  
  /** Randomise parameter values for a parameter group
   * @param paramGroup The name of the group to modify */
  void RandomiseParamValues(const char* paramGroup);
  
  /** Default parameter values within a range.
   * @param startIdx The index of the first parameter to modify
   * @param endIdx The index of the last parameter to modify */
  void DefaultParamValues(int startIdx, int endIdx);
  
  /** Default parameter values for a parameter group
   * @param paramGroup The name of the group to modify */
  void DefaultParamValues(const char* paramGroup);
  
#pragma mark -
  // The following methods are called from the plug-in/delegate class in order to update the user interface.
  
  /** In IGraphics plug-ins, this method is used to update IControls in the user interface from the plug-in class, when the control is not linked
   * to a parameter, for example a typical use case would be a meter control. It is called by the IPlug "user" a.k.a you - not by an API class.
   * Somewhere in ProcessBlock() the plug-in would call this method to update the IControl's value.
   * If you are not using IGraphics,  you could use it in a similar way, as long as your control/meter has a unique index
   * @param controlIdx The index of the control in the IGraphics::mControls list.
   * @param normalizedValue The normalised value to set the control to. This will modify IControl::mValue; */
  virtual void SetControlValueFromDelegate(int controlIdx, double normalizedValue) {};
  
  /** This method is called by the API class in order to update the user interface with the new parameter values, typically after automation.
   * In IGraphics plug-ins, this will update any IControls that have their mParamIdx set > -1
   * @param paramIdx The index of the parameter to be updated
   * @param value The new value of the parameter
   * @param normalized \c true if value is normalised */
  virtual void SendParameterValueToUIFromDelegate(int paramIdx, double value, bool normalized) {};

#pragma mark -
  // The following methods are called from the user interface in order to set or query values of parameters in the class implementing IDelegate
  
  /** Called by the user interface in order to get a const pointer to an IParam object
   * @param paramIdx The index of the parameter to be retrieved
   * @return Pointer to an IParam object */
  virtual const IParam* GetParamObjectFromUI(int paramIdx) { return GetParam(paramIdx); }
  
  /** Called by the user interface at the beginning of a parameter change gesture, in order to notify the host
   * (via a call in the API class) that the parameter is going to be modified
   * The host may be trying to automate the parameter as well, so it needs to relinquish control when the user is
   * modifying something in the user interface.
   * @param paramIdx The index of the parameter that is changing value */
  virtual void BeginInformHostOfParamChangeFromUI(int paramIdx) = 0;
  
  /** Called by the user interface during a parameter change gesture, in order to notify the host of the new value (via a call in the API class)
   * @param paramIdx The index of the parameter that is changing value
   * @param value The new normalised value of the parameter */
  virtual void SetParameterValueFromUI(int paramIdx, double normalizedValue) = 0;
  
  /** Called by the user interface at the end of a parameter change gesture, in order to notify the host
   * (via a call in the API class) that the parameter is no longer being modified
   * The host may be trying to automate the parameter as well, so it needs to relinquish control when the user is
   * modifying something in the user interface.
   * @param paramIdx The index of the parameter that is changing value */
  virtual void EndInformHostOfParamChangeFromUI(int paramIdx) = 0;
  
  virtual void ResizeGraphicsFromUI() {};
  
  virtual void SendMidiMsgFromUI(uint8_t status, uint8_t data1, uint8_t data2) {};
  
protected:
  /** A list of unique cstrings found specified as "parameter groups" when defining IParams. These are used in various APIs to group parameters together in automation dialogues. */
  WDL_PtrList<const char> mParamGroups;
  /** A list of IParam objects. This list is populated in the delicate constructor depending on the number of parameters passed as an argument to IPLUG_CTOR in the plugin class implementation constructor */
  WDL_PtrList<IParam> mParams;
};
