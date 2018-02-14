#pragma once

class IParam;

/** This pure virtual interface delegates/controls communication in both directions between the UI and the plug-in's main class/API class */
class IDelegate
{
public:
  IDelegate() {}
  virtual ~IDelegate() {}
  
#pragma mark -
  // The following methods are called from the plug-in class in order to update the user interface.
  
  /** In IGraphics plug-ins, this method is used to update IControls in the user interface from the plug-in class, when the control is not linked
   * to a parameter, for example a typical use case would be a meter control. It is called by the IPlug "user" a.k.a you - not by an API class.
   * Somewhere in ProcessBlock() the plug-in would call this method to update the IControl's value.
   * If you are not using IGraphics,  you could use it in a similar way, as long as your control/meter has a unique index
   * @param controlIdx The index of the control in the IGraphics::mControls list.
   * @param normalizedValue The normalised value to set the control to. This will modify IControl::mValue; */
  virtual void SetControlValueFromDelegate(int controlIdx, double normalizedValue) = 0;
  
  /** This method is called by the API class in order to update the user interface's knowledge of parameter values, typically after automation.
   * In IGraphics plug-ins, this will update any IControls that have their mParamIdx set > -1
   * @param paramIdx The index of the parameter to be updated
   * @param value The new value of the parameter
   * @param normalized \c true if value is normalised */
  virtual void SendParameterValueToUIFromDelegate(int paramIdx, double value, bool normalized) = 0;

#pragma mark -
  // The following methods are called from the user interface in order to set values of "parameters" in the plug-in class
  
  /** Called by the user interface in order to get a const pointer to an IParam object
   * @param paramIdx The index of the parameter to be retrieved
   * @return Pointer to an IParam object */
  virtual const IParam* GetParamFromUI(int paramIdx) = 0;
  
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
};
