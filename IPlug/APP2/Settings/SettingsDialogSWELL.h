/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for more info.

 ==============================================================================
*/

#pragma once

#include "ISettingsDialog.h"

#include <array>
#include <string>

#ifdef OS_WIN
  #include <windows.h>
#else
  #include "IPlugSWELL.h"
#endif

BEGIN_IPLUG_NAMESPACE

/**
 * SettingsDialogSWELL - SWELL-based settings dialog implementation
 *
 * Uses Win32/SWELL dialog resources for cross-platform compatibility.
 * Includes level meters for visual audio I/O feedback.
 */
class SettingsDialogSWELL : public ISettingsDialog
{
public:
  SettingsDialogSWELL();
  ~SettingsDialogSWELL() override;

  //--- ISettingsDialog interface ---

  bool ShowModal(void* parentWindow) override;
  void SetState(const AppState& state) override;
  AppState GetState() const override;
  void SetInputDevices(const std::vector<AudioDeviceInfo>& devices) override;
  void SetOutputDevices(const std::vector<AudioDeviceInfo>& devices) override;
  void SetMidiInputDevices(const std::vector<std::string>& devices) override;
  void SetMidiOutputDevices(const std::vector<std::string>& devices) override;
  void SetIOConfigs(const std::vector<IOConfig>& configs) override;
  void SetAudioEngine(AudioEngine* engine) override;

private:
  //--- Dialog procedures ---

  static INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
  INT_PTR HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

  //--- UI Population ---

  void PopulateDriverList(HWND hwnd);
  void PopulateInputDeviceList(HWND hwnd);
  void PopulateOutputDeviceList(HWND hwnd);
  void PopulateSampleRateList(HWND hwnd);
  void PopulateBufferSizeList(HWND hwnd);
  void PopulateIOConfigList(HWND hwnd);
  void PopulateInputChannelLists(HWND hwnd);
  void PopulateOutputChannelLists(HWND hwnd);
  void PopulateMidiInputList(HWND hwnd);
  void PopulateMidiOutputList(HWND hwnd);
  void PopulateMidiChannelLists(HWND hwnd);

  void UpdateControlStates(HWND hwnd);

  //--- Level Meter Handling ---

  void CreateLevelMeters(HWND hwnd);
  void UpdateLevelMeters();
  void PaintLevelMeters(HWND hwnd, HDC hdc);

  static void CALLBACK MeterTimerProc(HWND hwnd, UINT msg, UINT_PTR id, DWORD time);

  //--- State ---

  AppState mState;
  AppState mOriginalState;  // For cancel

  std::vector<AudioDeviceInfo> mInputDevices;
  std::vector<AudioDeviceInfo> mOutputDevices;
  std::vector<std::string> mMidiInputDevices;
  std::vector<std::string> mMidiOutputDevices;
  std::vector<IOConfig> mIOConfigs;

  AudioEngine* mAudioEngine = nullptr;

  //--- Level Meters ---

  static constexpr int kMaxMeters = 8;
  std::array<LevelMeter, kMaxMeters> mInputMeters;
  std::array<LevelMeter, kMaxMeters> mOutputMeters;
  int mNumInputMeters = 0;
  int mNumOutputMeters = 0;

  HWND mDialogHwnd = nullptr;
  UINT_PTR mMeterTimerId = 0;

  //--- Device info cache ---

  std::vector<uint32_t> mInputSampleRates;
  std::vector<uint32_t> mOutputSampleRates;
  int mSelectedInputChannels = 2;
  int mSelectedOutputChannels = 2;
};

END_IPLUG_NAMESPACE
