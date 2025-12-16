/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for more info.

 ==============================================================================
*/

#include "SettingsDialogSWELL.h"
#include "Audio/AudioEngine.h"

#include <algorithm>

BEGIN_IPLUG_NAMESPACE

// Buffer size options
static const uint32_t kBufferSizes[] = { 32, 64, 96, 128, 192, 256, 512, 1024, 2048, 4096, 8192 };
static const int kNumBufferSizes = sizeof(kBufferSizes) / sizeof(kBufferSizes[0]);

// Timer ID for level meter updates
static const UINT_PTR kMeterTimerId = 1001;
static const UINT kMeterTimerInterval = 50; // 20 Hz refresh

// Temporary: Dialog will be created programmatically until resource files are added
// Control IDs
enum
{
  IDC_DRIVER_COMBO = 1001,
  IDC_INPUT_DEV_COMBO,
  IDC_OUTPUT_DEV_COMBO,
  IDC_SAMPLE_RATE_COMBO,
  IDC_BUFFER_SIZE_COMBO,
  IDC_IO_CONFIG_COMBO,
  IDC_MIDI_IN_COMBO,
  IDC_MIDI_OUT_COMBO,
  IDC_MIDI_IN_CHAN_COMBO,
  IDC_MIDI_OUT_CHAN_COMBO,
  IDC_INPUT_METERS_STATIC,
  IDC_OUTPUT_METERS_STATIC,
  IDC_APPLY_BUTTON,
  IDC_CONFIG_BUTTON,
};

// Factory function
std::unique_ptr<ISettingsDialog> CreateSettingsDialog()
{
  return std::make_unique<SettingsDialogSWELL>();
}

SettingsDialogSWELL::SettingsDialogSWELL()
{
}

SettingsDialogSWELL::~SettingsDialogSWELL()
{
  if (mMeterTimerId)
  {
    KillTimer(mDialogHwnd, mMeterTimerId);
    mMeterTimerId = 0;
  }
}

bool SettingsDialogSWELL::ShowModal(void* parentWindow)
{
  mOriginalState = mState;

  // TODO: Create dialog from resource or programmatically
  // For now, return false as dialog is not yet implemented
  // This is a skeleton that will be fleshed out

  // The actual implementation would:
  // 1. Create the dialog window
  // 2. Populate all controls
  // 3. Run modal message loop
  // 4. Return true if OK, false if Cancel

  return false;
}

void SettingsDialogSWELL::SetState(const AppState& state)
{
  mState = state;
}

AppState SettingsDialogSWELL::GetState() const
{
  return mState;
}

void SettingsDialogSWELL::SetInputDevices(const std::vector<AudioDeviceInfo>& devices)
{
  mInputDevices = devices;
}

void SettingsDialogSWELL::SetOutputDevices(const std::vector<AudioDeviceInfo>& devices)
{
  mOutputDevices = devices;
}

void SettingsDialogSWELL::SetMidiInputDevices(const std::vector<std::string>& devices)
{
  mMidiInputDevices = devices;
}

void SettingsDialogSWELL::SetMidiOutputDevices(const std::vector<std::string>& devices)
{
  mMidiOutputDevices = devices;
}

void SettingsDialogSWELL::SetIOConfigs(const std::vector<IOConfig>& configs)
{
  mIOConfigs = configs;
}

void SettingsDialogSWELL::SetAudioEngine(AudioEngine* engine)
{
  mAudioEngine = engine;

  // Set up meters based on current channel count
  if (engine)
  {
    const auto& config = engine->GetActiveConfig();
    mNumInputMeters = std::min(static_cast<int>(config.inputMappings.size()), kMaxMeters);
    mNumOutputMeters = std::min(static_cast<int>(config.outputMappings.size()), kMaxMeters);
  }
}

INT_PTR CALLBACK SettingsDialogSWELL::DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  SettingsDialogSWELL* pThis = nullptr;

  if (msg == WM_INITDIALOG)
  {
    pThis = reinterpret_cast<SettingsDialogSWELL*>(lParam);
    SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
    pThis->mDialogHwnd = hwnd;
  }
  else
  {
    pThis = reinterpret_cast<SettingsDialogSWELL*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
  }

  if (pThis)
    return pThis->HandleMessage(hwnd, msg, wParam, lParam);

  return FALSE;
}

INT_PTR SettingsDialogSWELL::HandleMessage(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  switch (msg)
  {
    case WM_INITDIALOG:
    {
      PopulateDriverList(hwnd);
      PopulateInputDeviceList(hwnd);
      PopulateOutputDeviceList(hwnd);
      PopulateSampleRateList(hwnd);
      PopulateBufferSizeList(hwnd);
      PopulateIOConfigList(hwnd);
      PopulateMidiInputList(hwnd);
      PopulateMidiOutputList(hwnd);
      PopulateMidiChannelLists(hwnd);
      UpdateControlStates(hwnd);
      CreateLevelMeters(hwnd);

      // Start meter timer
      mMeterTimerId = SetTimer(hwnd, kMeterTimerId, kMeterTimerInterval, nullptr);

      return TRUE;
    }

    case WM_COMMAND:
    {
      int ctrlId = LOWORD(wParam);
      int notifyCode = HIWORD(wParam);

      switch (ctrlId)
      {
        case IDOK:
          if (OnApply && !OnApply(mState))
          {
            // Apply failed
            return TRUE;
          }
          KillTimer(hwnd, mMeterTimerId);
          mMeterTimerId = 0;
          EndDialog(hwnd, IDOK);
          return TRUE;

        case IDCANCEL:
          mState = mOriginalState;
          KillTimer(hwnd, mMeterTimerId);
          mMeterTimerId = 0;
          EndDialog(hwnd, IDCANCEL);
          return TRUE;

        case IDC_APPLY_BUTTON:
          if (OnApply)
            OnApply(mState);
          return TRUE;

        case IDC_DRIVER_COMBO:
          if (notifyCode == CBN_SELCHANGE)
          {
            int idx = (int)SendDlgItemMessage(hwnd, IDC_DRIVER_COMBO, CB_GETCURSEL, 0, 0);
            auto apis = GetAvailableAudioApis();
            if (idx >= 0 && idx < static_cast<int>(apis.size()))
            {
              mState.audioApi = apis[idx];
              if (OnDriverChange)
                OnDriverChange(mState.audioApi);
              PopulateInputDeviceList(hwnd);
              PopulateOutputDeviceList(hwnd);
            }
          }
          return TRUE;

        case IDC_INPUT_DEV_COMBO:
          if (notifyCode == CBN_SELCHANGE)
          {
            int idx = (int)SendDlgItemMessage(hwnd, IDC_INPUT_DEV_COMBO, CB_GETCURSEL, 0, 0);
            if (idx >= 0 && idx < static_cast<int>(mInputDevices.size()))
            {
              mState.inputDeviceName = mInputDevices[idx].name;
              PopulateSampleRateList(hwnd);
              PopulateInputChannelLists(hwnd);
              if (OnDeviceChange)
                OnDeviceChange();
            }
          }
          return TRUE;

        case IDC_OUTPUT_DEV_COMBO:
          if (notifyCode == CBN_SELCHANGE)
          {
            int idx = (int)SendDlgItemMessage(hwnd, IDC_OUTPUT_DEV_COMBO, CB_GETCURSEL, 0, 0);
            if (idx >= 0 && idx < static_cast<int>(mOutputDevices.size()))
            {
              mState.outputDeviceName = mOutputDevices[idx].name;
              PopulateSampleRateList(hwnd);
              PopulateOutputChannelLists(hwnd);
              if (OnDeviceChange)
                OnDeviceChange();
            }
          }
          return TRUE;

        case IDC_SAMPLE_RATE_COMBO:
          if (notifyCode == CBN_SELCHANGE)
          {
            int idx = (int)SendDlgItemMessage(hwnd, IDC_SAMPLE_RATE_COMBO, CB_GETCURSEL, 0, 0);
            uint32_t sr = (uint32_t)SendDlgItemMessage(hwnd, IDC_SAMPLE_RATE_COMBO, CB_GETITEMDATA, idx, 0);
            mState.sampleRate = sr;
          }
          return TRUE;

        case IDC_BUFFER_SIZE_COMBO:
          if (notifyCode == CBN_SELCHANGE)
          {
            int idx = (int)SendDlgItemMessage(hwnd, IDC_BUFFER_SIZE_COMBO, CB_GETCURSEL, 0, 0);
            if (idx >= 0 && idx < kNumBufferSizes)
              mState.bufferSize = kBufferSizes[idx];
          }
          return TRUE;

        case IDC_IO_CONFIG_COMBO:
          if (notifyCode == CBN_SELCHANGE)
          {
            int idx = (int)SendDlgItemMessage(hwnd, IDC_IO_CONFIG_COMBO, CB_GETCURSEL, 0, 0);
            mState.selectedIOConfig = idx;
            // Update channel mappings for new config
            if (idx >= 0 && idx < static_cast<int>(mIOConfigs.size()))
            {
              mState.CreateDefaultInputMappings(mIOConfigs[idx].numInputs);
              mState.CreateDefaultOutputMappings(mIOConfigs[idx].numOutputs);
            }
            UpdateControlStates(hwnd);
          }
          return TRUE;

        case IDC_MIDI_IN_COMBO:
          if (notifyCode == CBN_SELCHANGE)
          {
            int idx = (int)SendDlgItemMessage(hwnd, IDC_MIDI_IN_COMBO, CB_GETCURSEL, 0, 0);
            if (idx >= 0 && idx < static_cast<int>(mMidiInputDevices.size()))
              mState.midiInputDevice = mMidiInputDevices[idx];
          }
          return TRUE;

        case IDC_MIDI_OUT_COMBO:
          if (notifyCode == CBN_SELCHANGE)
          {
            int idx = (int)SendDlgItemMessage(hwnd, IDC_MIDI_OUT_COMBO, CB_GETCURSEL, 0, 0);
            if (idx >= 0 && idx < static_cast<int>(mMidiOutputDevices.size()))
              mState.midiOutputDevice = mMidiOutputDevices[idx];
          }
          return TRUE;

        case IDC_MIDI_IN_CHAN_COMBO:
          if (notifyCode == CBN_SELCHANGE)
          {
            int idx = (int)SendDlgItemMessage(hwnd, IDC_MIDI_IN_CHAN_COMBO, CB_GETCURSEL, 0, 0);
            mState.midiInputChannel = idx; // 0 = all, 1-16 = specific
          }
          return TRUE;

        case IDC_MIDI_OUT_CHAN_COMBO:
          if (notifyCode == CBN_SELCHANGE)
          {
            int idx = (int)SendDlgItemMessage(hwnd, IDC_MIDI_OUT_CHAN_COMBO, CB_GETCURSEL, 0, 0);
            mState.midiOutputChannel = idx;
          }
          return TRUE;
      }
      break;
    }

    case WM_TIMER:
      if (wParam == kMeterTimerId)
      {
        UpdateLevelMeters();
        // Invalidate meter area to trigger repaint
        RECT meterRect;
        // TODO: Get actual meter bounds
        InvalidateRect(hwnd, nullptr, FALSE);
      }
      return TRUE;

    case WM_PAINT:
    {
      PAINTSTRUCT ps;
      HDC hdc = BeginPaint(hwnd, &ps);
      PaintLevelMeters(hwnd, hdc);
      EndPaint(hwnd, &ps);
      return TRUE;
    }

    case WM_DESTROY:
      if (mMeterTimerId)
      {
        KillTimer(hwnd, mMeterTimerId);
        mMeterTimerId = 0;
      }
      mDialogHwnd = nullptr;
      return TRUE;
  }

  return FALSE;
}

void SettingsDialogSWELL::PopulateDriverList(HWND hwnd)
{
  SendDlgItemMessage(hwnd, IDC_DRIVER_COMBO, CB_RESETCONTENT, 0, 0);

  auto apis = GetAvailableAudioApis();
  int selectIdx = 0;

  for (size_t i = 0; i < apis.size(); ++i)
  {
    SendDlgItemMessage(hwnd, IDC_DRIVER_COMBO, CB_ADDSTRING, 0,
                       reinterpret_cast<LPARAM>(GetAudioApiName(apis[i])));
    if (apis[i] == mState.audioApi)
      selectIdx = static_cast<int>(i);
  }

  SendDlgItemMessage(hwnd, IDC_DRIVER_COMBO, CB_SETCURSEL, selectIdx, 0);
}

void SettingsDialogSWELL::PopulateInputDeviceList(HWND hwnd)
{
  SendDlgItemMessage(hwnd, IDC_INPUT_DEV_COMBO, CB_RESETCONTENT, 0, 0);

  int selectIdx = 0;
  for (size_t i = 0; i < mInputDevices.size(); ++i)
  {
    SendDlgItemMessage(hwnd, IDC_INPUT_DEV_COMBO, CB_ADDSTRING, 0,
                       reinterpret_cast<LPARAM>(mInputDevices[i].name.c_str()));
    if (mInputDevices[i].name == mState.inputDeviceName)
      selectIdx = static_cast<int>(i);
  }

  SendDlgItemMessage(hwnd, IDC_INPUT_DEV_COMBO, CB_SETCURSEL, selectIdx, 0);
}

void SettingsDialogSWELL::PopulateOutputDeviceList(HWND hwnd)
{
  SendDlgItemMessage(hwnd, IDC_OUTPUT_DEV_COMBO, CB_RESETCONTENT, 0, 0);

  int selectIdx = 0;
  for (size_t i = 0; i < mOutputDevices.size(); ++i)
  {
    SendDlgItemMessage(hwnd, IDC_OUTPUT_DEV_COMBO, CB_ADDSTRING, 0,
                       reinterpret_cast<LPARAM>(mOutputDevices[i].name.c_str()));
    if (mOutputDevices[i].name == mState.outputDeviceName)
      selectIdx = static_cast<int>(i);
  }

  SendDlgItemMessage(hwnd, IDC_OUTPUT_DEV_COMBO, CB_SETCURSEL, selectIdx, 0);
}

void SettingsDialogSWELL::PopulateSampleRateList(HWND hwnd)
{
  SendDlgItemMessage(hwnd, IDC_SAMPLE_RATE_COMBO, CB_RESETCONTENT, 0, 0);

  // Find matching sample rates between input and output devices
  std::vector<uint32_t> matchedRates;

  // Get input device sample rates
  for (const auto& dev : mInputDevices)
  {
    if (dev.name == mState.inputDeviceName)
    {
      mInputSampleRates = dev.sampleRates;
      break;
    }
  }

  // Get output device sample rates
  for (const auto& dev : mOutputDevices)
  {
    if (dev.name == mState.outputDeviceName)
    {
      mOutputSampleRates = dev.sampleRates;
      break;
    }
  }

  // Find common rates
  if (mInputSampleRates.empty())
  {
    matchedRates = mOutputSampleRates;
  }
  else if (mOutputSampleRates.empty())
  {
    matchedRates = mInputSampleRates;
  }
  else
  {
    for (uint32_t inRate : mInputSampleRates)
    {
      for (uint32_t outRate : mOutputSampleRates)
      {
        if (inRate == outRate)
          matchedRates.push_back(inRate);
      }
    }
  }

  // Add to combo box
  int selectIdx = 0;
  for (size_t i = 0; i < matchedRates.size(); ++i)
  {
    char buf[32];
    snprintf(buf, sizeof(buf), "%u", matchedRates[i]);
    LRESULT idx = SendDlgItemMessage(hwnd, IDC_SAMPLE_RATE_COMBO, CB_ADDSTRING, 0,
                                     reinterpret_cast<LPARAM>(buf));
    SendDlgItemMessage(hwnd, IDC_SAMPLE_RATE_COMBO, CB_SETITEMDATA, idx, matchedRates[i]);

    if (matchedRates[i] == mState.sampleRate)
      selectIdx = static_cast<int>(i);
  }

  SendDlgItemMessage(hwnd, IDC_SAMPLE_RATE_COMBO, CB_SETCURSEL, selectIdx, 0);
}

void SettingsDialogSWELL::PopulateBufferSizeList(HWND hwnd)
{
  SendDlgItemMessage(hwnd, IDC_BUFFER_SIZE_COMBO, CB_RESETCONTENT, 0, 0);

  int selectIdx = 0;
  for (int i = 0; i < kNumBufferSizes; ++i)
  {
    char buf[32];
    snprintf(buf, sizeof(buf), "%u", kBufferSizes[i]);
    SendDlgItemMessage(hwnd, IDC_BUFFER_SIZE_COMBO, CB_ADDSTRING, 0,
                       reinterpret_cast<LPARAM>(buf));

    if (kBufferSizes[i] == mState.bufferSize)
      selectIdx = i;
  }

  SendDlgItemMessage(hwnd, IDC_BUFFER_SIZE_COMBO, CB_SETCURSEL, selectIdx, 0);
}

void SettingsDialogSWELL::PopulateIOConfigList(HWND hwnd)
{
  SendDlgItemMessage(hwnd, IDC_IO_CONFIG_COMBO, CB_RESETCONTENT, 0, 0);

  for (size_t i = 0; i < mIOConfigs.size(); ++i)
  {
    std::string name = mIOConfigs[i].GetDisplayName();
    SendDlgItemMessage(hwnd, IDC_IO_CONFIG_COMBO, CB_ADDSTRING, 0,
                       reinterpret_cast<LPARAM>(name.c_str()));
  }

  int selectIdx = std::clamp(mState.selectedIOConfig, 0, static_cast<int>(mIOConfigs.size()) - 1);
  SendDlgItemMessage(hwnd, IDC_IO_CONFIG_COMBO, CB_SETCURSEL, selectIdx, 0);
}

void SettingsDialogSWELL::PopulateInputChannelLists(HWND hwnd)
{
  // TODO: Populate per-channel mapping combos
}

void SettingsDialogSWELL::PopulateOutputChannelLists(HWND hwnd)
{
  // TODO: Populate per-channel mapping combos
}

void SettingsDialogSWELL::PopulateMidiInputList(HWND hwnd)
{
  SendDlgItemMessage(hwnd, IDC_MIDI_IN_COMBO, CB_RESETCONTENT, 0, 0);

  int selectIdx = 0;
  for (size_t i = 0; i < mMidiInputDevices.size(); ++i)
  {
    SendDlgItemMessage(hwnd, IDC_MIDI_IN_COMBO, CB_ADDSTRING, 0,
                       reinterpret_cast<LPARAM>(mMidiInputDevices[i].c_str()));
    if (mMidiInputDevices[i] == mState.midiInputDevice)
      selectIdx = static_cast<int>(i);
  }

  SendDlgItemMessage(hwnd, IDC_MIDI_IN_COMBO, CB_SETCURSEL, selectIdx, 0);
}

void SettingsDialogSWELL::PopulateMidiOutputList(HWND hwnd)
{
  SendDlgItemMessage(hwnd, IDC_MIDI_OUT_COMBO, CB_RESETCONTENT, 0, 0);

  int selectIdx = 0;
  for (size_t i = 0; i < mMidiOutputDevices.size(); ++i)
  {
    SendDlgItemMessage(hwnd, IDC_MIDI_OUT_COMBO, CB_ADDSTRING, 0,
                       reinterpret_cast<LPARAM>(mMidiOutputDevices[i].c_str()));
    if (mMidiOutputDevices[i] == mState.midiOutputDevice)
      selectIdx = static_cast<int>(i);
  }

  SendDlgItemMessage(hwnd, IDC_MIDI_OUT_COMBO, CB_SETCURSEL, selectIdx, 0);
}

void SettingsDialogSWELL::PopulateMidiChannelLists(HWND hwnd)
{
  // Input channel
  SendDlgItemMessage(hwnd, IDC_MIDI_IN_CHAN_COMBO, CB_RESETCONTENT, 0, 0);
  SendDlgItemMessage(hwnd, IDC_MIDI_IN_CHAN_COMBO, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("all"));
  for (int i = 1; i <= 16; ++i)
  {
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", i);
    SendDlgItemMessage(hwnd, IDC_MIDI_IN_CHAN_COMBO, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(buf));
  }
  SendDlgItemMessage(hwnd, IDC_MIDI_IN_CHAN_COMBO, CB_SETCURSEL, mState.midiInputChannel, 0);

  // Output channel
  SendDlgItemMessage(hwnd, IDC_MIDI_OUT_CHAN_COMBO, CB_RESETCONTENT, 0, 0);
  SendDlgItemMessage(hwnd, IDC_MIDI_OUT_CHAN_COMBO, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("all"));
  for (int i = 1; i <= 16; ++i)
  {
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", i);
    SendDlgItemMessage(hwnd, IDC_MIDI_OUT_CHAN_COMBO, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(buf));
  }
  SendDlgItemMessage(hwnd, IDC_MIDI_OUT_CHAN_COMBO, CB_SETCURSEL, mState.midiOutputChannel, 0);
}

void SettingsDialogSWELL::UpdateControlStates(HWND hwnd)
{
  // Show/hide input controls based on I/O config
  int ioIdx = mState.selectedIOConfig;
  bool hasInputs = false;

  if (ioIdx >= 0 && ioIdx < static_cast<int>(mIOConfigs.size()))
  {
    hasInputs = mIOConfigs[ioIdx].numInputs > 0;
  }

  // Enable/disable input device combo
  EnableWindow(GetDlgItem(hwnd, IDC_INPUT_DEV_COMBO), hasInputs);
}

void SettingsDialogSWELL::CreateLevelMeters(HWND hwnd)
{
  // Level meters will be drawn in a reserved area of the dialog
  // The actual RECT bounds will be determined by the dialog layout
}

void SettingsDialogSWELL::UpdateLevelMeters()
{
  if (!mAudioEngine)
    return;

  // Update input meters
  for (int ch = 0; ch < mNumInputMeters; ++ch)
  {
    float level = mAudioEngine->GetInputPeakLevel(ch);
    mInputMeters[ch].SetLevel(level);
  }

  // Update output meters
  for (int ch = 0; ch < mNumOutputMeters; ++ch)
  {
    float level = mAudioEngine->GetOutputPeakLevel(ch);
    mOutputMeters[ch].SetLevel(level);
  }
}

void SettingsDialogSWELL::PaintLevelMeters(HWND hwnd, HDC hdc)
{
  // Define meter areas (TODO: get from actual dialog layout)
  RECT clientRect;
  GetClientRect(hwnd, &clientRect);

  // Input meters on left side
  const int meterWidth = 16;
  const int meterHeight = 100;
  const int meterSpacing = 4;
  int startX = 10;
  int startY = 200; // Below other controls

  for (int ch = 0; ch < mNumInputMeters; ++ch)
  {
    RECT meterRect = {
      startX + ch * (meterWidth + meterSpacing),
      startY,
      startX + ch * (meterWidth + meterSpacing) + meterWidth,
      startY + meterHeight
    };
    mInputMeters[ch].Paint(hdc, meterRect, false);
  }

  // Output meters on right side
  startX = clientRect.right - 10 - mNumOutputMeters * (meterWidth + meterSpacing);

  for (int ch = 0; ch < mNumOutputMeters; ++ch)
  {
    RECT meterRect = {
      startX + ch * (meterWidth + meterSpacing),
      startY,
      startX + ch * (meterWidth + meterSpacing) + meterWidth,
      startY + meterHeight
    };
    mOutputMeters[ch].Paint(hdc, meterRect, false);
  }
}

void CALLBACK SettingsDialogSWELL::MeterTimerProc(HWND hwnd, UINT msg, UINT_PTR id, DWORD time)
{
  // Not used - timer handled in WM_TIMER
}

END_IPLUG_NAMESPACE
