#pragma once

#include "IPlugPlatform.h"

/*

 Standalone osx/win app wrapper for IPlug, using SWELL
 Oli Larkin 2012 - 2018

 Notes:

 App settings are stored in a .ini file. The location is as follows:

 Windows7+: C:\Users\USERNAME\AppData\Local\IPlugEffect\settings.ini
 Windows XP/Vista: C:\Documents and Settings\USERNAME\Local Settings\Application Data\IPlugEffect\settings.ini
 OSX: /Users/USERNAME/Library/Application\ Support/IPlugEffect/settings.ini

*/

#ifdef OS_WIN
  #include <windows.h>
  #include <commctrl.h>

  #define DEFAULT_INPUT_DEV "Default Device"
  #define DEFAULT_OUTPUT_DEV "Default Device"

  #define DAC_DS 0
  #define DAC_ASIO 1
#elif defined OS_MAC
  #include "swell.h"
  #define SLEEP( milliseconds ) usleep( (unsigned long) (milliseconds * 1000.0) )

  #define DEFAULT_INPUT_DEV "Built-in Input"
  #define DEFAULT_OUTPUT_DEV "Built-in Output"

  #define DAC_COREAUDIO 0
//  #define DAC_JACK 1
#endif

#include "wdltypes.h"
#include "RtAudio.h"
#include "RtMidi.h"
#include <string>
#include <vector>

#include "../resources/resource.h" // change this to match your iplug resource .h file
#include "../IPlugEffect.h" // change this to match your iplug plugin .h file

#define MAX_DEVNAME_LEN 100

struct AppState
{
  // on osx core audio 0 or jack 1
  // on windows DS 0 or ASIO 1
  uint16_t mAudioDriverType;

  // strings
  char mAudioInDev[MAX_DEVNAME_LEN];
  char mAudioOutDev[MAX_DEVNAME_LEN];
  char mAudioSR[MAX_DEVNAME_LEN];
  char mAudioIOVS[MAX_DEVNAME_LEN];
  char mAudioSigVS[MAX_DEVNAME_LEN];

  uint16_t mAudioInChanL;
  uint16_t mAudioInChanR;
  uint16_t mAudioOutChanL;
  uint16_t mAudioOutChanR;
  uint16_t mAudioInIsMono;

  // strings containing the names of the midi devices
  char mMidiInDev[MAX_DEVNAME_LEN];
  char mMidiOutDev[MAX_DEVNAME_LEN];

  uint16_t mMidiInChan;
  uint16_t mMidiOutChan;

  AppState():
    mAudioDriverType(0), // DS / CoreAudio by default
    mAudioInChanL(1),
    mAudioInChanR(2),
    mAudioOutChanL(1),
    mAudioOutChanR(2),
    mMidiInChan(0),
    mMidiOutChan(0)
  {
    strcpy(mAudioInDev, DEFAULT_INPUT_DEV);
    strcpy(mAudioOutDev, DEFAULT_OUTPUT_DEV);
    strcpy(mAudioSR, "44100");
    strcpy(mAudioIOVS, "512");
    strcpy(mAudioSigVS, "32");

    strcpy(mMidiInDev, "off");
    strcpy(mMidiOutDev, "off");
  }
};

extern WDL_DLGRET MainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern WDL_DLGRET PreferencesDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
extern HINSTANCE gHInstance;
extern HWND gHWND;
extern UINT gScrollMessage;
extern IPlug* gPluginInstance;

extern std::string GetAudioDeviceName(int idx);
extern int GetAudioDeviceID(const char* deviceNameToTest);

extern void ProbeAudioIO();
extern bool InitialiseAudio(uint32_t inId, uint32_t outId, uint32_t sr, uint32_t iovs, uint32_t chnls, uint32_t inChanL, uint32_t outChanL);

extern bool AudioSettingsInStateAreEqual(AppState* os, AppState* ns);
extern bool MIDISettingsInStateAreEqual(AppState* os, AppState* ns);

extern bool TryToChangeAudioDriverType();
extern bool TryToChangeAudio();
extern bool ChooseMidiInput(const char* portName);
extern bool ChooseMidiOutput(const char* portName);

extern bool AttachGUI();

extern RtAudio* gDAC;
extern RtMidiIn* gMidiIn;
extern RtMidiOut* gMidiOut;

extern AppState* gState;
extern AppState* gTempState; // The state is copied here when the pref dialog is opened, and restored if cancel is pressed
extern AppState* gActiveState; // When the audio driver is started the current state is copied here so that if OK is pressed after APPLY nothing is changed

extern uint32_t gSigVS;
extern uint32_t gBufIndex; // index for signal vector, loops from 0 to gSigVS

extern char gINIPath[MAX_PATH];
extern void UpdateINI();

extern std::vector<uint32_t> gAudioInputDevs;
extern std::vector<uint32_t> gAudioOutputDevs;
extern std::vector<std::string> gMIDIInputDevNames;
extern std::vector<std::string> gMIDIOutputDevNames;
