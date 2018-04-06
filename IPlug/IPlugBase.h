#pragma once

#include <cstring>
#include <cstdint>

#include "ptrlist.h"
#include "mutex.h"

#include "IPlugPlatform.h"
#include "IPlugConstants.h"
#include "IPlugStructs.h"
#include "IPlugUtilities.h"
#include "IPlugParameter.h"

/**
 * @file
 * @copydoc IPlugBase
 * @defgroup APIClasses IPlug::APIClasses
*/

struct IPlugConfig;

/** The lowest level base class of an IPlug plug-in. No UI framework code included.  This interface does not handle audio processing, see @IPlugProcessor  */
class IPlugBase
{

public:
  IPlugBase(IPlugConfig config, EAPI plugAPI);
  virtual ~IPlugBase();

#pragma mark - Methods you can implement/override in your plug-in class - you do not call these methods

  /** Override this method to do something when a parameter changes.
   * THIS METHOD **CAN BE** CALLED BY THE HIGH PRIORITY AUDIO THREAD
   * @param paramIdx The index of the parameter that changed
   * @param source One of the EParamSource options to indicate where the parameter change came from. */
  virtual void OnParamChange(int paramIdx, EParamSource source);

  /** Another version of the OnParamChange method without an EParamSource, for backwards compatibility / simplicity. */
  virtual void OnParamChange(int paramIdx) {}

  /** Override this method to serialize custom state data, if your plugin does state chunks.
   * @param chunk The output bytechunk where data can be serialized
   * @return \c true if serialization was successful*/
  virtual bool SerializeState(IByteChunk& chunk) { TRACE; return SerializeParams(chunk); }

  /** Override this method to unserialize custom state data, if your plugin does state chunks.
   * Implementations should call UnserializeParams() after custom data is unserialized
   * @param chunk The incoming chunk containing the state data.
   * @param startPos The position in the chunk where the data starts
   * @return The new chunk position (endPos)*/
  virtual int UnserializeState(const IByteChunk& chunk, int startPos) { TRACE; return UnserializeParams(chunk, startPos); }

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

  /** Override this method when not using IGraphics in order to return a platform view handle e.g. NSView, UIView, HWND */
  virtual void* OpenWindow(void* pHandle) { return nullptr; }

  /** Override this method when not using IGraphics if you need to free resources etc when the window closes */
  virtual void CloseWindow() {}

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

  /** This is called by API classes after restoring stateand by IPlugPresetHandler::RestorePreset(). Typically used to update user interface, where parameter values have changed. */
  virtual void OnRestoreState() {};

#pragma mark - Methods you can call - some of which have custom implementations in the API classes, some implemented in IPlugBase.cpp;
  /** Helper method, used to print some info to the console in debug builds. Can be overridden in other IPlugBases, for specific functionality, such as printing UI details. */
  virtual void PrintDebugInfo() const;

  /** @return Returns the number of parameters that belong to the plug-in. */
  int NParams() const { return mParams.GetSize(); }

  /** Get a pointer to one of the plug-ins IParam objects
   ** WARNING: The IPlugBase::mParams is accessed by multiple threads, and this method has no thread safety mechanism.
   * @param paramIdx The index of the parameter object to be got
   * @return A pointer to the IParam object at paramIdx */
  IParam* GetParam(int paramIdx) { return mParams.Get(paramIdx); }
  
  /** @return the name of the plug-in as a CString */
  const char* GetPluginName() const { return mPluginName.Get(); }

  /** Get the plug-in version number
   * @param decimal Sets the output format
   * @return Effect version in VVVVRRMM (if \p decimal is \c true) or Hexadecimal 0xVVVVRRMM (if \p decimal is \c false) format */
  int GetPluginVersion(bool decimal) const;

  /** Gets the plug-in version as a string
   * @param str WDL_String to write to
   * The output format is vX.M.m, where X - version, M - major, m - minor
   * @note If \c _DEBUG is defined, \c D is appended to the version string
   * @note If \c TRACER_BUILD is defined, \c T is appended to the version string*/
  void GetPluginVersionStr(WDL_String& str) const;

  /** Get the manufacturer name as a CString */
  const char* GetMfrName() const { return mMfrName.Get(); }

  /** Get the product name as a CString. A shipping product may contain multiple plug-ins, hence this. Not used in all APIs */
  const char* GetProductName() const { return mProductName.Get(); }

  /** @return The plug-in's unique four character ID as an integer */
  int GetUniqueID() const { return mUniqueID; }

  /** @return The plug-in manufacturer's unique four character ID as an integer */
  int GetMfrID() const { return mMfrID; }

  /** @return The host if it has been identified, see EHost enum for a list of possible hosts, implemented in the API class for VST2 and AUv2 */
  virtual EHost GetHost() { return mHost; }

  /** Get the host version number as an integer
   * @param decimal \c true indicates decimal format = VVVVRRMM, otherwise hexadecimal 0xVVVVRRMM.
   * @return The host version number as an integer. */
  int GetHostVersion(bool decimal); //

  /** Get the host version number as a string
   * @param str string into which to write the host version */
  void GetHostVersionStr(WDL_String& str);

  /** @return The The plug-in API, see EAPI enum for a list of possible APIs */
  EAPI GetAPI() const { return mAPI; }

  /** @return  Returns a CString describing the plug-in API, e.g. "VST2" */
  const char* GetAPIStr() const;

  /** @return  Returns a CString either "x86" or "x64" describing the binary architecture */
  const char* GetArchStr() const;

  /** @brief Used to get the build date of the plug-in and architecture/api details in one string
   * @note since the implementation is in IPlugBase.cpp, you may want to touch that file as part of your build script to force recompilation
   * @param str WDL_String will be set with the Plugin name, architecture, api, build date, build time*/
  void GetBuildInfoStr(WDL_String& str) const;

  /** @return \c true if the plug-in has been set up to do state chunks, via config.h */
  bool DoesStateChunks() const { return mStateChunks; }

  /** @return \c true if the plug-in is meant to have a UI, as defined in config.h */
  bool HasUI() const { return mHasUI; }

  /** @return The default width of the plug-in UI in pixels, if defined in config.h */
  int Width() const { return mWidth; }

  /** @return The default height of the plug-in UI in pixels, if defined in config.h */
  int Height() const { return mHeight; }

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

  /** Implemented by the API class, called by the UI (etc) when the plug-in initiates a program/preset change (not applicable to all APIs) */
  virtual void InformHostOfProgramChange() {};
  
#pragma mark - Parameter methods
  
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
  
#pragma mark - Methods called by the API class - you do not call these methods in your plug-in class

  /** This is called from the plug-in API class in order to update UI controls linked to plug-in parameters, prior to calling OnParamChange()
   * NOTE: It may be called on the high priority audio thread
   * @param paramIdx The index of the parameter that changed
   * @param value The new value
   * @param normalized /true if @param value is normalised */
  virtual void SendParameterValueToUIFromAPI(int paramIdx, double value, bool normalized) {};

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

  void InitChunkWithIPlugVer(IByteChunk& chunk);

  /** Helper method to retrieve the IPlug version number from the beginning of the byte chunk
   * @param chunk The incoming byte chunk that contains the version number
   * @param pos The position (in bytes) to start looking
   * @return The IPlug version number, retrieved from the chunk, or 0 if it failed */
  int GetIPlugVerFromChunk(const IByteChunk& chunk, int& pos);

  /** Called to set the name of the current host, if known.
  * @param host The name of the plug-in host
  * @param version The version of the plug-in host where version in hex = 0xVVVVRRMM */
  void SetHost(const char* host, int version);

  /** This method is called by some API classes, in order to do specific initialisation for particular problematic hosts.
   * This is not the same as OnHostIdentified(), which you may implement in your plug-in class to do your own specific initialisation after a host has been identified */
  virtual void HostSpecificInit() {}; //TODO: sort this method out, it's called differently from different APIs

  /** Serializes the current double precision floating point, non-normalised values (IParam::mValue) of all parameters, into a binary byte chunk.
   * @param chunk The output chunk to serialize to. Will append data if the chunk has already been started.
   * @return \c true if the serialization was successful */
  bool SerializeParams(IByteChunk& chunk);

  /** Unserializes double precision floating point, non-normalised values from a byte chunk into mParams.
   * @param chunk The incoming chunk where parameter values are stored to unserialize
   * @param startPos The start position in the chunk where parameter values are stored
   * @return The new chunk position (endPos) */
  int UnserializeParams(const IByteChunk& chunk, int startPos);

  /** Calls OnParamChange() for each parameter and finally OnReset().
   * @param source Specifies the source of this parameter change */
  void OnParamReset(EParamSource source);  //

private:
  /** Implemented by the API class, called by the UI via SetParameterValue() with the value of a parameter change gesture
   * @param paramIdx The parameter that is being changed
   * @param normalizedValue The new normalised value of the parameter being changed */
  virtual void InformHostOfParamChange(int paramIdx, double normalizedValue) = 0;
protected:
  /** The name of this plug-in */
  WDL_String mPluginName;
  /** Product name: if the plug-in is part of collection of plug-ins it might be one product */
  WDL_String mProductName;
  /** Plug-in Manufacturer name */
  WDL_String mMfrName;
  /* Plug-in unique four char ID as an int */
  int mUniqueID;
  /* Manufacturer unique four char ID as an int */
  int mMfrID;
  /** Plug-in version number stored as 0xVVVVRRMM: V = version, R = revision, M = minor revision */
  int mVersion;
  /** Host version number stored as 0xVVVVRRMM: V = version, R = revision, M = minor revision */
  int mHostVersion = 0;
  /** Host that has been identified, see EHost enum */
  EHost mHost = kHostUninit;
  /** API of this instance */
  EAPI mAPI;
  /** \c true if the plug-in does opaque state chunks. If false the host will provide a default interface */
  bool mStateChunks = false;
  /** \c true if the plug-in has a user interface. If false the host will provide a default interface */
  bool mHasUI = false;
  /** The default width of the plug-in UI if it has an interface. */
  int mWidth = 0;
  /** The default height of the plug-in UI if it has an interface. */
  int mHeight = 0;
  /** A list of unique cstrings found specified as "parameter groups" when defining IParams. These are used in various APIs to group parameters together in automation dialogues. */
  WDL_PtrList<const char> mParamGroups;
  /** A list of IParam objects. This list is resized in the IPlugBase constructor depending on the number of parameters passed as an argument to IPLUG_CTOR in the plugin class implementation constructor */
  WDL_PtrList<IParam> mParams;
public:
  /** Lock when accessing mParams (including via GetParam) from the audio thread */
  WDL_Mutex mParams_mutex;
  WDL_String mParamDisplayStr;
};
