/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

#include <cstring>
#include <cstdint>
#include <memory>

#include "ptrlist.h"
#include "mutex.h"

#include "IPlugPlatform.h"
#include "IPlugPluginBase.h"
#include "IPlugConstants.h"
#include "IPlugStructs.h"
#include "IPlugUtilities.h"
#include "IPlugParameter.h"
#include "IPlugQueue.h"
#include "IPlugTimer.h"

/**
 * @file
 * @copydoc IPlugAPIBase
 * @defgroup APIClasses IPlug::APIClasses
 * An IPlug API class is the base class for a particular audio plug-in API
*/

BEGIN_IPLUG_NAMESPACE

struct Config;

/** The base class of an IPlug plug-in, which interacts with the different plug-in APIs.
 *  This interface does not handle audio processing, see @IPlugProcessor  */
class IPlugAPIBase : public IPluginBase
{

public:
  IPlugAPIBase(Config config, EAPI plugAPI);
  virtual ~IPlugAPIBase();
  
  IPlugAPIBase(const IPlugAPIBase&) = delete;
  IPlugAPIBase& operator=(const IPlugAPIBase&) = delete;
  
#pragma mark - Methods you can implement/override in your plug-in class - you do not call these methods

  /** Override this method to implement a custom comparison of incoming state data with your plug-ins state data, in order
   * to support the ProTools compare light when using custom state chunks. The default implementation will compare the serialized parameters.
   * @param pIncomingState The incoming state data
   * @param startPos The position to start in the incoming data in bytes
   * @return \c true in order to indicate that the states are equal. */
  virtual bool CompareState(const uint8_t* pIncomingState, int startPos) const;

  /* implement this and return true to trigger your custom about box, when someone clicks about in the menu of a standalone app or VST3 plugin */
  virtual bool OnHostRequestingAboutBox() { return false; }

  /* implement this and return true to trigger your custom help info, when someone clicks help in the menu of a standalone app or VST3 plugin */
  virtual bool OnHostRequestingProductHelp() { return false; }
  
  /** Implement this to do something specific when IPlug becomes aware of the particular host that is hosting the plug-in.
   * The method may get called multiple times. */
  virtual void OnHostIdentified() {}
  
  /** Called by AUv3 plug-ins to get the "overview parameters"
   * @param count How many overview parameters
   * @param results You should populate this typed buf with the indexes of the overview parameters if the host wants to show count number of controls */
  virtual void OnHostRequestingImportantParameters(int count, WDL_TypedBuf<int>& results);
  
  /** Called by AUv3 plug-in hosts to query support for multiple UI sizes
   * @param width The width the host offers
   * @param height The height the host offers
   * @return return \c true if your plug-in supports these dimensions */
  virtual bool OnHostRequestingSupportedViewConfiguration(int width, int height) { return true; }
  
  /** Called by some AUv3 plug-in hosts when a particular UI size is selected
   * @param width The selected width
   * @param height The selected height */
  virtual void OnHostSelectedViewConfiguration(int width, int height) {}

  /** KeyDown handler for VST2, in order to get keystrokes from certain hosts 
   * @param key Information about the key that was pressed
   * @return \c true if the key was handled by the plug-in */
  virtual bool OnKeyDown(const IKeyPress& key) { return false; }

  /** KeyDown handler for VST2, in order to get keystrokes from certain hosts
   * @param key Information about the key that was released
   * @return \c true if the key was handled by the plug-in */
  virtual bool OnKeyUp(const IKeyPress& key) { return false; }

  /** Override this method to provide custom text linked to MIDI note numbers in API classes that support that (VST2)
   * Typically this might be used for a drum machine plug-in, in order to label a certainty "kick drum" etc.
   * @param noteNumber MIDI note to get the textual description for
   * @param str char array to set the text for the note. Should be less thatn kVstMaxNameLen (64) characters
   * @return \c true if you specified a custom textual description for this note */
  virtual bool GetMidiNoteText(int noteNumber, char* str) const { *str = '\0'; return false; }

  /** You need to implement this method if you are not using IGraphics and you want to support AAX's view interface functionality
   * (special shortcuts to add automation for a parameter etc.)
   * @return pointer to the class that implements the IAAXViewInterface */
  virtual void* GetAAXViewInterface()
  {
#ifndef NO_IGRAPHICS
    return (void*) GetUI();
#else
    return nullptr;
#endif
  }

  /** Override this method to get an "idle"" call on the main thread */
  virtual void OnIdle() {}
    
#pragma mark - Methods you can call - some of which have custom implementations in the API classes, some implemented in IPlugAPIBase.cpp
  /** Helper method, used to print some info to the console in debug builds. Can be overridden in other IPlugAPIBases, for specific functionality, such as printing UI details. */
  virtual void PrintDebugInfo() const;

  /** Call this method from a delegate in order to resize the plugin window.
   * If calling from a UI interaction use EditorResizeFromUI()
   * When this is overridden in subclasses the subclass should call this in order to update the member variables
   * returns a bool to indicate whether the DAW or plugin class has resized the host window */
  virtual bool EditorResizeFromDelegate(int width, int height);
  
   /** Call this method from a delegate if you want to store arbitrary data about the editor (e.g. layout/scale info).
   * If calling from a UI interaction use EditorDataChangedFromUI()
   * When this is overridden in subclasses the subclass should call this in order to update member variables */
   virtual void EditorDataChangedFromDelegate(const IByteChunk& data) { mEditorData = data; }
    
  /** Implemented by the API class, called by the UI (or by a delegate) at the beginning of a parameter change gesture
   * @param paramIdx The parameter that is being changed */
  virtual void BeginInformHostOfParamChange(int paramIdx) {};

  /** Implemented by the API class, called by the UI (or by a delegate) at the end of a parameter change gesture
   * @param paramIdx The parameter that is being changed */
  virtual void EndInformHostOfParamChange(int paramIdx) {};

  /** SetParameterValue is called from the UI in the middle of a parameter change gesture (possibly via delegate) in order to update a parameter's value.
   * It will update mParams[paramIdx], call InformHostOfParamChange and IPlugAPIBase::OnParamChange();
   * @param paramIdx The index of the parameter that changed
   * @param normalizedValue The new (normalised) value */
  void SetParameterValue(int paramIdx, double normalizedValue);
  
  /** Get the color of the track that the plug-in is inserted on */
  virtual void GetTrackColor(int& r, int& g, int& b) {};

  /** Get the name of the track that the plug-in is inserted on */
  virtual void GetTrackName(WDL_String& str) {};
  
  /** /todo */
  virtual void DirtyParametersFromUI() override;

#pragma mark - Methods called by the API class - you do not call these methods in your plug-in class

  /** This is called from the plug-in API class in order to update UI controls linked to plug-in parameters, prior to calling OnParamChange()
   * NOTE: It may be called on the high priority audio thread. Its purpose is to place parameter changes in a queue to defer to main thread for the UI
   * @param paramIdx The index of the parameter that changed
   * @param value The new value
   * @param normalized /true if value is normalised */
  virtual void SendParameterValueFromAPI(int paramIdx, double value, bool normalized);

  /** Called to set the name of the current host, if known (calls on to HostSpecificInit() and OnHostIdentified()).
  * @param host The name of the plug-in host
  * @param version The version of the plug-in host where version in hex = 0xVVVVRRMM */
  void SetHost(const char* host, int version);

  /** This method is implemented in some API classes, in order to do specific initialisation for particular problematic hosts.
   * This is not the same as OnHostIdentified(), which you may implement in your plug-in class to do your own specific initialisation after a host has been identified */
  virtual void HostSpecificInit() {}

  //IEditorDelegate
  void BeginInformHostOfParamChangeFromUI(int paramIdx) override { BeginInformHostOfParamChange(paramIdx); }
  
  void EndInformHostOfParamChangeFromUI(int paramIdx) override { EndInformHostOfParamChange(paramIdx); }
  
  bool EditorResizeFromUI(int viewWidth, int viewHeight) override { return EditorResizeFromDelegate(viewWidth, viewHeight); }
    
  void EditorDataChangedFromUI(const IByteChunk& data) override { EditorDataChangedFromDelegate(data); }
  
  void SendParameterValueFromUI(int paramIdx, double normalisedValue) override
  {
    SetParameterValue(paramIdx, normalisedValue);
    IPluginBase::SendParameterValueFromUI(paramIdx, normalisedValue);
  }
  
  //These are handled in IPlugAPIBase for non DISTRIBUTED APIs
  void SendMidiMsgFromUI(const IMidiMsg& msg) override;
  
  void SendSysexMsgFromUI(const ISysEx& msg) override;
  
  void SendArbitraryMsgFromUI(int msgTag, int ctrlTag = kNoTag, int dataSize = 0, const void* pData = nullptr) override;
  
  void DeferMidiMsg(const IMidiMsg& msg) override { mMidiMsgsFromEditor.Push(msg); }
  
  void DeferSysexMsg(const ISysEx& msg) override
  {
    SysExData data(msg.mOffset, msg.mSize, msg.mData); // copies data
    mSysExDataFromEditor.Push(data);
  }

  /** /todo */
  void CreateTimer();
  
private:
  /** Implemented by the API class, called by the UI via SetParameterValue() with the value of a parameter change gesture
   * @param paramIdx The parameter that is being changed
   * @param normalizedValue The new normalised value of the parameter being changed */
  virtual void InformHostOfParamChange(int paramIdx, double normalizedValue) {};
  
  //DISTRIBUTED ONLY (Currently only VST3)
  /** /todo */
  virtual void TransmitMidiMsgFromProcessor(const IMidiMsg& msg) {};
  
  /** /todo */
  virtual void TransmitSysExDataFromProcessor(const SysExData& data) {};

  void OnTimer(Timer& t);

protected:
  WDL_String mParamDisplayStr;
  std::unique_ptr<Timer> mTimer;
  
  IPlugQueue<ParamTuple> mParamChangeFromProcessor {PARAM_TRANSFER_SIZE};
  IPlugQueue<IMidiMsg> mMidiMsgsFromEditor {MIDI_TRANSFER_SIZE}; // a queue of midi messages generated in the editor by clicking keyboard UI etc
  IPlugQueue<IMidiMsg> mMidiMsgsFromProcessor {MIDI_TRANSFER_SIZE}; // a queue of MIDI messages received (potentially on the high priority thread), by the processor to send to the editor
  IPlugQueue<SysExData> mSysExDataFromEditor {SYSEX_TRANSFER_SIZE}; // a queue of SYSEX data to send to the processor
  IPlugQueue<SysExData> mSysExDataFromProcessor {SYSEX_TRANSFER_SIZE}; // a queue of SYSEX data to send to the editor
  SysExData mSysexBuf;
};

END_IPLUG_NAMESPACE
