/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

/**
 
 IPlug plug-in -> Standalone app wrapper, using Cockos' SWELL
 
 Oli Larkin 2014-2023
 
 Notes:
 
 App settings are stored in a .ini (text) file. The location is as follows:
 
 Windows7: C:\Users\USERNAME\AppData\Local\BUNDLE_NAME\settings.ini
 Windows XP/Vista: C:\Documents and Settings\USERNAME\Local Settings\Application Data\BUNDLE_NAME\settings.ini
 macOS: /Users/USERNAME/Library/Application\ Support/BUNDLE_NAME/settings.ini
 OR
 /Users/USERNAME/Library/Containers/BUNDLE_ID/Data/Library/Application Support/BUNDLE_NAME/settings.ini
 
 */

#include <cstdlib>
#include <string>
#include <vector>
#include <limits>
#include <memory>
#include <optional>

#include "wdltypes.h"
#include "wdlstring.h"

#include "IPlugPlatform.h"
#include "IPlugConstants.h"

#include "IPlugAPP.h"

#include "config.h"

#ifdef OS_WIN
  #include <WindowsX.h>
  #include <commctrl.h>
  #include <shlobj.h>
  #define DEFAULT_INPUT_DEV "Default Device"
  #define DEFAULT_OUTPUT_DEV "Default Device"
#elif defined(OS_MAC)
  #include "IPlugSWELL.h"
  #define SLEEP( milliseconds ) usleep( (unsigned long) (milliseconds * 1000.0) )
  #define DEFAULT_INPUT_DEV "Built-in Input"
  #define DEFAULT_OUTPUT_DEV "Built-in Output"
#elif defined(OS_LINUX)
  #include "IPlugSWELL.h"
#endif

#include "RtAudio.h"
#include "RtMidi.h"

#ifdef OS_MAC
#include <CoreAudio/CoreAudio.h>
#include <CoreMIDI/CoreMIDI.h>
#endif

#define OFF_TEXT "off"

extern HWND gHWND;
extern HINSTANCE gHINSTANCE;

BEGIN_IPLUG_NAMESPACE

const int kNumBufferSizeOptions = 11;
const std::string kBufferSizeOptions[kNumBufferSizeOptions] = {"32", "64", "96", "128", "192", "256", "512", "1024", "2048", "4096", "8192" };
const int kDeviceDS = 0; const int kDeviceCoreAudio = 0; const int kDeviceAlsa = 0;
const int kDeviceASIO = 1; const int kDeviceJack = 1;
extern UINT gSCROLLMSG;

class IPlugAPP;

/** A class that hosts an IPlug as a standalone app and provides Audio/Midi I/O */
class IPlugAPPHost
{
public:
  
  /** Used to manage changes to app I/O */
  struct AppState
  {
    WDL_String mAudioInDev;
    WDL_String mAudioOutDev;
    WDL_String mMidiInDev;
    WDL_String mMidiOutDev;
    uint32_t mAudioDriverType;
    uint32_t mAudioSR;
    uint32_t mBufferSize;
    uint32_t mMidiInChan;
    uint32_t mMidiOutChan;
    
    uint32_t mAudioInChanL;
    uint32_t mAudioInChanR;
    uint32_t mAudioOutChanL;
    uint32_t mAudioOutChanR;
    
    AppState()
    : mAudioInDev(DEFAULT_INPUT_DEV)
    , mAudioOutDev(DEFAULT_OUTPUT_DEV)
    , mMidiInDev(OFF_TEXT)
    , mMidiOutDev(OFF_TEXT)
    , mAudioDriverType(0) // DirectSound / CoreAudio by default
    , mBufferSize(512)
    , mAudioSR(44100)
    , mMidiInChan(0)
    , mMidiOutChan(0)
    
    , mAudioInChanL(1)
    , mAudioInChanR(2)
    , mAudioOutChanL(1)
    , mAudioOutChanR(2)
    {
    }
    
    AppState (const AppState& obj)
    : mAudioInDev(obj.mAudioInDev.Get())
    , mAudioOutDev(obj.mAudioOutDev.Get())
    , mMidiInDev(obj.mMidiInDev.Get())
    , mMidiOutDev(obj.mMidiOutDev.Get())
    , mAudioDriverType(obj.mAudioDriverType)
    , mBufferSize(obj.mBufferSize)
    , mAudioSR(obj.mAudioSR)
    , mMidiInChan(obj.mMidiInChan)
    , mMidiOutChan(obj.mMidiOutChan)
    
    , mAudioInChanL(obj.mAudioInChanL)
    , mAudioInChanR(obj.mAudioInChanR)
    , mAudioOutChanL(obj.mAudioInChanL)
    , mAudioOutChanR(obj.mAudioInChanR)
    {
    }
    
    bool operator==(const AppState& rhs) const {
      return (rhs.mAudioDriverType == mAudioDriverType &&
              rhs.mBufferSize == mBufferSize &&
              rhs.mAudioSR == mAudioSR &&
              rhs.mMidiInChan == mMidiInChan &&
              rhs.mMidiOutChan == mMidiOutChan &&
              (std::string_view(rhs.mAudioInDev.Get()) == mAudioInDev.Get()) &&
              (std::string_view(rhs.mAudioOutDev.Get()) == mAudioOutDev.Get()) &&
              (std::string_view(rhs.mMidiInDev.Get()) == mMidiInDev.Get()) &&
              (std::string_view(rhs.mMidiOutDev.Get()) == mMidiOutDev.Get()) &&
              rhs.mAudioInChanL == mAudioInChanL &&
              rhs.mAudioInChanR == mAudioInChanR &&
              rhs.mAudioOutChanL == mAudioOutChanL &&
              rhs.mAudioOutChanR == mAudioOutChanR
      );
    }
    bool operator!=(const AppState& rhs) const { return !operator==(rhs); }
  };
  
  static IPlugAPPHost* Create();
  static std::unique_ptr<IPlugAPPHost> sInstance;
  
  void PopulateSampleRateList(HWND hwndDlg, RtAudio::DeviceInfo* pInputDevInfo, RtAudio::DeviceInfo* pOutputDevInfo);
  void PopulateAudioInputList(HWND hwndDlg, RtAudio::DeviceInfo* pInfo);
  void PopulateAudioOutputList(HWND hwndDlg, RtAudio::DeviceInfo* pInfo);
  void PopulateDriverSpecificControls(HWND hwndDlg);
  void PopulateAudioDialogs(HWND hwndDlg);
  bool PopulateMidiDialogs(HWND hwndDlg);
  void PopulatePreferencesDialog(HWND hwndDlg);
  
  IPlugAPPHost();
  ~IPlugAPPHost();
  
  bool OpenWindow(HWND pParent);
  void CloseWindow();

  bool Init();
  bool InitState();
  void UpdateINI();
  
  /** Returns the name of the audio device with a given RTAudio device ID
   * @param deviceID The ID RTAudio has given the audio device
   * @return The device name. Core Audio device names are truncated. */
  std::string GetAudioDeviceName(uint32_t deviceID) const;
  
  /** Returns the a validated audio device ID linked to a particular name
  * @param name The name of the audio device to test
  * @return The ID RTAudio has given the audio device if found */
  std::optional<uint32_t> GetAudioDeviceID(const char* name) const;
  
  /** @param direction Either kInput or kOutput
   * @param name The name of the midi device
   * @return An integer specifying the output port number, where 0 means any */
  int GetMIDIPortNumber(ERoute direction, const char* name) const;
  
  void ProbeAudioIO();
  void ProbeMidiIO();
  bool InitMidi();
  void CloseAudio();
  bool InitAudio(uint32_t inId, uint32_t outId, uint32_t sr, uint32_t iovs);
  bool AudioSettingsInStateAreEqual(AppState& os, AppState& ns);
  bool MIDISettingsInStateAreEqual(AppState& os, AppState& ns);

  bool TryToChangeAudioDriverType();
  bool TryToChangeAudio();
  bool SelectMIDIDevice(ERoute direction, const char* portName);
  
  static int AudioCallback(void* pOutputBuffer, void* pInputBuffer, uint32_t nFrames, double streamTime, RtAudioStreamStatus status, void* pUserData);
  static void MIDICallback(double deltatime, std::vector<uint8_t>* pMsg, void* pUserData);
  static void ErrorCallback(RtAudioErrorType type, const std::string& errorText);

  static WDL_DLGRET PreferencesDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
  static WDL_DLGRET MainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

  IPlugAPP* GetPlug() { return mIPlug.get(); }

  int GetNumDeviceInputChannels() const { return mNumDeviceInputs; }
  int GetNumDeviceOutputChannels() const { return mNumDeviceOutputs; }

private:
  std::unique_ptr<IPlugAPP> mIPlug = nullptr;
  std::unique_ptr<RtAudio> mDAC = nullptr;
  std::unique_ptr<RtMidiIn> mMidiIn = nullptr;
  std::unique_ptr<RtMidiOut> mMidiOut = nullptr;
  int mMidiOutChannel = -1;
  int mMidiInChannel = -1;

  AppState mState;
  /** When the preferences dialog is opened the existing state is cached here, and restored if cancel is pressed */
  AppState mTempState;
  /** When the audio driver is started the current state is copied here so that if OK is pressed after APPLY nothing is changed */
  AppState mActiveState;
  
  double mSampleRate = 44100.;
  uint32_t mSamplesElapsed = 0;
  uint32_t mVecWait = 0;
  uint32_t mBufferSize = 512;
  uint32_t mBufIndex = 0; // index for signal vector, loops from 0 to mSigVS
  bool mExiting = false;
  bool mAudioEnding = false;
  bool mAudioDone = false;

  /** The ID of the operating system's default input device if detected */
  std::optional<uint32_t> mDefaultInputDev;
  /** The ID of the operating system's default output device if detected */
  std::optional<uint32_t> mDefaultOutputDev;
    
  WDL_String mINIPath;

  std::vector<uint32_t> mAudioInputDevIDs;
  std::vector<uint32_t> mAudioOutputDevIDs;
  std::vector<std::string> mMidiInputDevNames;
  std::vector<std::string> mMidiOutputDevNames;
  
  WDL_PtrList<double> mInputBufPtrs;
  WDL_PtrList<double> mOutputBufPtrs;
  
  int mNumDeviceInputs = 0;
  int mNumDeviceOutputs = 0;
  
  friend class IPlugAPP;

#ifdef OS_MAC
  static OSStatus AudioDeviceListChanged(AudioObjectID inObjectID, UInt32 inNumberAddresses, const AudioObjectPropertyAddress* inAddresses, void* inClientData);
  static void MIDIDeviceListChanged(const MIDINotification *message, void* refCon);
  
  void RegisterDeviceNotifications();
  void UnregisterDeviceNotifications();
  void OnDeviceListChanged();
#endif
};

END_IPLUG_NAMESPACE
