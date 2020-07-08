/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc IPluginBase
 */

#include "IPlugDelegate_select.h"
#include "IPlugParameter.h"
#include "IPlugStructs.h"
#include "IPlugLogger.h"

BEGIN_IPLUG_NAMESPACE

/** Base class that contains plug-in info and state manipulation methods */
class IPluginBase : public EDITOR_DELEGATE_CLASS
{
public:
  IPluginBase(int nParams, int nPresets);
  virtual ~IPluginBase();
  
  IPluginBase(const IPluginBase&) = delete;
  IPluginBase& operator=(const IPluginBase&) = delete;

#pragma mark - Plug-in properties
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
  
  /** @return The host if it has been identified, see EHost enum for a list of possible hosts */
   EHost GetHost() const { return mHost; }
  
  /** Get the host name as a CString
   * @param str string into which to write the host name */
  void GetHostStr(WDL_String& str) const { GetHostNameStr(GetHost(), str); }
  
  /** Get the host version number as an integer
   * @param decimal \c true indicates decimal format = VVVVRRMM, otherwise hexadecimal 0xVVVVRRMM.
   * @return The host version number as an integer. */
  int GetHostVersion(bool decimal) const;
  
  /** Get the host version number as a string
   * @param str string into which to write the host version */
  void GetHostVersionStr(WDL_String& str) const;
  
  /** @return The The plug-in API, see EAPI enum for a list of possible APIs */
  EAPI GetAPI() const { return mAPI; }
  
  /** @return  Returns a CString describing the plug-in API, e.g. "VST2" */
  const char* GetAPIStr() const;
  
  /** @return  Returns a CString either "x86" or "x64" or "WASM" describing the binary architecture */
  const char* GetArchStr() const;
  
  /** @brief Used to get the build date of the plug-in and architecture/api details in one string
   * @note since the implementation is in IPlugAPIBase.cpp, you may want to touch that file as part of your build script to force recompilation
   * @param str WDL_String will be set with the Plugin name, architecture, api, build date, build time*/
  void GetBuildInfoStr(WDL_String& str) const;
  
  /** @return \c true if the plug-in is meant to have a UI, as defined in config.h */
  bool HasUI() const { return mHasUI; }
  
  /** @return \c true if the plug-in allows reszing via the host's window chrome, as defined in config.h */
  bool GetHostResizeEnabled() const { return mHostResize; }
  
  /*** @return a CString with the bundle identifier (macOS/IOS only) */
  const char* GetBundleID() const { return mBundleID.Get(); }
    
#pragma mark - Parameters
  
  /** @return The number of unique parameter groups identified */
  int NParamGroups() const { return mParamGroups.GetSize(); }
  
  /** Called to add a parameter group name, when a unique group name is discovered
   * @param name CString for the unique group name
   * @return Number of parameter groups */
  int AddParamGroup(const char* name) { mParamGroups.Add(name); return NParamGroups(); }
  
  /** Get the parameter group name as a particular index
   * @param idx The index to return
   * @return CString for the unique group name */
  const char* GetParamGroupName(int idx) const { return mParamGroups.Get(idx); }
  
  /** Implemented by the API class, call this if you update parameter labels and hopefully the host should update it's displays (not applicable to all APIs) */
  virtual void InformHostOfParameterDetailsChange() {};
    
#pragma mark - State Serialization
  /** @return \c true if the plug-in has been set up to do state chunks, via config.h */
  bool DoesStateChunks() const { return mStateChunks; }
  
  /** Serializes the current double precision floating point, non-normalised values (IParam::mValue) of all parameters, into a binary byte chunk.
   * @param chunk The output chunk to serialize to. Will append data if the chunk has already been started.
   * @return \c true if the serialization was successful */
  bool SerializeParams(IByteChunk& chunk) const;
  
  /** Unserializes double precision floating point, non-normalised values from a byte chunk into mParams.
   * @param chunk The incoming chunk where parameter values are stored to unserialize
   * @param startPos The start position in the chunk where parameter values are stored
   * @return The new chunk position (endPos) */
  int UnserializeParams(const IByteChunk& chunk, int startPos);
    
  /** Override this method to serialize custom state data, if your plugin does state chunks.
   * @param chunk The output bytechunk where data can be serialized
   * @return \c true if serialization was successful*/
  virtual bool SerializeState(IByteChunk& chunk) const { TRACE return SerializeParams(chunk); }
  
  /** Override this method to unserialize custom state data, if your plugin does state chunks.
   * Implementations should call UnserializeParams() after custom data is unserialized
   * @param chunk The incoming chunk containing the state data.
   * @param startPos The position in the chunk where the data starts
   * @return The new chunk position (endPos)*/
  virtual int UnserializeState(const IByteChunk& chunk, int startPos) { TRACE return UnserializeParams(chunk, startPos); }
  
  /** VST3 ONLY! - THIS IS ONLY INCLUDED FOR COMPATIBILITY - NOONE ELSE SHOULD NEED IT!
   * @param chunk The output bytechunk where data can be serialized.
   * @return \c true if serialization was successful */
  virtual bool SerializeVST3CtrlrState(IByteChunk& chunk) const { return true; }
  
  /** VST3 ONLY! - THIS IS ONLY INCLUDED FOR COMPATIBILITY - NOONE ELSE SHOULD NEED IT!
   * @param chunk chunk The incoming chunk containing the state data.
   * @return The new chunk position (endPos) */
  virtual int UnserializeVST3CtrlrState(const IByteChunk& chunk, int startPos) { return startPos; }
  
  /** Get the index of the current, active preset
   * @return The index of the current preset */
  int GetCurrentPresetIdx() const { return mCurrentPresetIdx; }
  
  /** Set the index of the current, active preset
   * @param idx The index of the current preset */
  void SetCurrentPresetIdx(int idx) { assert(idx > -1 && idx < NPresets()); mCurrentPresetIdx = idx; }
  
  /** Implemented by the API class, called by the UI (etc) when the plug-in initiates a program/preset change (not applicable to all APIs) */
  virtual void InformHostOfPresetChange() {};

#pragma mark - Preset Manipulation
  
  /** Get a ptr to a factory preset
   * @ param idx The index number of the preset you are referring to */
  IPreset* GetPreset(int idx) { return mPresets.Get(idx); }
  
  /** This method should update the current preset with current values
   * NOTE: This is only relevant for VST2 plug-ins, which is the only format to have the notion of banks?
   * @param name CString name of the modified preset */
  void ModifyCurrentPreset(const char* name = 0);

  /** Gets the number of factory presets. NOTE: some hosts don't like 0 presets, so even if you don't support factory presets, this method should return 1
   * @return The number of factory presets */
  int NPresets() const { return mPresets.GetSize(); }

  /** Restore a preset by index. This should also update mCurrentPresetIdx
   * @param idx The index of the preset to restore
   * @return \c true on success */
  bool RestorePreset(int idx);

  /** Restore a preset by name
   * @param CString name of the preset to restore
   * @return \c true on success */
  bool RestorePreset(const char* name);

  /** Get the name a preset
   * @param idx The index of the preset whose name to get
   * @return CString preset name */
  const char* GetPresetName(int idx) const;
  
  /** Copy source preset to preset at index
  * @param pSrc source preset
  * @param destIdx index of internal destination preset */
  void CopyPreset(IPreset* pSrc, int destIdx, bool copyname = false)
  {
    IPreset* pDst = mPresets.Get(destIdx);

    pDst->mChunk.Clear();
    pDst->mChunk.PutChunk(&pSrc->mChunk);
    pDst->mInitialized = true;
    strncpy(pDst->mName, pSrc->mName, MAX_PRESET_NAME_LEN - 1);
  }
  
  /** /todo 
   * @param name /todo
   * @param nPresets /todo */
  void MakeDefaultPreset(const char* name = 0, int nPresets = 1);

  /** This method can be used from Plugin class to create a baked-in factory preset
   * usage: MakePreset(name, param1, param2, ..., paramN)
   * @param name The preset name
   * @param ... The list of parameter values, ordered acording to paramIdx */
  void MakePreset(const char* name, ...);

  /** /todo
   * MakePresetFromNamedParams(name, nParamsNamed, paramEnum1, paramVal1, paramEnum2, paramVal2, ..., paramEnumN, paramVal2)
   * nParamsNamed may be less than the total number of params.
   * @param name /todo
   * @param nParamsNamed /todo
   * @param ... /todo  */
  void MakePresetFromNamedParams(const char* name, int nParamsNamed, ...);
  
  /** This method is primary called by chunk based plugins
   * @param name The preset name
   * @param chunk The block of memory to use */
  void MakePresetFromChunk(const char* name, IByteChunk& chunk);

  /** This method makes a preset from a Binary large object (blob)
   * @param name The preset name
   * @param blob The binary string
   * @param sizeOfChunk The binary string size */
  void MakePresetFromBlob(const char* name, const char* blob, int sizeOfChunk);
  
  /** /todo */
  void PruneUninitializedPresets();
  
  /** [VST2 only] /todo *  */
  virtual void OnPresetsModified() {}

  /** [VST2 only] /todo *  */
  void EnsureDefaultPreset();

  /** [VST2 only] /todo *  
   * @param chunk /todo
   * @return true /todo
   * @return false /todo */
  bool SerializePresets(IByteChunk& chunk) const;

  /** [VST2 only] /todo * 
   * @param chunk /todo
   * @param startPos /todo
   * @return int /todo */
  int UnserializePresets(IByteChunk& chunk, int startPos); // Returns the new chunk position (endPos).
  
  /** Writes a call to MakePreset() for the current preset to a new text file
   * @param file The name of the file to write or overwrite. */
  void DumpMakePresetSrc(const char* file) const;

  /** Writes a call to MakePresetFromNamedParams() for the current preset to a new text file
   * @param file The name of the file to write.
   * @param paramEnumNames A list of all parameter names. e.g. const char* pParamNames[] = {"kParam1", "kParam2", "kParam3"}; */
  void DumpMakePresetFromNamedParamsSrc(const char* file, const char* paramEnumNames[]) const;

  /** Writes a call to MakePresetFromBlob() for the current preset to a new text file
   * @param file The name of the file to write or overwrite. */
  void DumpPresetBlob(const char* file) const;

  /** Writes a call to MakePresetFromBlob() for all presets to a new text file
   * Note: Set PLUG_DOES_STATE_CHUNKS to 1 to load blob presets.
   * @param filename The name of the file to write.*/
  void DumpAllPresetsBlob(const char* filename) const;

  /** Writes a call to MakePresetFromBlob() for all presets to a new text file
   * Note: Set PLUG_DOES_STATE_CHUNKS to 1 to load blob presets.
   * @param file The name of the file to write. */
  void DumpBankBlob(const char* file) const;
  
  /** Save current state as a VST2 format preset
   * @param file /todo
   * @return true /todo */
  bool SavePresetAsFXP(const char* file) const;

  /** Save current bank as a VST2 format bank [VST2 only]
   * @param file /todo
   * @return true /todo */
  bool SaveBankAsFXB(const char* file) const;

  /** Load VST2 format preset 
   * @param file /todo
   * @return true /todo */
  bool LoadPresetFromFXP(const char* file);

  /** Load VST2 format bank [VST2 only]
   * @param file /todo
   * @return true /todo
   * @return false /todo */
  bool LoadBankFromFXB(const char* file);

  /** Save current bank as individual VST2 format presets [VST2 only]
   * @param path /todo
   * @return true /todo
   * @return false /todo */
  bool SaveBankAsFXPs(const char* path) const { return false; }
  
  /** /todo 
   * @param chunk /todo
   * @param componentState /todo
   * @param controllerState /todo */
  void MakeVSTPresetChunk(IByteChunk& chunk, IByteChunk& componentState, IByteChunk& controllerState) const;

  /** /todo 
   * @param file /todo
   * @return true /todo */
  bool SavePresetAsVSTPreset(const char* file) const;

  /** /todo 
   * @param file /todo
   * @return true /todo*/
  bool LoadPresetFromVSTPreset(const char* file);

  /** /todo 
   * @param path /todo
   * @return true */
  bool SaveBankAsVSTPresets(const char* path) { return false; }
  
  /** /todo 
   * @param name /todo
   * @param file /todo
   * @return true /todo  */
  bool SavePresetAsAUPreset(const char* name, const char* file) const { return false; }

  /** /todo 
   * @param file /todo
   * @return true /todo  */
  bool LoadPresetFromAUPreset(const char* file) { return false; }

  /** /todo 
   * @param path /todo
   * @return true /todo */
  bool SaveBankAsAUPresets(const char* path) { return false; }
  
  /** /todo 
   * @param presetName /todo
   * @param file /todo
   * @param pluginID /todo
   * @return true /todo */
  bool SavePresetAsProToolsPreset(const char* presetName, const char* file, unsigned long pluginID) const { return false; }

  /** /todo 
   * @param file /todo
   * @return true /todo */
  bool LoadPresetFromProToolsPreset(const char* file) { return false; }

  /** /todo 
   * @param path /todo
   * @param pluginID /todo
   * @return true /todo
   * @return false /todo */
  bool SaveBankAsProToolsPresets(const char* path, unsigned long pluginID) { return false; }
  
#pragma mark - Parameter manipulation
    
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
  void InitParamRange(int startIdx, int endIdx, int countStart, const char* nameFmtStr, double defaultVal, double minVal, double maxVal, double step, const char* label = "", int flags = 0, const char* group = "", const IParam::Shape& shape = IParam::ShapeLinear(), IParam::EParamUnit unit = IParam::kUnitCustom, IParam::DisplayFunc displayFunc = nullptr);
  
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
  void ForParamInRange(int startIdx, int endIdx, std::function<void(int paramIdx, IParam& param)> func);
  
  /** Modify a parameter group simulataneously
   * @param paramGroup The name of the group to modify
   * @param func A lambda function to modify the parameter. Ideas: you could randomise the parameter value or reset to default*/
  void ForParamInGroup(const char* paramGroup, std::function<void(int paramIdx, IParam& param)> func);
  
  /** Copy a range of parameter values
   * @param startIdx The index of the first parameter value to copy
   * @param destIdx The index of the first destination parameter
   * @param nParams The number of parameters to copy */
  void CopyParamValues(int startIdx, int destIdx, int nParams);
  
  /** Copy a range of parameter values for a parameter group
   * @param inGroup The name of the group to copy from
   * @param outGroup The name of the group to copy to */
  void CopyParamValues(const char* inGroup, const char* outGroup);
  
  /** Randomise all parameters */
  void RandomiseParamValues();
  
  /** Randomise parameter values within a range. NOTE for more flexibility in terms of RNG etc, use ForParamInRange()
   * @param startIdx The index of the first parameter to modify
   * @param endIdx The index of the last parameter to modify */
  void RandomiseParamValues(int startIdx, int endIdx);
  
  /** Randomise parameter values for a parameter group
   * @param paramGroup The name of the group to modify */
  void RandomiseParamValues(const char* paramGroup);
  
  /** Set all parameters to their default values */
  void DefaultParamValues();

  /** Default parameter values within a range.
   * @param startIdx The index of the first parameter to modify
   * @param endIdx The index of the last parameter to modify */
  void DefaultParamValues(int startIdx, int endIdx);
  
  /** Default parameter values for a parameter group
   * @param paramGroup The name of the group to modify */
  void DefaultParamValues(const char* paramGroup);
  
  /** Default parameter values for a parameter group  */
  void PrintParamValues();

protected:
  int mCurrentPresetIdx = 0;
  /** \c true if the plug-in does opaque state chunks. If false the host will provide a default interface */
  bool mStateChunks = false;
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
  /** macOS/iOS bundle ID */
  WDL_String mBundleID;
  /** Saving VST3 format presets requires this see SavePresetAsVSTPreset */
  WDL_String mVST3ProductCategory;
  /** Saving VST3 format presets requires this see SavePresetAsVSTPreset */
  WDL_String mVST3ProcessorUIDStr;
  /** Saving VST3 format presets requires this see SavePresetAsVSTPreset */
  WDL_String mVST3ControllerUIDStr;
  /** \c true if the plug-in has a user interface. If false the host will provide a default interface */
  bool mHasUI = false;
  /** \c true if the host window chrome should be able to resize the plug-in UI, only applicable in certain formats/hosts */
  bool mHostResize = false;
  /** A list of unique cstrings found specified as "parameter groups" when defining IParams. These are used in various APIs to group parameters together in automation dialogues. */
  WDL_PtrList<const char> mParamGroups;
  /** "Baked in" Factory presets */
  WDL_PtrList<IPreset> mPresets;

#ifdef PARAMS_MUTEX
  friend class IPlugVST3ProcessorBase;
protected:
  /** Lock when accessing mParams (including via GetParam) from the audio thread */
  WDL_Mutex mParams_mutex;
#endif  
};

END_IPLUG_NAMESPACE
