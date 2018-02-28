#pragma once

/**
 
 IPlug plug-in -> Standalone app wrapper, using Cockos' SWELL
 
 Oli Larkin 2014-2018
 
 Notes:
 
 App settings are stored in a .ini (text) file. The location is as follows:
 
 Windows7: C:\Users\USERNAME\AppData\Local\AppName\settings.ini
 Windows XP/Vista: C:\Documents and Settings\USERNAME\Local Settings\Application Data\AppName\settings.ini
 OSX: /Users/USERNAME/Library/Application\ Support/AppName/settings.ini
 
 */

#include <cstdlib>
#include <string>
#include <vector>
#include <limits>

#include "RtAudio.h"
#include "RtMidi.h"

#include "wdltypes.h"
#include "wdlstring.h"

#include "IPlugPlatform.h"
#include "IPlugConstants.h"

#include "config.h"

#ifdef OS_WIN
  #include <WindowsX.h>
  #include <commctrl.h>
  #include <shlobj.h>
  #define DEFAULT_INPUT_DEV "Default Device"
  #define DEFAULT_OUTPUT_DEV "Default Device"
#elif defined(OS_MAC)
  #include "swell.h"
  #define SLEEP( milliseconds ) usleep( (unsigned long) (milliseconds * 1000.0) )
  #define DEFAULT_INPUT_DEV "Built-in Input"
  #define DEFAULT_OUTPUT_DEV "Built-in Output"
#elif defined(OS_LINUX)
  #include "swell.h"
#endif

const int kNumBufferSizeOptions = 11;
const std::string kBufferSizeOptions[kNumBufferSizeOptions] = {"32", "64", "96", "128", "192", "256", "512", "1024", "2048", "4096", "8192" };
const int kDeviceDS = 0; const int kDeviceCoreAudio = 0; const int kDeviceAlsa = 0;
const int kDeviceASIO = 0; const int kDeviceJack = 1;
extern HWND gHWND;
extern HINSTANCE gHINSTANCE;
extern UINT gSCROLLMSG;
extern WDL_DLGRET MainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern WDL_DLGRET PreferencesDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

class IPlugAPPHost
{
public:
  struct AppState
  {
    uint32_t mAudioDriverType = 0; // DirectSound / CoreAudio by default
    
    WDL_String mAudioInDev = WDL_String(DEFAULT_INPUT_DEV);
    WDL_String mAudioOutDev = WDL_String(DEFAULT_OUTPUT_DEV);
    WDL_String mMidiInDev = WDL_String("off");
    WDL_String mMidiOutDev = WDL_String("off");
    
    uint32_t mAudioSR =  44100;
    uint32_t mBufferSize = 512;
    
    uint32_t mMidiInChan = 0;
    uint32_t mMidiOutChan = 0;
  };
  
  static IPlugAPPHost* create();
  static IPlugAPPHost* sInstance;
  
//  void PopulateSampleRateList(HWND hwndDlg, RtAudio::DeviceInfo* pInputDevInfo, RtAudio::DeviceInfo* pOutputDevInfo);
//  void PopulateAudioInputList(HWND hwndDlg, RtAudio::DeviceInfo* pInfo);
//  void PopulateAudioOutputList(HWND hwndDlg, RtAudio::DeviceInfo* pInfo);
//  void PopulateDriverSpecificControls(HWND hwndDlg);
//  void PopulateAudioDialogs(HWND hwndDlg);
//  bool PopulateMidiDialogs(HWND hwndDlg);
  void PopulatePreferencesDialog(HWND hwndDlg);
  
  IPlugAPPHost();
  ~IPlugAPPHost();

//  bool init();
//  bool AttachGUI();
//  bool InitialiseState();
//  void UpdateINI();
  
  /** Returns the name of the audio device at idx
   * @param idx The index RTAudio has given the audio device
   * @return The device name. Core Audio device names are truncated. */
  std::string GetAudioDeviceName(int idx);
  // returns the rtaudio device ID, based on the (truncated) device name
  
  /** Returns the audio device index linked to a particular name
  * @param name The name of the audio device to test
  * @return The integer index RTAudio has given the audio device */
  int GetAudioDeviceIdx(const char* name);
  
  /** @param direction Either kInput or kOutput
   * @param name The name of the midi device
   * @return An integer specifying the output port number, where 0 means any */
  uint32_t GetMIDIPortNumber(ERoute direction, const char* name);
  
//  void ProbeAudioIO();
//  void ProbeMidiIO();
//  bool InitialiseMidi();
//  bool InitialiseAudio(uint32_t inId, uint32_t outId, uint32_t sr, uint32_t iovs, uint32_t chnls, uint32_t inChanL, uint32_t outChanL);
//  bool AudioSettingsInStateAreEqual(AppState* os, AppState* ns);
//  bool MIDISettingsInStateAreEqual(AppState* os, AppState* ns);
//
//  bool TryToChangeAudioDriverType();
//  bool TryToChangeAudio();
//  bool ChooseMidiInput(const char* portName);
//  bool ChooseMidiOutput(const char* portName);
  
  static int AudioCallback(void* pOutputBuffer, void* pInputBuffer, uint32_t nFrames, double streamTime, RtAudioStreamStatus status, void* pUserData);
  static void MIDICallback(double deltatime, std::vector<uint8_t>* pMsg, void* pUserData);
  static void ErrorCallback(RtAudioError::Type type, const std::string& errorText);

private:
  RtAudio* mDAC = nullptr;
  RtMidiIn* mMidiIn = nullptr;
  RtMidiOut* mMidiOut = nullptr;

  /** When the preferences dialog is opened the existing state is cached here, and restored if cancel is pressed */
  AppState* mState = nullptr;
  /** When the preferences dialog is opened the existing state is cached here, and restored if cancel is pressed */
  AppState* mTempState = nullptr;
  /** When the audio driver is started the current state is copied here so that if OK is pressed after APPLY nothing is changed */
  AppState* mActiveState = nullptr;
  
  double mFadeMult = 0.; // Fade multiplier
  double mSampleRate = 44100.;
  uint32_t mVecElapsed = 0;
  uint32_t mBufferSize = 512;
  uint32_t mBufIndex; // index for signal vector, loops from 0 to mSigVS
  
  /** The index of the operating systems default input device, -1 if not detected */
  int32_t mDefaultInputDev = -1;
  /** The index of the operating systems default output device, -1 if not detected */
  int32_t mDefaultOutputDev;
    
  WDL_String mINIPath;
  
  //TODO: replace with std::map or WDL something
  std::vector<uint32_t> mAudioInputDevs;
  std::vector<uint32_t> mAudioOutputDevs;
  std::vector<std::string> mAudioIDDevNames;
  std::vector<std::string> mMidiInputDevNames;
  std::vector<std::string> mMidiOutputDevNames;
};
