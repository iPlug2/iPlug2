#pragma once

class IParam;

/** This interface controls communication between the user interface and the plug-in's main class/API class */
class IDelegate
{
public:
  IDelegate() {}
  virtual ~IDelegate() {}
  
  /** Called by the user interface in order to get a pointer to an IParam object
   * NOTE: IParam objects access by multiple threads. There is no thread safety mechanism here currently. BEWARE!
   * @param paramIdx The index of the parameter to be retrieved
   * @return Pointer to an IParam object */
  virtual IParam* GetParamFromUI(int paramIdx) = 0;
  
  /** Called by the user interface at the beginning of a parameter change gesture, in order to notify the host
   * (via a call in the API class) that the parameter is going to be modified
   * The host may be trying to automate the parameter as well, so it needs to relinquish control when the user is
   * modifying something in the user interface.
   * @param paramIdx The index of the parameter that is changing value */
  virtual void BeginInformHostOfParamChangeFromUI(int paramIdx) = 0;
  
  /** Called by the user interface during a parameter change gesture, in order to notify the host of the new value (via a call in the API class)
   * @param paramIdx The index of the parameter that is changing value
   * @param value The new normalised value of the parameter */
  virtual void SetParameterValueFromUI(int paramIdx, double value) = 0;
  
  /** Called by the user interface at the end of a parameter change gesture, in order to notify the host
   * (via a call in the API class) that the parameter is no longer being modified
   * The host may be trying to automate the parameter as well, so it needs to relinquish control when the user is
   * modifying something in the user interface.
   * @param paramIdx The index of the parameter that is changing value */
  virtual void EndInformHostOfParamChangeFromUI(int paramIdx) = 0;
};
