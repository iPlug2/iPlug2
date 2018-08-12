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
#include <cassert>
#include <cstring>
#include <stdint.h>

#include "ptrlist.h"

#include "IPlugParameter.h"
#include "IPlugMidi.h"

/** This pure virtual interface delegates communication in both directions between a UI editor and something else (which is usually a plug-in)
 *  It is also the class that owns parameter objects (for historical reasons) - although it's not necessary to allocate them
 *
 *  In distributed plug-in architectures and remote editors, certain methods will be overridden in order to pipe messages to various places
 *
 *  It needn't be a "plug-in" that implements this interface, it can also be used for other things
 *  An example use case: you would like to pop up a custom preferences window with a few simple checkboxes.
 *  You should be able to do that with a new graphics context and something implementing this interface in order to send/receive values
 *  to/from your new UI.
 *
 *  Note on method names:
 *  - "FromUI" in a method name, means that that method is called by something in the UI i.e. a control.
 *  - "FromDelegate" in a method name mean that method is called from the class that implements the IEditorDelegate interface,
 *     which is usually your plug-in base class, but may not be in the case of an isolated editor class, or if you are using IGraphics without IPlug, and your IEditorDelegate is not a plug-in
 *
 *  NOTES:
 *  A parameter VALUE is a floating point number linked to an integer parameter index. TODO: Normalised ?
 *  A parameter OBJECT (IParam) is an instance of the IParam class as defined in IPlugParameter.h
 *  A parameter OBJECT is also referred to as a "param", in method names such as IEditorDelegate::GetParam(int paramIdx) and IControl::GetParam(). */

class IEditorDelegate
{
public:
  IEditorDelegate(int nParams)
  {
    for (int i = 0; i < nParams; i++)
      AddParam();
  }
  
  virtual ~IEditorDelegate()
  {
    mParams.Empty(true);
  }
  
  IParam* AddParam() { return mParams.Add(new IParam()); }
  
  void RemoveParam(int idx) { return mParams.Delete(idx); }
  
  /** Get a pointer to one of the delegate's IParam objects
   * @param paramIdx The index of the parameter object to be got
   * @return A pointer to the IParam object at paramIdx or nullptr if paramIdx is invalid */
  IParam* GetParam(int paramIdx) { return mParams.Get(paramIdx); }
  
  /** @return Returns the number of parameters that belong to the plug-in. */
  int NParams() const { return mParams.GetSize(); }
  
#pragma mark - Methods you may want to override...
  
  /** Override this method when not using IGraphics in order to hook into the native parent view e.g. NSView, UIView, HWND */
  virtual void* OpenWindow(void* pParent) { return nullptr; }
  
  /** Override this method when not using IGraphics if you need to free resources etc when the window closes */
  virtual void CloseWindow() {};
  
  /** Override this method to do something before the UI is opened. Call base implementations. */
  virtual void OnUIOpen()
  {
    for (auto i = 0; i < NParams(); ++i)
    {
      SendParameterValueFromDelegate(i, GetParam(i)->GetNormalized(), true);
    }
  };
  
  /** Override this method to do something before the UI is closed. */
  virtual void OnUIClose() {};
  
  /** This is an OnParamChange that will only trigger on the UI thread at low priority, and therefore is appropriate for hiding or showing elements of the UI.
   * You should not update parameter objects using this method.
   * @param paramIdx The index of the parameter that changed */
  virtual void OnParamChangeUI(int paramIdx, EParamSource source) {};
  
  /** TODO: */
  virtual void OnMidiMsgUI(const IMidiMsg& msg) {};
  
  /** TODO: */
  virtual void OnSysexMsgUI(const ISysEx& msg) {};
  
  /** This could be implemented in either DSP or EDITOR to receive a message from the other one */
  virtual bool OnMessage(int messageTag, int dataSize, const void* pData) { return false; }
  
  /** This is called by API classes after restoring state and by IPluginBase::RestorePreset(). Typically used to update user interface, where multiple parameter values have changed.
   * If you need to do something when state is restored you can override it */
  virtual void OnRestoreState() {};
  
#pragma mark - Methods for sending values TO the user interface
  // The following methods are called from the plug-in/delegate class in order to update the user interface.
  
  /** TODO: SCVFD */
  /** In IGraphics plug-ins, this method is used to update IControls in the user interface from the plug-in class, when the control is not linked
   * to a parameter, for example a typical use case would be a meter control. It is called by the IPlug "user" a.k.a you - not by an API class.
   * In OnIdle() your plug-in would call this method to update the IControl's value. YOU SHOULD NOT CALL THIS ON THE AUDIO THREAD!
   * If you are not using IGraphics,  you could use it in a similar way, as long as your control/meter has a unique tag
   * @param controlTag A tag for the control
   * @param normalizedValue The normalised value to set the control to. This will modify IControl::mValue; */
  virtual void SendControlValueFromDelegate(int controlTag, double normalizedValue) {};
  
  /** TODO: SCMFD */
  virtual void SendControlMsgFromDelegate(int controlTag, int messageTag, int dataSize = 0, const void* pData = nullptr) {};
  
  /** TODO: SAMFD */
  virtual void SendArbitraryMsgFromDelegate(int messageTag, int dataSize, const void* pData) {};
  
  /** TODO: SMMFD */
  virtual void SendMidiMsgFromDelegate(const IMidiMsg& msg) { OnMidiMsgUI(msg); }
  
  /** TODO: SSMFD */
  virtual void SendSysexMsgFromDelegate(const ISysEx& msg) { OnSysexMsgUI(msg); }
  
  /** This method is called by the class implementing the delegate interface, NOT THE PLUGIN API class in order to update the user interface with the new parameter values, typically after automation.
   * This method should only be called from the main thread. The similarly named IPlugAPIBase::_SendParameterValueFromAPI() should take care of queueing and deferring, if there is no main thread notification from the API
   * If you override this method you should call the base class implementation to make sure OnParamChangeUI gets triggered
   * In IGraphics plug-ins, this will update any IControls that have their mParamIdx set > -1
   * @param paramIdx The index of the parameter to be updated
   * @param value The new value of the parameter
   * @param normalized \c true if value is normalised */
  virtual void SendParameterValueFromDelegate(int paramIdx, double value, bool normalized) { OnParamChangeUI(paramIdx, EParamSource::kDelegate); } // TODO: normalised?

#pragma mark - Methods for sending values FROM the user interface
  // The following methods are called from the user interface in order to set or query values of parameters in the class implementing IEditorDelegate
  
  /** Called by the UI at the beginning of a parameter change gesture, in order to notify the host
   * (via a call in the API class) that the parameter is going to be modified
   * The host may be trying to automate the parameter as well, so it needs to relinquish control when the user is modifying something in the user interface.
   * @param paramIdx The index of the parameter that is changing value */
  virtual void BeginInformHostOfParamChangeFromUI(int paramIdx) = 0;
  
  /** SPVFUI Called by the UI during a parameter change gesture, in order to notify the host of the new value (via a call in the API class)
   * If you override this method you should call the base class implementation to make sure OnParamChangeUI gets triggered
   * @param paramIdx The index of the parameter that is changing value
   * @param value The new normalised value of the parameter */
  virtual void SendParameterValueFromUI(int paramIdx, double normalizedValue)
  {
    assert(paramIdx < NParams());
    
    GetParam(paramIdx)->SetNormalized(normalizedValue);
    OnParamChangeUI(paramIdx, EParamSource::kUI);
  }
  
  /** Called by the user interface at the end of a parameter change gesture, in order to notify the host
   * (via a call in the API class) that the parameter is no longer being modified
   * The host may be trying to automate the parameter as well, so it needs to relinquish control when the user is
   * modifying something in the user interface.
   * @param paramIdx The index of the parameter that is changing value */
  virtual void EndInformHostOfParamChangeFromUI(int paramIdx) = 0;
  
  /** Sometimes when a plug-in wants to change its UI dimensions we need to call into the plug-in api class first when we click a button in our UI
   * This method is implemented in various classes that inherit this interface to implement that behaviour */
  virtual void ResizeGraphicsFromUI(int viewWidth, int viewHeight, float scale) {};
  
  /** TODO: SMMFUI */
  /** When we want to send a MIDI message from the UI for example clicking on a key in a virtual keyboard, this method should be used*/
  virtual void SendMidiMsgFromUI(const IMidiMsg& msg) {};

  /** TODO: SSMFUI */
  virtual void SendSysexMsgFromUI(const ISysEx& msg) {};
  
  /** TODO: SAMFUI */
  virtual void SendArbitraryMsgFromUI(int messageTag, int dataSize = 0, const void* pData = nullptr) {};

#pragma mark -
  /** This method is needed, for remote editors to avoid a feedback loop */
  virtual void DeferMidiMsg(const IMidiMsg& msg) {};
  
#pragma mark
  /** @return The width of the plug-in editor in pixels */
  int GetEditorWidth() const { return mEditorWidth; }
  
  /** @return The height of the plug-in editor in pixels */
  int GetEditorHeight() const { return mEditorHeight; }
  
  /** @return Any scaling applied to the UI  */
  float GetEditorScale() const { return mEditorScale; }
  
  /** @return Get the editor layout index (if used) */
  int GetEditorLayout() const { return mEditorLayoutIdx; }
  
protected:
  /** The width of the plug-in editor in pixels. Can be updated by resizing, exists here for persistance.  */
  int mEditorWidth = 0;
  /** The height of the plug-in editor in pixels. Can be updated by resizing, exists here for persistance.*/
  int mEditorHeight = 0;
  /** Any scaling of the plug-in editor. Can be updated by resizing, exists here for persistance.*/
  float mEditorScale = 1.f;
  /* An index representing one of several UI layouts for the editor, if needed, exists here for persistance */
  int mEditorLayoutIdx = 0;
  /** A list of IParam objects. This list is populated in the delegate constructor depending on the number of parameters passed as an argument to IPLUG_CTOR in the plug-in class implementation constructor */
  WDL_PtrList<IParam> mParams;
};
