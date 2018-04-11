#pragma once

#include <cstring>
#include <cstdint>

#include "ptrlist.h"
#include "mutex.h"

#include "IPlugPlatform.h"
#include "IPlugDelegate_select.h"
#include "IPlugConstants.h"
#include "IPlugStructs.h"
#include "IPlugUtilities.h"
#include "IPlugParameter.h"
#include "IPlugQueue.h"
#include "IPlugTimer.h"

/**
 * @file
 * @copydoc IPlugBase
 * @defgroup APIClasses IPlug::APIClasses
*/

struct IPlugConfig;

/** The base class of an IPlug plug-in, which interacts with the different plug-in APIs. No UI framework code here.
 *  This interface does not handle audio processing, see @IPlugProcessor  */
class IPlugBase : public PLUGBASE_SUPER_CLASS
                , public ITimerCallback
{

public:
  IPlugBase(IPlugConfig config, EAPI plugAPI);
  virtual ~IPlugBase();

#pragma mark - Methods you can implement/override in your plug-in class - you do not call these methods

  /** Override this method to implement a custom comparison of incoming state data with your plug-ins state data, in order
   * to support the ProTools compare light when using custom state chunks. The default implementation will compare the serialized parameters.
   * @param incomingState The incoming state data
   * @param startPos The position to start in the incoming data in bytes
   * @return \c true in order to indicate that the states are equal. */
  virtual bool CompareState(const uint8_t* pIncomingState, int startPos);

  /** Override this method to be notified when the UI is opened. */
  virtual void OnUIOpen() { TRACE; }

  /** Override this method to be notified when the UI is closed. */
  virtual void OnUIClose() { TRACE; }

  /** Implement this to do something after the user interface is resized */
  virtual void OnWindowResize() {}

  /* implement this and return true to trigger your custom about box, when someone clicks about in the menu of a standalone app or VST3 plugin */
  virtual void OnHostRequestingAboutBox() {} // TODO: implement this for VST 3

  /* implement this and return true to trigger your custom help info, when someone clicks help in the menu of a standalone app or VST3 plugin */
  virtual void OnHostRequestingProductHelp() {} // TODO: implement this for VST 3
  
  /** Implement this to do something specific when IPlug becomes aware of the particular host that is hosting the plug-in.
   * The method may get called multiple times. */
  virtual void OnHostIdentified() {}

  /** Override this method to provide custom text linked to MIDI note numbers in API classes that support that (VST2)
   * Typically this might be used for a drum machine plug-in, in order to label a certainty "kick drum" etc.
   * @param noteNumber MIDI note to get the textual description for
   * @param str char array to set the text for the note. Should be less thatn kVstMaxNameLen (64) characters
   * @return \c true if you specified a custom textual description for this note */
  virtual bool GetMidiNoteText(int noteNumber, char* str) const { *str = '\0'; return false; }

  /** You need to implement this method if you are not using IGraphics and you want to support AAX's view interface functionality
   * (special shortcuts to add automation for a parameter etc.)
   * @return pointer to the class that implements the IAAXViewInterface */
  virtual void* GetAAXViewInterface() { return nullptr; }
  
  /** Override this method to get an "idle"" call on the main thread */
  virtual void OnIdle() {}
  
#pragma mark - Methods you can call - some of which have custom implementations in the API classes, some implemented in IPlugBase.cpp;
  /** Helper method, used to print some info to the console in debug builds. Can be overridden in other IPlugBases, for specific functionality, such as printing UI details. */
  virtual void PrintDebugInfo() const;

  /** This method will loop through all parameters, telling the host that they changed. You can use it if you restore a preset using a custom preset mechanism.*/
  void DirtyParameters(); // TODO: This is a hack to tell the host to dirty the project state, when a preset is recalled, is it necessary?

  /** Call this method in order to notify the API of a graphics resize. */
  virtual void ResizeGraphics() = 0;

  /** Implemented by the API class, called by the UI (or by a delegate) at the beginning of a parameter change gesture
   * @param paramIdx The parameter that is being changed */
  virtual void BeginInformHostOfParamChange(int paramIdx) = 0;

  /** Implemented by the API class, called by the UI (or by a delegate) at the end of a parameter change gesture
   * @param paramIdx The parameter that is being changed */
  virtual void EndInformHostOfParamChange(int paramIdx) = 0;

  /** SetParameterValue is called from the UI in the middle of a parameter change gesture (possibly via delegate) in order to update a parameter's value.
   * It will update mParams[paramIdx], call InformHostOfParamChange and IPlugBase::OnParamChange();
   * @param idx The index of the parameter that changed
   * @param normalizedValue The new (normalised) value*/
  void SetParameterValue(int paramIdx, double normalizedValue);

#pragma mark - Methods called by the API class - you do not call these methods in your plug-in class

  /** This is called from the plug-in API class in order to update UI controls linked to plug-in parameters, prior to calling OnParamChange()
   * NOTE: It may be called on the high priority audio thread. Its purpose is to place parameter changes in a queue to defer to main thread for the UI
   * @param paramIdx The index of the parameter that changed
   * @param value The new value
   * @param normalized /true if @param value is normalised */
  void _SendParameterValueToUIFromAPI(int paramIdx, double value, bool normalized);

  /** Called to set the name of the current host, if known.
  * @param host The name of the plug-in host
  * @param version The version of the plug-in host where version in hex = 0xVVVVRRMM */
  void SetHost(const char* host, int version);

  /** This method is called by some API classes, in order to do specific initialisation for particular problematic hosts.
   * This is not the same as OnHostIdentified(), which you may implement in your plug-in class to do your own specific initialisation after a host has been identified */
  virtual void HostSpecificInit() {}; //TODO: sort this method out, it's called differently from different APIs

  /** Calls OnParamChange() for each parameter and finally OnReset().
   * @param source Specifies the source of this parameter change */
  void OnParamReset(EParamSource source);  //

  //IDelegate
  void SetParameterValueFromUI(int paramIdx, double value) override { SetParameterValue(paramIdx, value); }
  void BeginInformHostOfParamChangeFromUI(int paramIdx) override { BeginInformHostOfParamChange(paramIdx); }
  void EndInformHostOfParamChangeFromUI(int paramIdx) override { EndInformHostOfParamChange(paramIdx); }
  
private:
  /** Implemented by the API class, called by the UI via SetParameterValue() with the value of a parameter change gesture
   * @param paramIdx The parameter that is being changed
   * @param normalizedValue The new normalised value of the parameter being changed */
  virtual void InformHostOfParamChange(int paramIdx, double normalizedValue) = 0;

private:
  void OnTimer(Timer& t) override;

public:
  struct ParamChange
  {
    int paramIdx;
    double value;
    bool normalized; // TODO: we shouldn't bother with this
  };
  
  IPlugQueue<ParamChange> mHighPriorityToUIQueue;
  /** Lock when accessing mParams (including via GetParam) from the audio thread */
  WDL_Mutex mParams_mutex;
  WDL_String mParamDisplayStr;
  Timer* mTimer = nullptr;
};
