/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for more info.

 ==============================================================================
*/

#include "SettingsDialogSWELL.h"
#include "../Audio/AudioEngine.h"
#include "../Resources/resource.h"

#include <algorithm>
#include <cstring>

#ifdef OS_WIN
extern HINSTANCE gHINSTANCE;
#endif

BEGIN_IPLUG_NAMESPACE

// Buffer size options
static const uint32_t kBufferSizes[] = { 32, 64, 96, 128, 192, 256, 512, 1024, 2048, 4096, 8192 };
static const int kNumBufferSizes = sizeof(kBufferSizes) / sizeof(kBufferSizes[0]);

// Timer ID for level meter updates
static const UINT_PTR kMeterTimerId = 1001;
static const UINT kMeterTimerInterval = 50; // 20 Hz refresh

// Channel combo box ID arrays for easy iteration
static const int kInputChannelCombos[] = {
  IDC_COMBO_IN_CH1, IDC_COMBO_IN_CH2, IDC_COMBO_IN_CH3, IDC_COMBO_IN_CH4,
  IDC_COMBO_IN_CH5, IDC_COMBO_IN_CH6, IDC_COMBO_IN_CH7, IDC_COMBO_IN_CH8
};
static const int kInputChannelLabels[] = {
  IDC_STATIC_IN_CH1, IDC_STATIC_IN_CH2, IDC_STATIC_IN_CH3, IDC_STATIC_IN_CH4,
  IDC_STATIC_IN_CH5, IDC_STATIC_IN_CH6, IDC_STATIC_IN_CH7, IDC_STATIC_IN_CH8
};
static const int kOutputChannelCombos[] = {
  IDC_COMBO_OUT_CH1, IDC_COMBO_OUT_CH2, IDC_COMBO_OUT_CH3, IDC_COMBO_OUT_CH4,
  IDC_COMBO_OUT_CH5, IDC_COMBO_OUT_CH6, IDC_COMBO_OUT_CH7, IDC_COMBO_OUT_CH8
};
static const int kOutputChannelLabels[] = {
  IDC_STATIC_OUT_CH1, IDC_STATIC_OUT_CH2, IDC_STATIC_OUT_CH3, IDC_STATIC_OUT_CH4,
  IDC_STATIC_OUT_CH5, IDC_STATIC_OUT_CH6, IDC_STATIC_OUT_CH7, IDC_STATIC_OUT_CH8
};
static const int kMaxChannelControls = 8;

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
  if (mMeterTimerId && mDialogHwnd)
  {
    KillTimer(mDialogHwnd, mMeterTimerId);
    mMeterTimerId = 0;
  }
}

bool SettingsDialogSWELL::ShowModal(void* parentWindow)
{
  mOriginalState = mState;

#ifdef OS_WIN
  INT_PTR result = DialogBoxParamA(
    gHINSTANCE,
    MAKEINTRESOURCEA(IDD_DIALOG_PREF),
    static_cast<HWND>(parentWindow),
    &SettingsDialogSWELL::DialogProc,
    reinterpret_cast<LPARAM>(this)
  );
  return (result == IDOK);
#else
  // SWELL implementation for macOS/Linux
  // TODO: Use SWELL_DialogBox or create dialog programmatically
  return false;
#endif
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

    // If no mappings, use I/O config
    if (mNumInputMeters == 0 && !mIOConfigs.empty())
    {
      int ioIdx = std::clamp(mState.selectedIOConfig, 0, static_cast<int>(mIOConfigs.size()) - 1);
      mNumInputMeters = std::min(mIOConfigs[ioIdx].numInputs, kMaxMeters);
      mNumOutputMeters = std::min(mIOConfigs[ioIdx].numOutputs, kMaxMeters);
    }
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
      // Populate all controls
      PopulateDriverList(hwnd);
      PopulateInputDeviceList(hwnd);
      PopulateOutputDeviceList(hwnd);
      PopulateSampleRateList(hwnd);
      PopulateBufferSizeList(hwnd);
      PopulateIOConfigList(hwnd);
      PopulateInputChannelLists(hwnd);
      PopulateOutputChannelLists(hwnd);
      PopulateMidiInputList(hwnd);
      PopulateMidiOutputList(hwnd);
      PopulateMidiChannelLists(hwnd);
      UpdateControlStates(hwnd);

      // Start meter timer
      mMeterTimerId = SetTimer(hwnd, kMeterTimerId, kMeterTimerInterval, nullptr);

      return TRUE;
    }

    case WM_TIMER:
      if (wParam == kMeterTimerId)
      {
        UpdateLevelMeters();
        // Invalidate meter controls to trigger repaint
        InvalidateRect(GetDlgItem(hwnd, IDC_STATIC_INPUT_METERS), nullptr, FALSE);
        InvalidateRect(GetDlgItem(hwnd, IDC_STATIC_OUTPUT_METERS), nullptr, FALSE);
      }
      return TRUE;

    case WM_DRAWITEM:
    {
      LPDRAWITEMSTRUCT pDIS = reinterpret_cast<LPDRAWITEMSTRUCT>(lParam);
      if (pDIS->CtlID == IDC_STATIC_INPUT_METERS)
      {
        // Draw input meters
        int meterWidth = (pDIS->rcItem.right - pDIS->rcItem.left) / std::max(mNumInputMeters, 1);
        for (int ch = 0; ch < mNumInputMeters; ++ch)
        {
          RECT meterRect = pDIS->rcItem;
          meterRect.left += ch * meterWidth + 2;
          meterRect.right = meterRect.left + meterWidth - 4;
          mInputMeters[ch].Paint(pDIS->hDC, meterRect, false);
        }
        return TRUE;
      }
      else if (pDIS->CtlID == IDC_STATIC_OUTPUT_METERS)
      {
        // Draw output meters
        int meterWidth = (pDIS->rcItem.right - pDIS->rcItem.left) / std::max(mNumOutputMeters, 1);
        for (int ch = 0; ch < mNumOutputMeters; ++ch)
        {
          RECT meterRect = pDIS->rcItem;
          meterRect.left += ch * meterWidth + 2;
          meterRect.right = meterRect.left + meterWidth - 4;
          mOutputMeters[ch].Paint(pDIS->hDC, meterRect, false);
        }
        return TRUE;
      }
      break;
    }

    case WM_COMMAND:
    {
      int ctrlId = LOWORD(wParam);
      int notifyCode = HIWORD(wParam);

      switch (ctrlId)
      {
        case IDOK:
          if (OnApply)
          {
            if (!OnApply(mState))
            {
              // Apply failed - stay in dialog
              return TRUE;
            }
          }
          KillTimer(hwnd, mMeterTimerId);
          mMeterTimerId = 0;
          EndDialog(hwnd, IDOK);
          return TRUE;

        case IDCANCEL:
          mState = mOriginalState;
          if (OnApply)
            OnApply(mState);  // Restore original settings
          KillTimer(hwnd, mMeterTimerId);
          mMeterTimerId = 0;
          EndDialog(hwnd, IDCANCEL);
          return TRUE;

        case IDAPPLY:
          if (OnApply)
            OnApply(mState);
          return TRUE;

        case IDC_COMBO_AUDIO_DRIVER:
          if (notifyCode == CBN_SELCHANGE)
          {
            int idx = static_cast<int>(SendDlgItemMessage(hwnd, IDC_COMBO_AUDIO_DRIVER, CB_GETCURSEL, 0, 0));
            auto apis = GetAvailableAudioApis();
            if (idx >= 0 && idx < static_cast<int>(apis.size()))
            {
              mState.audioApi = apis[idx];
              if (OnDriverChange)
                OnDriverChange(mState.audioApi);
              PopulateInputDeviceList(hwnd);
              PopulateOutputDeviceList(hwnd);
              PopulateSampleRateList(hwnd);
              UpdateControlStates(hwnd);
            }
          }
          return TRUE;

        case IDC_COMBO_AUDIO_IN_DEV:
          if (notifyCode == CBN_SELCHANGE)
          {
            int idx = static_cast<int>(SendDlgItemMessage(hwnd, IDC_COMBO_AUDIO_IN_DEV, CB_GETCURSEL, 0, 0));
            if (idx >= 0 && idx < static_cast<int>(mInputDevices.size()))
            {
              mState.inputDeviceName = mInputDevices[idx].name;
              mInputSampleRates = mInputDevices[idx].sampleRates;
              PopulateSampleRateList(hwnd);
              PopulateInputChannelLists(hwnd);
              if (OnDeviceChange)
                OnDeviceChange();
            }
          }
          return TRUE;

        case IDC_COMBO_AUDIO_OUT_DEV:
          if (notifyCode == CBN_SELCHANGE)
          {
            int idx = static_cast<int>(SendDlgItemMessage(hwnd, IDC_COMBO_AUDIO_OUT_DEV, CB_GETCURSEL, 0, 0));
            if (idx >= 0 && idx < static_cast<int>(mOutputDevices.size()))
            {
              mState.outputDeviceName = mOutputDevices[idx].name;
              mOutputSampleRates = mOutputDevices[idx].sampleRates;
              PopulateSampleRateList(hwnd);
              PopulateOutputChannelLists(hwnd);
              if (OnDeviceChange)
                OnDeviceChange();
            }
          }
          return TRUE;

        case IDC_COMBO_AUDIO_SR:
          if (notifyCode == CBN_SELCHANGE)
          {
            int idx = static_cast<int>(SendDlgItemMessage(hwnd, IDC_COMBO_AUDIO_SR, CB_GETCURSEL, 0, 0));
            uint32_t sr = static_cast<uint32_t>(SendDlgItemMessage(hwnd, IDC_COMBO_AUDIO_SR, CB_GETITEMDATA, idx, 0));
            mState.sampleRate = sr;
          }
          return TRUE;

        case IDC_COMBO_AUDIO_BUF_SIZE:
          if (notifyCode == CBN_SELCHANGE)
          {
            int idx = static_cast<int>(SendDlgItemMessage(hwnd, IDC_COMBO_AUDIO_BUF_SIZE, CB_GETCURSEL, 0, 0));
            if (idx >= 0 && idx < kNumBufferSizes)
              mState.bufferSize = kBufferSizes[idx];
          }
          return TRUE;

        case IDC_COMBO_IO_CONFIG:
          if (notifyCode == CBN_SELCHANGE)
          {
            int idx = static_cast<int>(SendDlgItemMessage(hwnd, IDC_COMBO_IO_CONFIG, CB_GETCURSEL, 0, 0));
            mState.selectedIOConfig = idx;
            if (idx >= 0 && idx < static_cast<int>(mIOConfigs.size()))
            {
              // Update channel count and recreate default mappings
              mState.CreateDefaultInputMappings(mIOConfigs[idx].numInputs);
              mState.CreateDefaultOutputMappings(mIOConfigs[idx].numOutputs);
              mNumInputMeters = std::min(mIOConfigs[idx].numInputs, kMaxMeters);
              mNumOutputMeters = std::min(mIOConfigs[idx].numOutputs, kMaxMeters);
            }
            UpdateControlStates(hwnd);
            PopulateInputChannelLists(hwnd);
            PopulateOutputChannelLists(hwnd);
          }
          return TRUE;

        case IDC_BUTTON_OS_DEV_SETTINGS:
        {
#ifdef OS_WIN
          // Open ASIO control panel if using ASIO
          if (mState.audioApi == EAudioApi::kASIO)
          {
            // TODO: Call ASIOControlPanel() if stream is running
          }
#endif
          return TRUE;
        }

        case IDC_COMBO_MIDI_IN_DEV:
          if (notifyCode == CBN_SELCHANGE)
          {
            int idx = static_cast<int>(SendDlgItemMessage(hwnd, IDC_COMBO_MIDI_IN_DEV, CB_GETCURSEL, 0, 0));
            if (idx >= 0 && idx < static_cast<int>(mMidiInputDevices.size()))
              mState.midiInputDevice = mMidiInputDevices[idx];
          }
          return TRUE;

        case IDC_COMBO_MIDI_OUT_DEV:
          if (notifyCode == CBN_SELCHANGE)
          {
            int idx = static_cast<int>(SendDlgItemMessage(hwnd, IDC_COMBO_MIDI_OUT_DEV, CB_GETCURSEL, 0, 0));
            if (idx >= 0 && idx < static_cast<int>(mMidiOutputDevices.size()))
              mState.midiOutputDevice = mMidiOutputDevices[idx];
          }
          return TRUE;

        case IDC_COMBO_MIDI_IN_CHAN:
          if (notifyCode == CBN_SELCHANGE)
          {
            int idx = static_cast<int>(SendDlgItemMessage(hwnd, IDC_COMBO_MIDI_IN_CHAN, CB_GETCURSEL, 0, 0));
            mState.midiInputChannel = idx;
          }
          return TRUE;

        case IDC_COMBO_MIDI_OUT_CHAN:
          if (notifyCode == CBN_SELCHANGE)
          {
            int idx = static_cast<int>(SendDlgItemMessage(hwnd, IDC_COMBO_MIDI_OUT_CHAN, CB_GETCURSEL, 0, 0));
            mState.midiOutputChannel = idx;
          }
          return TRUE;

        default:
          // Handle channel mapping combos
          for (int ch = 0; ch < kMaxChannelControls; ++ch)
          {
            if (ctrlId == kInputChannelCombos[ch] && notifyCode == CBN_SELCHANGE)
            {
              int devCh = static_cast<int>(SendDlgItemMessage(hwnd, ctrlId, CB_GETCURSEL, 0, 0));
              if (ch < static_cast<int>(mState.inputMappings.size()))
                mState.inputMappings[ch].deviceChannel = devCh;
              return TRUE;
            }
            if (ctrlId == kOutputChannelCombos[ch] && notifyCode == CBN_SELCHANGE)
            {
              int devCh = static_cast<int>(SendDlgItemMessage(hwnd, ctrlId, CB_GETCURSEL, 0, 0));
              if (ch < static_cast<int>(mState.outputMappings.size()))
                mState.outputMappings[ch].deviceChannel = devCh;
              return TRUE;
            }
          }
          break;
      }
      break;
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
  SendDlgItemMessage(hwnd, IDC_COMBO_AUDIO_DRIVER, CB_RESETCONTENT, 0, 0);

  auto apis = GetAvailableAudioApis();
  int selectIdx = 0;

  for (size_t i = 0; i < apis.size(); ++i)
  {
    SendDlgItemMessageA(hwnd, IDC_COMBO_AUDIO_DRIVER, CB_ADDSTRING, 0,
                        reinterpret_cast<LPARAM>(GetAudioApiName(apis[i])));
    if (apis[i] == mState.audioApi)
      selectIdx = static_cast<int>(i);
  }

  SendDlgItemMessage(hwnd, IDC_COMBO_AUDIO_DRIVER, CB_SETCURSEL, selectIdx, 0);
}

void SettingsDialogSWELL::PopulateInputDeviceList(HWND hwnd)
{
  SendDlgItemMessage(hwnd, IDC_COMBO_AUDIO_IN_DEV, CB_RESETCONTENT, 0, 0);

  int selectIdx = 0;
  for (size_t i = 0; i < mInputDevices.size(); ++i)
  {
    SendDlgItemMessageA(hwnd, IDC_COMBO_AUDIO_IN_DEV, CB_ADDSTRING, 0,
                        reinterpret_cast<LPARAM>(mInputDevices[i].name.c_str()));
    if (mInputDevices[i].name == mState.inputDeviceName)
    {
      selectIdx = static_cast<int>(i);
      mInputSampleRates = mInputDevices[i].sampleRates;
    }
  }

  SendDlgItemMessage(hwnd, IDC_COMBO_AUDIO_IN_DEV, CB_SETCURSEL, selectIdx, 0);
}

void SettingsDialogSWELL::PopulateOutputDeviceList(HWND hwnd)
{
  SendDlgItemMessage(hwnd, IDC_COMBO_AUDIO_OUT_DEV, CB_RESETCONTENT, 0, 0);

  int selectIdx = 0;
  for (size_t i = 0; i < mOutputDevices.size(); ++i)
  {
    SendDlgItemMessageA(hwnd, IDC_COMBO_AUDIO_OUT_DEV, CB_ADDSTRING, 0,
                        reinterpret_cast<LPARAM>(mOutputDevices[i].name.c_str()));
    if (mOutputDevices[i].name == mState.outputDeviceName)
    {
      selectIdx = static_cast<int>(i);
      mOutputSampleRates = mOutputDevices[i].sampleRates;
    }
  }

  SendDlgItemMessage(hwnd, IDC_COMBO_AUDIO_OUT_DEV, CB_SETCURSEL, selectIdx, 0);
}

void SettingsDialogSWELL::PopulateSampleRateList(HWND hwnd)
{
  SendDlgItemMessage(hwnd, IDC_COMBO_AUDIO_SR, CB_RESETCONTENT, 0, 0);

  // Find common sample rates between input and output
  std::vector<uint32_t> matchedRates;

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
        {
          matchedRates.push_back(inRate);
          break;
        }
      }
    }
  }

  // Sort rates
  std::sort(matchedRates.begin(), matchedRates.end());

  int selectIdx = 0;
  for (size_t i = 0; i < matchedRates.size(); ++i)
  {
    char buf[32];
    snprintf(buf, sizeof(buf), "%u", matchedRates[i]);
    LRESULT idx = SendDlgItemMessageA(hwnd, IDC_COMBO_AUDIO_SR, CB_ADDSTRING, 0,
                                      reinterpret_cast<LPARAM>(buf));
    SendDlgItemMessage(hwnd, IDC_COMBO_AUDIO_SR, CB_SETITEMDATA, idx, matchedRates[i]);

    if (matchedRates[i] == mState.sampleRate)
      selectIdx = static_cast<int>(i);
  }

  SendDlgItemMessage(hwnd, IDC_COMBO_AUDIO_SR, CB_SETCURSEL, selectIdx, 0);
}

void SettingsDialogSWELL::PopulateBufferSizeList(HWND hwnd)
{
  SendDlgItemMessage(hwnd, IDC_COMBO_AUDIO_BUF_SIZE, CB_RESETCONTENT, 0, 0);

  int selectIdx = 6; // Default to 512
  for (int i = 0; i < kNumBufferSizes; ++i)
  {
    char buf[32];
    snprintf(buf, sizeof(buf), "%u", kBufferSizes[i]);
    SendDlgItemMessageA(hwnd, IDC_COMBO_AUDIO_BUF_SIZE, CB_ADDSTRING, 0,
                        reinterpret_cast<LPARAM>(buf));

    if (kBufferSizes[i] == mState.bufferSize)
      selectIdx = i;
  }

  SendDlgItemMessage(hwnd, IDC_COMBO_AUDIO_BUF_SIZE, CB_SETCURSEL, selectIdx, 0);
}

void SettingsDialogSWELL::PopulateIOConfigList(HWND hwnd)
{
  SendDlgItemMessage(hwnd, IDC_COMBO_IO_CONFIG, CB_RESETCONTENT, 0, 0);

  for (size_t i = 0; i < mIOConfigs.size(); ++i)
  {
    std::string name = mIOConfigs[i].GetDisplayName();
    SendDlgItemMessageA(hwnd, IDC_COMBO_IO_CONFIG, CB_ADDSTRING, 0,
                        reinterpret_cast<LPARAM>(name.c_str()));
  }

  int selectIdx = std::clamp(mState.selectedIOConfig, 0,
                             static_cast<int>(mIOConfigs.size()) - 1);
  SendDlgItemMessage(hwnd, IDC_COMBO_IO_CONFIG, CB_SETCURSEL, selectIdx, 0);
}

void SettingsDialogSWELL::PopulateInputChannelLists(HWND hwnd)
{
  // Get input device channel count
  int deviceChannels = 0;
  for (const auto& dev : mInputDevices)
  {
    if (dev.name == mState.inputDeviceName)
    {
      deviceChannels = dev.inputChannels;
      break;
    }
  }

  // Get plugin input channel count from current I/O config
  int pluginChannels = 0;
  if (mState.selectedIOConfig >= 0 &&
      mState.selectedIOConfig < static_cast<int>(mIOConfigs.size()))
  {
    pluginChannels = mIOConfigs[mState.selectedIOConfig].numInputs;
  }

  // Update each channel combo
  for (int ch = 0; ch < kMaxChannelControls; ++ch)
  {
    SendDlgItemMessage(hwnd, kInputChannelCombos[ch], CB_RESETCONTENT, 0, 0);

    bool visible = (ch < pluginChannels);
    ShowWindow(GetDlgItem(hwnd, kInputChannelCombos[ch]), visible ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hwnd, kInputChannelLabels[ch]), visible ? SW_SHOW : SW_HIDE);

    if (visible)
    {
      // Add device channels
      for (int devCh = 0; devCh < deviceChannels; ++devCh)
      {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", devCh + 1);
        SendDlgItemMessageA(hwnd, kInputChannelCombos[ch], CB_ADDSTRING, 0,
                            reinterpret_cast<LPARAM>(buf));
      }

      // Select current mapping
      int selectIdx = 0;
      if (ch < static_cast<int>(mState.inputMappings.size()))
        selectIdx = std::min(mState.inputMappings[ch].deviceChannel, deviceChannels - 1);
      SendDlgItemMessage(hwnd, kInputChannelCombos[ch], CB_SETCURSEL, selectIdx, 0);
    }
  }
}

void SettingsDialogSWELL::PopulateOutputChannelLists(HWND hwnd)
{
  // Get output device channel count
  int deviceChannels = 0;
  for (const auto& dev : mOutputDevices)
  {
    if (dev.name == mState.outputDeviceName)
    {
      deviceChannels = dev.outputChannels;
      break;
    }
  }

  // Get plugin output channel count from current I/O config
  int pluginChannels = 0;
  if (mState.selectedIOConfig >= 0 &&
      mState.selectedIOConfig < static_cast<int>(mIOConfigs.size()))
  {
    pluginChannels = mIOConfigs[mState.selectedIOConfig].numOutputs;
  }

  // Update each channel combo
  for (int ch = 0; ch < kMaxChannelControls; ++ch)
  {
    SendDlgItemMessage(hwnd, kOutputChannelCombos[ch], CB_RESETCONTENT, 0, 0);

    bool visible = (ch < pluginChannels);
    ShowWindow(GetDlgItem(hwnd, kOutputChannelCombos[ch]), visible ? SW_SHOW : SW_HIDE);
    ShowWindow(GetDlgItem(hwnd, kOutputChannelLabels[ch]), visible ? SW_SHOW : SW_HIDE);

    if (visible)
    {
      // Add device channels
      for (int devCh = 0; devCh < deviceChannels; ++devCh)
      {
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", devCh + 1);
        SendDlgItemMessageA(hwnd, kOutputChannelCombos[ch], CB_ADDSTRING, 0,
                            reinterpret_cast<LPARAM>(buf));
      }

      // Select current mapping
      int selectIdx = 0;
      if (ch < static_cast<int>(mState.outputMappings.size()))
        selectIdx = std::min(mState.outputMappings[ch].deviceChannel, deviceChannels - 1);
      SendDlgItemMessage(hwnd, kOutputChannelCombos[ch], CB_SETCURSEL, selectIdx, 0);
    }
  }
}

void SettingsDialogSWELL::PopulateMidiInputList(HWND hwnd)
{
  SendDlgItemMessage(hwnd, IDC_COMBO_MIDI_IN_DEV, CB_RESETCONTENT, 0, 0);

  int selectIdx = 0;
  for (size_t i = 0; i < mMidiInputDevices.size(); ++i)
  {
    SendDlgItemMessageA(hwnd, IDC_COMBO_MIDI_IN_DEV, CB_ADDSTRING, 0,
                        reinterpret_cast<LPARAM>(mMidiInputDevices[i].c_str()));
    if (mMidiInputDevices[i] == mState.midiInputDevice)
      selectIdx = static_cast<int>(i);
  }

  SendDlgItemMessage(hwnd, IDC_COMBO_MIDI_IN_DEV, CB_SETCURSEL, selectIdx, 0);
}

void SettingsDialogSWELL::PopulateMidiOutputList(HWND hwnd)
{
  SendDlgItemMessage(hwnd, IDC_COMBO_MIDI_OUT_DEV, CB_RESETCONTENT, 0, 0);

  int selectIdx = 0;
  for (size_t i = 0; i < mMidiOutputDevices.size(); ++i)
  {
    SendDlgItemMessageA(hwnd, IDC_COMBO_MIDI_OUT_DEV, CB_ADDSTRING, 0,
                        reinterpret_cast<LPARAM>(mMidiOutputDevices[i].c_str()));
    if (mMidiOutputDevices[i] == mState.midiOutputDevice)
      selectIdx = static_cast<int>(i);
  }

  SendDlgItemMessage(hwnd, IDC_COMBO_MIDI_OUT_DEV, CB_SETCURSEL, selectIdx, 0);
}

void SettingsDialogSWELL::PopulateMidiChannelLists(HWND hwnd)
{
  // Input channel
  SendDlgItemMessage(hwnd, IDC_COMBO_MIDI_IN_CHAN, CB_RESETCONTENT, 0, 0);
  SendDlgItemMessageA(hwnd, IDC_COMBO_MIDI_IN_CHAN, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("all"));
  for (int i = 1; i <= 16; ++i)
  {
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", i);
    SendDlgItemMessageA(hwnd, IDC_COMBO_MIDI_IN_CHAN, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(buf));
  }
  SendDlgItemMessage(hwnd, IDC_COMBO_MIDI_IN_CHAN, CB_SETCURSEL, mState.midiInputChannel, 0);

  // Output channel
  SendDlgItemMessage(hwnd, IDC_COMBO_MIDI_OUT_CHAN, CB_RESETCONTENT, 0, 0);
  SendDlgItemMessageA(hwnd, IDC_COMBO_MIDI_OUT_CHAN, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>("all"));
  for (int i = 1; i <= 16; ++i)
  {
    char buf[8];
    snprintf(buf, sizeof(buf), "%d", i);
    SendDlgItemMessageA(hwnd, IDC_COMBO_MIDI_OUT_CHAN, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(buf));
  }
  SendDlgItemMessage(hwnd, IDC_COMBO_MIDI_OUT_CHAN, CB_SETCURSEL, mState.midiOutputChannel, 0);
}

void SettingsDialogSWELL::UpdateControlStates(HWND hwnd)
{
  // Get current I/O config
  int numInputs = 0;
  if (mState.selectedIOConfig >= 0 &&
      mState.selectedIOConfig < static_cast<int>(mIOConfigs.size()))
  {
    numInputs = mIOConfigs[mState.selectedIOConfig].numInputs;
  }

  // Enable/disable input device combo based on whether plugin has inputs
  bool hasInputs = (numInputs > 0);
  EnableWindow(GetDlgItem(hwnd, IDC_COMBO_AUDIO_IN_DEV), hasInputs);

#ifdef OS_WIN
  // Enable ASIO config button only when using ASIO
  EnableWindow(GetDlgItem(hwnd, IDC_BUTTON_OS_DEV_SETTINGS),
               mState.audioApi == EAudioApi::kASIO);
#endif
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

END_IPLUG_NAMESPACE
