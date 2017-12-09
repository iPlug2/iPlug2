#pragma once

#include <cstring>
#include "ptrlist.h"
#include "mutex.h"

#include "IPlugConstants.h"
#include "IPlugStructs.h"
#include "IPlugByteChunk.h"
#include "IParam.h"
#include "Hosts.h"
#include "Log.h"
#include "NChanDelay.h"

class IPlugBase
{
public:
  // Use IPLUG_CTOR instead of calling directly (defined in IPlug_include_in_plug_src.h).
  IPlugBase(int nParams,
            const char* channelIOStr,
            int nPresets,
            const char* effectName,
            const char* productName,
            const char* mfrName,
            int vendorVersion,
            int uniqueID,
            int mfrID,
            int latency,
            bool plugDoesMidi,
            bool plugDoesChunks,
            bool plugIsInst,
            EAPI plugAPI);

  // ----------------------------------------
  // Your plugin class implements these.
  // There are default impls, mostly just for reference.

  virtual ~IPlugBase();

  virtual void Reset() { TRACE; }
  virtual void OnParamChange(int paramIdx) {}

  // Default passthrough.  Inputs and outputs are [nChannel][nSample].
  virtual void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);
  
  // In case the audio processing thread needs to do anything when the GUI opens
  // (like for example, set some state dependent initial values for controls).
  virtual void OnGUIOpen() { TRACE; }
  virtual void OnGUIClose() { TRACE; }
  
  virtual void* OpenWindow(void* handle) { return nullptr; }
  virtual void* OpenWindow(void* handle, void* control) { return nullptr; }
  virtual void CloseWindow() {} // plugin api asking to close window
  
  // This is an idle call from the audio processing thread
  // Only active if USE_IDLE_CALLS is defined
  virtual void OnIdle() {}

  // Not usually needed ... Reset is called on activate regardless of whether this is implemented.
  // Also different hosts have different interpretations of "activate".
  virtual void OnActivate(bool active) { TRACE; }

  virtual void ProcessMidiMsg(IMidiMsg& msg);
  virtual void ProcessSysEx(ISysEx& msg) {}

  virtual bool MidiNoteName(int noteNumber, char* pNameStr) { *pNameStr = '\0'; return false; }

  virtual bool SerializeState(ByteChunk& chunk) { TRACE; return SerializeParams(chunk); }
  // Return the new chunk position (endPos). Implementations should call UnserializeParams() after custom data is unserialized
  virtual int UnserializeState(ByteChunk& chunk, int startPos) { TRACE; return UnserializeParams(chunk, startPos); }
  
  // Only used by AAX, override in plugins that do chunks
  virtual bool CompareState(const unsigned char* incomingState, int startPos);
  
  virtual void OnWindowResize() {}
  // implement this and return true to trigger your custom about box, when someone clicks about in the menu of a standalone
  virtual bool OnHostRequestingAboutBox() { return false; }

  // implement this to do something specific when IPlug is aware of the host
  // may get called multiple times
  virtual void OnHostIdentified() {}

  virtual void PopupHostContextMenuForParam(int paramIdx, int x, int y) { }; // only for VST3, call it from the GUI

  // ----------------------------------------
  // Your plugin class, or a control class, can call these functions.

  int NParams() const { return mParams.GetSize(); }
  IParam* GetParam(int paramIdx) { return mParams.Get(paramIdx); }

  const char* GetEffectName() const { return mEffectName; }
  int GetEffectVersion(bool decimal) const;   // Decimal = VVVVRRMM, otherwise 0xVVVVRRMM.
  void GetEffectVersionStr(char* str) const;
  const char* GetMfrName() const { return mMfrName; }
  const char* GetProductName() const { return mProductName; }
  int GetUniqueID() const { return mUniqueID; }
  int GetMfrID() const { return mMfrID; }

  //
  
  virtual void SetParameterInUIFromAPI(int paramIdx, double value, bool normalized) {}; // call from plugin API class to update GUI prior to calling OnParamChange();
  virtual void SetParameterFromGUI(int idx, double normalizedValue); // called from GUI to update 
  // If a parameter change comes from the GUI, midi, or external input,
  // the host needs to be informed in case the changes are being automated.
  virtual void BeginInformHostOfParamChange(int idx) = 0;
  virtual void InformHostOfParamChange(int idx, double normalizedValue) = 0;
  virtual void EndInformHostOfParamChange(int idx) = 0;
  virtual void InformHostOfProgramChange() = 0;
  
  // ----------------------------------------
  // Useful stuff for your plugin class or an outsider to call,
  // most of which is implemented by the API class.

  double GetSampleRate() { return mSampleRate; }
  int GetBlockSize() const { return mBlockSize; }
  int GetLatency() const { return mLatency; }
  bool GetIsBypassed() const { return mIsBypassed; }

  // In ProcessDoubleReplacing you are always guaranteed to get valid pointers
  // to all the channels the plugin requested.  If the host hasn't connected all the pins,
  // the unconnected channels will be full of zeros.
  int NInChannels() const { return mInChannels.GetSize(); }
  int NOutChannels() const { return mOutChannels.GetSize(); }
  bool IsInChannelConnected(int chIdx) const;
  bool IsOutChannelConnected(int chIdx) const;
  int GetNumInputsConnected() const;
  int GetNumOutputsConnected() const;
  
  virtual bool IsRenderingOffline() { return false; };
  virtual int GetSamplePos() = 0;   // Samples since start of project.
  virtual double GetTempo() = 0;
  double GetSamplesPerBeat();
  virtual void GetTimeSig(int& numerator, int& denominator) = 0;
  virtual void GetTime(ITimeInfo& timeInfo) = 0;
  virtual EHost GetHost() { return mHost; }
  virtual EAPI GetAPI() { return mAPI; }
  const char* GetAPIString();
  int GetHostVersion(bool decimal); // Decimal = VVVVRRMM, otherwise 0xVVVVRRMM.
  void GetHostVersionStr(char* str);
  const char* GetArchString();
  
  int GetTailSize() { return mTailSize; }
  
  virtual bool GetHasUI() { return mHasUI; }
  virtual int GetUIWidth() { return 0; }
  virtual int GetUIHeight() { return 0; }
  
  // implement in API class to do something once editor is created/attached (called from IPlugBaseGraphics AttachGraphics)
  virtual void OnGUICreated() {};

  // Tell the host that the graphics resized.
  virtual void ResizeGraphics(int w, int h) = 0;

  void EnsureDefaultPreset();

protected:
  // ----------------------------------------
  // Useful stuff for your plugin class to call, implemented here or in the API class, or partly in both.

  // for labelling individual inputs/outputs (VST2)
  void SetInputLabel(int idx, const char* label);
  void SetOutputLabel(int idx, const char* label);

  const WDL_String* GetInputLabel(int idx) { return &(mInChannels.Get(idx)->mLabel); }
  const WDL_String* GetOutputLabel(int idx) { return &(mOutChannels.Get(idx)->mLabel); }

  // for labelling bus inputs/outputs (AU/VST3)
  void SetInputBusLabel(int idx, const char* pLabel);
  void SetOutputBusLabel(int idx, const char* pLabel);

  const WDL_String* GetInputBusLabel(int idx) { return mInputBusLabels.Get(idx); }
  const WDL_String* GetOutputBusLabel(int idx) { return mOutputBusLabels.Get(idx); }

  bool LegalIO(int nIn, int nOut);    // -1 for either means check the other value only.
  void LimitToStereoIO();

  void InitChunkWithIPlugVer(ByteChunk& chunk);
  int GetIPlugVerFromChunk(ByteChunk& chunk, int* pPos);

  void SetHost(const char* host, int version);   // Version = 0xVVVVRRMM.
  virtual void HostSpecificInit() {};

  // If latency changes after initialization (often not supported by the host).
  virtual void SetLatency(int samples);
  
  // set to 0xffffffff for infinite tail (VST3), or 0 for none (default)
  // for VST2 setting to 1 means no tail, but it would be better i think to leave it at 0, the default
  void SetTailSize(unsigned int tailSizeSamples) { mTailSize = tailSizeSamples; }
  
  virtual bool SendMidiMsg(IMidiMsg& msg) = 0;
  bool SendMidiMsgs(WDL_TypedBuf<IMidiMsg>* pMsgs);
  virtual bool SendSysEx(ISysEx& msg) { return false; }
  bool IsInst() { return mIsInst; }
  bool DoesMIDI() { return mDoesMIDI; }
  
  // You can't use these three methods with chunks-based plugins, because there is no way to set the custom data
  void MakeDefaultPreset(const char* name = 0, int nPresets = 1);
  // MakePreset(name, param1, param2, ..., paramN)
  void MakePreset(const char* name, ...);
  // MakePresetFromNamedParams(name, nParamsNamed, paramEnum1, paramVal1, paramEnum2, paramVal2, ..., paramEnumN, paramVal2)
  // nParamsNamed may be less than the total number of params.
  void MakePresetFromNamedParams(const char* name, int nParamsNamed, ...);

  // Use these methods with chunks-based plugins
  void MakePresetFromChunk(const char* name, ByteChunk& chunk);
  void MakePresetFromBlob(const char* name, const char* blob, int sizeOfChunk);

  bool DoesStateChunks() { return mStateChunks; }

  // Will append if the chunk is already started
  bool SerializeParams(ByteChunk& chunk);
  int UnserializeParams(ByteChunk& chunk, int startPos); // Returns the new chunk position (endPos)

  virtual void RedrawParamControls() {};  // Called after restoring state.

  // ----------------------------------------
  // Internal IPlug stuff (but API classes need to get at it).

  void OnParamReset();  // Calls OnParamChange(each param) + Reset().

  void PruneUninitializedPresets();

  // Unserialize / SerializePresets - Only used by VST2
  bool SerializePresets(ByteChunk& chunk);
  int UnserializePresets(ByteChunk& chunk, int startPos); // Returns the new chunk position (endPos).

  // Set connection state for n channels.
  // If a channel is connected, we expect a call to attach the buffers before each process call.
  // If a channel is not connected, we attach scratch buffers now and don't need to do anything else.
  void SetInputChannelConnections(int idx, int n, bool connected);
  void SetOutputChannelConnections(int idx, int n, bool connected);

  void AttachInputBuffers(int idx, int n, double** ppData, int nFrames);
  void AttachInputBuffers(int idx, int n, float** ppData, int nFrames);
  void AttachOutputBuffers(int idx, int n, double** ppData);
  void AttachOutputBuffers(int idx, int n, float** ppData);
  void PassThroughBuffers(float sampleType, int nFrames);
  void PassThroughBuffers(double sampleType, int nFrames);
  void ProcessBuffers(float sampleType, int nFrames);
  void ProcessBuffers(double sampleType, int nFrames);
  void ProcessBuffersAccumulating(float sampleType, int nFrames);
  void ZeroScratchBuffers();
  
public:
  void ModifyCurrentPreset(const char* name = 0);     // Sets the currently active preset to whatever current params are.
  int NPresets() { return mPresets.GetSize(); }
  int GetCurrentPresetIdx() { return mCurrentPresetIdx; }
  bool RestorePreset(int idx);
  bool RestorePreset(const char* name);
  const char* GetPresetName(int idx);
  
  virtual void DirtyPTCompareState() {}; // needed in chunks based plugins to tell PT a non-indexed param changed and to turn on the compare light

  // Dump the current state as source code for a call to MakePresetFromNamedParams / MakePresetFromBlob
  void DumpPresetSrcCode(const char* file, const char* paramEnumNames[]);
  void DumpPresetBlob(const char* file);
  void DumpBankBlob(const char* file);
  
  virtual void PresetsChangedByHost() {} // does nothing by default
  void DirtyParameters(); // hack to tell the host to dirty file state, when a preset is recalled
  
  bool SaveProgramAsFXP(WDL_String& file);
  bool SaveBankAsFXB(WDL_String& file);
  bool LoadProgramFromFXP(WDL_String& file);
  bool LoadBankFromFXB(WDL_String& file);
  
  void SetSampleRate(double sampleRate);
  virtual void SetBlockSize(int blockSize); // overridden in IPlugAU

private:
  char mEffectName[MAX_EFFECT_NAME_LEN], mProductName[MAX_EFFECT_NAME_LEN], mMfrName[MAX_EFFECT_NAME_LEN];
  int mUniqueID, mMfrID, mVersion;   //  Version stored as 0xVVVVRRMM: V = version, R = revision, M = minor revision.

  EAPI mAPI;
  EHost mHost;
  int mHostVersion;   //  Version stored as 0xVVVVRRMM: V = version, R = revision, M = minor revision.

  struct InChannel
  {
    bool mConnected;
    double** mSrc;   // Points into mInData.
    WDL_TypedBuf<double> mScratchBuf;
    WDL_String mLabel;
  };

  struct OutChannel
  {
    bool mConnected;
    double** mDest;  // Points into mOutData.
    float* mFDest;
    WDL_TypedBuf<double> mScratchBuf;
    WDL_String mLabel;
  };

protected:
  bool mStateChunks, mIsInst, mDoesMIDI, mIsBypassed, mHasUI;
  int mCurrentPresetIdx;
  double mSampleRate;
  int mBlockSize, mLatency;
  unsigned int mTailSize;
  NChanDelayLine<double>* mLatencyDelay; // for delaying dry signal when mLatency > 0 and plugin is bypassed
  WDL_PtrList<const char> mParamGroups;
  WDL_PtrList<IParam> mParams;
  WDL_PtrList<IPreset> mPresets;
  WDL_TypedBuf<double*> mInData, mOutData;
  WDL_PtrList<InChannel> mInChannels;
  WDL_PtrList<OutChannel> mOutChannels;
  WDL_PtrList<WDL_String> mInputBusLabels;
  WDL_PtrList<WDL_String> mOutputBusLabels;
  WDL_PtrList<ChannelIO> mChannelIO;
  
public:
  WDL_Mutex mParams_mutex; // lock when accessing mParams (including via GetParam) from the audio thread
};
