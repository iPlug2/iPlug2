/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for more info.

 ==============================================================================
*/

#pragma once

/**
 * IPlugAPP2 - Modernized Standalone App Wrapper
 *
 * A complete rewrite of the iPlug2 standalone app wrapper with:
 * - Clean separation of platform code from core logic
 * - Flexible audio I/O respecting PLUG_CHANNEL_IO configurations
 * - Real-time level metering in settings dialog
 * - Extensible settings UI (SWELL, IGraphics, or WebView)
 *
 * See ARCHITECTURE.md for design documentation.
 */

#include <memory>
#include <string>
#include <vector>

#include "IPlugPlatform.h"
#include "IPlugConstants.h"

#include "Audio/AudioEngine.h"
#include "State/AppState.h"
#include "Settings/ISettingsDialog.h"

// Forward declarations
class RtMidiIn;
class RtMidiOut;

BEGIN_IPLUG_NAMESPACE

class IPlugAPP2;

/**
 * IPlugAPP2Host - Orchestrates all standalone app components
 *
 * Owns the plugin instance, audio/MIDI engines, and settings dialog.
 * Handles initialization, state management, and main window lifecycle.
 */
class IPlugAPP2Host
{
public:
  /** Create the singleton instance */
  static IPlugAPP2Host* Create();

  /** Get the singleton instance */
  static IPlugAPP2Host* Get() { return sInstance.get(); }

  IPlugAPP2Host();
  ~IPlugAPP2Host();

  // Non-copyable
  IPlugAPP2Host(const IPlugAPP2Host&) = delete;
  IPlugAPP2Host& operator=(const IPlugAPP2Host&) = delete;

  //--- Initialization ---

  /** Initialize the host (called after window creation) */
  bool Init();

  /** Load state from INI file */
  bool LoadState();

  /** Save state to INI file */
  bool SaveState();

  //--- Plugin Access ---

  /** Get the plugin instance */
  IPlugAPP2* GetPlug() { return mPlug.get(); }

  //--- Window Management ---

  /** Open the main window and attach plugin UI */
  bool OpenWindow(void* pParent);

  /** Close the main window */
  void CloseWindow();

  //--- Settings Dialog ---

  /** Show the settings/preferences dialog */
  bool ShowSettingsDialog(void* pParent);

  //--- Audio Control ---

  /** Start audio processing */
  bool StartAudio();

  /** Stop audio processing */
  void StopAudio();

  /** Check if audio is running */
  bool IsAudioRunning() const;

  //--- I/O Configuration ---

  /** Get available I/O configurations from PLUG_CHANNEL_IO */
  const std::vector<IOConfig>& GetIOConfigs() const { return mIOConfigs; }

  /** Get currently selected I/O configuration */
  const IOConfig& GetCurrentIOConfig() const;

  /** Parse PLUG_CHANNEL_IO string into IOConfig list */
  static std::vector<IOConfig> ParseChannelIOString(const char* ioStr);

  //--- Platform Dialog Procs (for SWELL compatibility) ---

  /** Main window dialog proc */
  static WDL_DLGRET MainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

  /** Preferences dialog proc */
  static WDL_DLGRET PreferencesDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
  //--- Audio/MIDI Engine Management ---

  /** Initialize audio with current state settings */
  bool InitializeAudio();

  /** Initialize MIDI with current state settings */
  bool InitializeMidi();

  /** Probe available devices and update state */
  void ProbeDevices();

  /** Audio processing callback */
  void AudioCallback(double** inputs, double** outputs,
                     int nInputs, int nOutputs,
                     int nFrames, double sampleRate, double streamTime);

  /** MIDI input callback */
  static void MidiCallback(double deltaTime, std::vector<uint8_t>* pMsg, void* pUserData);

  //--- State Application ---

  /** Apply audio state changes (may restart audio) */
  bool ApplyAudioState(const AppState& newState);

  /** Apply MIDI state changes */
  bool ApplyMidiState(const AppState& newState);

  //--- Members ---

  static std::unique_ptr<IPlugAPP2Host> sInstance;

  std::unique_ptr<IPlugAPP2> mPlug;
  std::unique_ptr<AudioEngine> mAudioEngine;
  std::unique_ptr<RtMidiIn> mMidiIn;
  std::unique_ptr<RtMidiOut> mMidiOut;
  std::unique_ptr<ISettingsDialog> mSettingsDialog;

  AppState mState;
  AppState mActiveState;  // State currently in use by audio engine
  std::string mSettingsPath;

  std::vector<IOConfig> mIOConfigs;
  std::vector<std::string> mMidiInputDevices;
  std::vector<std::string> mMidiOutputDevices;

  bool mAudioInitialized = false;
  bool mExiting = false;
};

END_IPLUG_NAMESPACE

// Global window handles (for SWELL compatibility)
extern HWND gHWND;
extern HINSTANCE gHINSTANCE;
