#pragma once
#include <cassert>
#include <cstring>
#include <stdint.h>

#include "ptrlist.h"

#include "IPlugParameter.h"

/** This pure virtual interface delegates communication in both directions between a UI editor and something else (which is usually a plug-in)
 *  It is also the class that owns parameter objects
 *  It needn't be a "plug-in" that implements this interface, it can also be used for other things
 *  An example use case: you would like to pop up a custom preferences window with a few simple checkboxes.
 *  You should be able to do that with a new graphics context and something implementing this interface in order to send/receive values
 *  to/from your new UI.
 *
 *  Note on method names: "FromUI" in a method name, means that that method is called by the UI class. Likewise "ToUI" means 
 *  that the method is delivering something wait for it... to the UI.
 *  The words "FromDelegate" in a method name mean that method is called from the class that implements the IDelegate interface,
 *  which is usually your plug-in base class, but may not be in the case of an isolated editor class, or if you are using IGraphics for a separate task.
 *
 *  A parameter VALUE is a floating point number linked to an integer parameter index. TODO: Normalised ?
 *  A parameter OBJECT (IParam) is an instance of the IParam class as defined in IPlugParameter.h
 *  A parameter OBJECT is also referred to as a "param", in method names such as IDelegate::GetParam(int paramIdx) and IControl::GetParam(). */

class IDelegate
{
public:
  IDelegate(int nParams)
  {
    for (int i = 0; i < nParams; ++i)
      mParams.Add(new IParam());
  }
  
  virtual ~IDelegate()
  {
    mParams.Empty(true);
  }
  
  /** Get a pointer to one of the delegate's IParam objects
   * @param paramIdx The index of the parameter object to be got
   * @return A pointer to the IParam object at paramIdx */
  IParam* GetParam(int paramIdx) { return mParams.Get(paramIdx); }
  
  /** @return Returns the number of parameters that belong to the plug-in. */
  int NParams() const { return mParams.GetSize(); }
  
#pragma mark - Methods you may want to override...
  
  /** Override this method when not using IGraphics in order to return a platform view handle e.g. NSView, UIView, HWND */
  virtual void* OpenWindow(void* pHandle) { return nullptr; }
  
  /** Override this method when not using IGraphics if you need to free resources etc when the window closes */
  virtual void CloseWindow() {};
  
  /** This is an OnParamChange that will only trigger on the UI thread at low priority, and therefore is appropriate for hiding or showing elements of the UI.
   * You should not update parameter objects using this method
   * @param paramIdx The index of the parameter that changed */
  virtual void OnParamChangeUI(int paramIdx) {};
  
  /** This is called by API classes after restoring state and by IPresetsDelegate::RestorePreset(). Typically used to update user interface, where multiple parameter values have changed.
   * If you need to do something when state is restored you can override it */
  virtual void OnRestoreState() {};
  
#pragma mark - DELEGATION methods for sending values TO the user interface
  // The following methods are called from the plug-in/delegate class in order to update the user interface.
  
  /** In IGraphics plug-ins, this method is used to update IControls in the user interface from the plug-in class, when the control is not linked
   * to a parameter, for example a typical use case would be a meter control. It is called by the IPlug "user" a.k.a you - not by an API class.
   * Somewhere in ProcessBlock() the plug-in would call this method to update the IControl's value.
   * If you are not using IGraphics,  you could use it in a similar way, as long as your control/meter has a unique index
   * @param controlIdx The index of the control in the IGraphics::mControls list.
   * @param normalizedValue The normalised value to set the control to. This will modify IControl::mValue; */
  virtual void SetControlValueFromDelegate(int controlIdx, double normalizedValue) {};
  
  /** This method is called by the class implementing DELEGATE, NOT THE PLUGIN API class in order to update the user interface with the new parameter values, typically after automation.
   * This method should only be called from the main thread. The similarly named IPlugBase::_SendParameterValueToUIFromAPI() should take care of queueing and deferring, if there is no main thread notification from the API
   * If you override this method you should call the base class implementation to make sure OnParamChangeUI gets triggered
   * In IGraphics plug-ins, this will update any IControls that have their mParamIdx set > -1
   * @param paramIdx The index of the parameter to be updated
   * @param value The new value of the parameter
   * @param normalized \c true if value is normalised */
  virtual void SendParameterValueToUIFromDelegate(int paramIdx, double value, bool normalized) { OnParamChangeUI(paramIdx); } // TODO: normalised?

#pragma mark - DELEGATION methods for sending values FROM the user interface
  // The following methods are called from the user interface in order to set or query values of parameters in the class implementing IDelegate
  
  /** Called by the user interface at the beginning of a parameter change gesture, in order to notify the host
   * (via a call in the API class) that the parameter is going to be modified
   * The host may be trying to automate the parameter as well, so it needs to relinquish control when the user is
   * modifying something in the user interface.
   * @param paramIdx The index of the parameter that is changing value */
  virtual void BeginInformHostOfParamChangeFromUI(int paramIdx) = 0;
  
  /** Called by the user interface during a parameter change gesture, in order to notify the host of the new value (via a call in the API class)
   * If you override this method you should call the base class implementation to make sure OnParamChangeUI gets triggered
   * @param paramIdx The index of the parameter that is changing value
   * @param value The new normalised value of the parameter */
  virtual void SetParameterValueFromUI(int paramIdx, double normalizedValue)
  {
    assert(paramIdx < NParams());
    
    GetParam(paramIdx)->SetNormalized(normalizedValue);
    OnParamChangeUI(paramIdx);
  }
  
  /** Called by the user interface at the end of a parameter change gesture, in order to notify the host
   * (via a call in the API class) that the parameter is no longer being modified
   * The host may be trying to automate the parameter as well, so it needs to relinquish control when the user is
   * modifying something in the user interface.
   * @param paramIdx The index of the parameter that is changing value */
  virtual void EndInformHostOfParamChangeFromUI(int paramIdx) = 0;
  
  /** Sometimes when a plug-in wants to change its UI dimensions we need to call into the plug-in api class first when we click a button in our UI
   * This method is overrided in various classes that inherit this interface to implement that behaviour */
  virtual void ResizeGraphicsFromUI() {};
  
  /** When we want to send a MIDI message from the UI for example clicking on a key in a virtual keyboard, this method should be used rather than
   * @param status The status byte of the MIDI message
   * @param data1 The first data byte of the MIDI message
   * @param data2 The second data byte of the MIDI message*/
  virtual void SendMidiMsgFromUI(uint8_t status, uint8_t data1, uint8_t data2) {};
  
protected:
  /** A list of IParam objects. This list is populated in the delicate constructor depending on the number of parameters passed as an argument to IPLUG_CTOR in the plugin class implementation constructor */
  WDL_PtrList<IParam> mParams;
};
