#pragma once

class IParam;

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
  IDelegate() {}
  virtual ~IDelegate() {}
  
#pragma mark -
  // The following methods are called from the plug-in/delegate class in order to update the user interface.
  
  /** In IGraphics plug-ins, this method is used to update IControls in the user interface from the plug-in class, when the control is not linked
   * to a parameter, for example a typical use case would be a meter control. It is called by the IPlug "user" a.k.a you - not by an API class.
   * Somewhere in ProcessBlock() the plug-in would call this method to update the IControl's value.
   * If you are not using IGraphics,  you could use it in a similar way, as long as your control/meter has a unique index
   * @param controlIdx The index of the control in the IGraphics::mControls list.
   * @param normalizedValue The normalised value to set the control to. This will modify IControl::mValue; */
  virtual void SetControlValueFromDelegate(int controlIdx, double normalizedValue) = 0;
  
  /** This method is called by the API class in order to update the user interface with the new parameter values, typically after automation.
   * In IGraphics plug-ins, this will update any IControls that have their mParamIdx set > -1
   * @param paramIdx The index of the parameter to be updated
   * @param value The new value of the parameter
   * @param normalized \c true if value is normalised */
  virtual void SendParameterValueToUIFromDelegate(int paramIdx, double value, bool normalized) = 0;

#pragma mark -
  // The following methods are called from the user interface in order to set or query values of parameters in the class implementing IDelegate
  
  /** Called by the user interface in order to get a const pointer to an IParam object
   * @param paramIdx The index of the parameter to be retrieved
   * @return Pointer to an IParam object */
  virtual const IParam* GetParamObjectFromUI(int paramIdx) = 0;
  
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
};
