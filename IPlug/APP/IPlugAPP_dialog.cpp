/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#include "IPlugAPP_platform.h"
#include "IPlugAPP_host.h"
#include "config.h"
#include "resource.h"

#ifdef OS_WIN
#include "asio.h"
#endif


using namespace iplug;

#if !defined NO_IGRAPHICS
#include "IGraphics.h"
using namespace igraphics;
#endif


// check the input and output devices, find matching srs
void IPlugAPPHost::PopulateSampleRateList(HWND hwndDlg, RtAudio::DeviceInfo* inputDevInfo, RtAudio::DeviceInfo* outputDevInfo)
{
  WDL_String buf;

  SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_SR, CB_RESETCONTENT, 0, 0);

  std::vector<int> matchedSRs;

  if (inputDevInfo->probed && outputDevInfo->probed)
  {
    for (int i=0; i<inputDevInfo->sampleRates.size(); i++)
    {
      for (int j=0; j<outputDevInfo->sampleRates.size(); j++)
      {
        if(inputDevInfo->sampleRates[i] == outputDevInfo->sampleRates[j])
          matchedSRs.push_back(inputDevInfo->sampleRates[i]);
      }
    }
  }

  for (int k=0; k<matchedSRs.size(); k++)
  {
    buf.SetFormatted(20, "%i", matchedSRs[k]);
    SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_SR, CB_ADDSTRING, 0, ItemStringUTF8(buf.Get()));
    SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_SR, CB_SETITEMDATA, k, (LPARAM) matchedSRs[k]);
  }
  
  WDL_String str;
  str.SetFormatted(32, "%i", mState.mAudioSR);

  LRESULT sridx = SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_SR, CB_FINDSTRINGEXACT, -1, ItemStringUTF8(str));
  SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_SR, CB_SETCURSEL, sridx, 0);
}

void IPlugAPPHost::PopulateAudioInputList(HWND hwndDlg, RtAudio::DeviceInfo* info)
{
  if(!info->probed)
    return;

  WDL_String buf;

  SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_IN_L, CB_RESETCONTENT, 0, 0);
  SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_IN_R, CB_RESETCONTENT, 0, 0);

  int i;

  for (i=0; i<info->inputChannels -1; i++)
  {
    buf.SetFormatted(20, "%i", i+1);
    SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_IN_L, CB_ADDSTRING, 0, ItemStringUTF8(buf));
    SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_IN_R, CB_ADDSTRING, 0, ItemStringUTF8(buf));
  }

  // TEMP
  buf.SetFormatted(20, "%i", i+1);
  SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_IN_R, CB_ADDSTRING, 0, ItemStringUTF8(buf));

  SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_IN_L, CB_SETCURSEL, mState.mAudioInChanL - 1, 0);
  SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_IN_R, CB_SETCURSEL, mState.mAudioInChanR - 1, 0);
}

void IPlugAPPHost::PopulateAudioOutputList(HWND hwndDlg, RtAudio::DeviceInfo* info)
{
  if(!info->probed)
    return;

  WDL_String buf;

  SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_OUT_L, CB_RESETCONTENT, 0, 0);
  SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_OUT_R, CB_RESETCONTENT, 0, 0);

  int i;

  for (i=0; i<info->outputChannels -1; i++)
  {
    buf.SetFormatted(20, "%i", i+1);
    SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_OUT_L, CB_ADDSTRING, 0, ItemStringUTF8(buf));
    SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_OUT_R, CB_ADDSTRING, 0, ItemStringUTF8(buf));
  }

  // TEMP
  buf.SetFormatted(20, "%i", i+1);
  SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_OUT_R, CB_ADDSTRING, 0, ItemStringUTF8(buf));

  SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_OUT_L, CB_SETCURSEL, mState.mAudioOutChanL - 1, 0);
  SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_OUT_R, CB_SETCURSEL, mState.mAudioOutChanR - 1, 0);
}

// This has to get called after any change to audio driver/in dev/out dev
void IPlugAPPHost::PopulateDriverSpecificControls(HWND hwndDlg)
{
#ifdef OS_WIN
  int driverType = (int) SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_DRIVER, CB_GETCURSEL, 0, 0);
  if(driverType == kDeviceASIO)
  {
    ComboBox_Enable(GetDlgItem(hwndDlg, IDC_COMBO_AUDIO_IN_DEV), FALSE);
    Button_Enable(GetDlgItem(hwndDlg, IDC_BUTTON_OS_DEV_SETTINGS), TRUE);
  }
  else
  {
    ComboBox_Enable(GetDlgItem(hwndDlg, IDC_COMBO_AUDIO_IN_DEV), TRUE);
    Button_Enable(GetDlgItem(hwndDlg, IDC_BUTTON_OS_DEV_SETTINGS), FALSE);
  }
#endif

  int indevidx = 0;
  int outdevidx = 0;

  SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_IN_DEV, CB_RESETCONTENT, 0, 0);
  SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_OUT_DEV, CB_RESETCONTENT, 0, 0);

  for (int i = 0; i<mAudioInputDevs.size(); i++)
  {
    SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_IN_DEV, CB_ADDSTRING, 0, ItemStringUTF8(GetAudioDeviceName(mAudioInputDevs[i])));

    if(!strcmp(GetAudioDeviceName(mAudioInputDevs[i]).c_str(), mState.mAudioInDev.Get()))
      indevidx = i;
  }

  for (int i = 0; i<mAudioOutputDevs.size(); i++)
  {
    SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_OUT_DEV, CB_ADDSTRING, 0, ItemStringUTF8(GetAudioDeviceName(mAudioOutputDevs[i])));

    if(!strcmp(GetAudioDeviceName(mAudioOutputDevs[i]).c_str(), mState.mAudioOutDev.Get()))
      outdevidx = i;
  }

#ifdef OS_WIN
  if(driverType == kDeviceASIO)
    SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_IN_DEV, CB_SETCURSEL, outdevidx, 0);
  else
#endif
    SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_IN_DEV, CB_SETCURSEL, indevidx, 0);

  SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_OUT_DEV, CB_SETCURSEL, outdevidx, 0);

  RtAudio::DeviceInfo inputDevInfo;
  RtAudio::DeviceInfo outputDevInfo;

  if (mAudioInputDevs.size())
  {
    inputDevInfo = mDAC->getDeviceInfo(mAudioInputDevs[indevidx]);
    PopulateAudioInputList(hwndDlg, &inputDevInfo);
  }

  if (mAudioOutputDevs.size())
  {
    outputDevInfo = mDAC->getDeviceInfo(mAudioOutputDevs[outdevidx]);
    PopulateAudioOutputList(hwndDlg, &outputDevInfo);
  }

  PopulateSampleRateList(hwndDlg, &inputDevInfo, &outputDevInfo);
}

void IPlugAPPHost::PopulateAudioDialogs(HWND hwndDlg)
{
  PopulateDriverSpecificControls(hwndDlg);

//  if (mState.mAudioInIsMono)
//  {
//    SendDlgItemMessageUTF8(hwndDlg, IDC_CB_MONO_INPUT, BM_SETCHECK, BST_CHECKED, 0);
//  }
//  else
//  {
//    SendDlgItemMessageUTF8(hwndDlg, IDC_CB_MONO_INPUT, BM_SETCHECK, BST_UNCHECKED, 0);
//  }

//  Populate buffer size combobox
  for (int i = 0; i< kNumBufferSizeOptions; i++)
  {
    SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_BUF_SIZE, CB_ADDSTRING, 0, ItemStringUTF8(kBufferSizeOptions[i]));
  }
  
  WDL_String str;
  str.SetFormatted(32, "%i", mState.mBufferSize);

  LRESULT iovsidx = SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_BUF_SIZE, CB_FINDSTRINGEXACT, -1, ItemStringUTF8(str));
  SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_BUF_SIZE, CB_SETCURSEL, iovsidx, 0);
}

bool IPlugAPPHost::PopulateMidiDialogs(HWND hwndDlg)
{
  if ( !mMidiIn || !mMidiOut )
    return false;
  else
  {
    for (int i=0; i<mMidiInputDevNames.size(); i++ )
    {
      SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_MIDI_IN_DEV, CB_ADDSTRING, 0, ItemStringUTF8(mMidiInputDevNames[i]));
    }

    LRESULT indevidx = SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_MIDI_IN_DEV, CB_FINDSTRINGEXACT, -1, ItemStringUTF8(mState.mMidiInDev));

    // if the midi port name wasn't found update the ini file, and set to off
    if(indevidx == -1)
    {
      mState.mMidiInDev.Set("off");
      UpdateINI();
      indevidx = 0;
    }

    SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_MIDI_IN_DEV, CB_SETCURSEL, indevidx, 0);

    for (int i=0; i<mMidiOutputDevNames.size(); i++ )
    {
      SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_MIDI_OUT_DEV, CB_ADDSTRING, 0, ItemStringUTF8(mMidiOutputDevNames[i]));
    }

    LRESULT outdevidx = SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_MIDI_OUT_DEV, CB_FINDSTRINGEXACT, -1, ItemStringUTF8(mState.mMidiOutDev));

    // if the midi port name wasn't found update the ini file, and set to off
    if(outdevidx == -1)
    {
      mState.mMidiOutDev.Set("off");
      UpdateINI();
      outdevidx = 0;
    }

    SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_MIDI_OUT_DEV, CB_SETCURSEL, outdevidx, 0);

    // Populate MIDI channel dialogs

    SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_MIDI_IN_CHAN, CB_ADDSTRING, 0, ItemStringUTF8("all"));
    SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_MIDI_OUT_CHAN, CB_ADDSTRING, 0, ItemStringUTF8("all"));

    WDL_String buf;

    for (int i=0; i<16; i++)
    {
      buf.SetFormatted(20, "%i", i+1);
      SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_MIDI_IN_CHAN, CB_ADDSTRING, 0, ItemStringUTF8(buf));
      SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_MIDI_OUT_CHAN, CB_ADDSTRING, 0, ItemStringUTF8(buf));
    }

    SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_MIDI_IN_CHAN, CB_SETCURSEL, (WPARAM) mState.mMidiInChan, 0);
    SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_MIDI_OUT_CHAN, CB_SETCURSEL, (WPARAM) mState.mMidiOutChan, 0);

    return true;
  }
}

#ifdef OS_WIN
void IPlugAPPHost::PopulatePreferencesDialog(HWND hwndDlg)
{
  SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_DRIVER, CB_ADDSTRING, 0, ItemStringUTF8("DirectSound"));
  SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_DRIVER, CB_ADDSTRING, 0, ItemStringUTF8("ASIO"));
  SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_DRIVER, CB_SETCURSEL, mState.mAudioDriverType, 0);
  PopulateAudioDialogs(hwndDlg);
  PopulateMidiDialogs(hwndDlg);
}

#elif defined OS_MAC
void IPlugAPPHost::PopulatePreferencesDialog(HWND hwndDlg)
{
  SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_DRIVER, CB_ADDSTRING, 0, ItemStringUTF8("CoreAudio"));
  //SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_DRIVER, CB_ADDSTRING, 0, ItemStringUTF8("Jack"));
  SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_DRIVER, CB_SETCURSEL, mState.mAudioDriverType, 0);

  PopulateAudioDialogs(hwndDlg);
  PopulateMidiDialogs(hwndDlg);
}
#else
  #error NOT IMPLEMENTED
#endif

WDL_DLGRET IPlugAPPHost::PreferencesDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  IPlugAPPHost* _this = sInstance.get();
  AppState& mState = _this->mState;
  AppState& mTempState = _this->mTempState;
  AppState& mActiveState = _this->mActiveState;
 
  int v = 0;

  switch(uMsg)
  {
    case WM_INITDIALOG:
      _this->PopulatePreferencesDialog(hwndDlg);
      mTempState = mState;
      
      return TRUE;

    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDOK:
          if(mActiveState != mState)
            _this->TryToChangeAudio();

          EndDialog(hwndDlg, IDOK); // INI file will be changed see MainDialogProc
          break;
        case IDAPPLY:
          _this->TryToChangeAudio();
          break;
        case IDCANCEL:
          EndDialog(hwndDlg, IDCANCEL);

          // if state has been changed reset to previous state, INI file won't be changed
          if (!_this->AudioSettingsInStateAreEqual(mState, mTempState)
              || !_this->MIDISettingsInStateAreEqual(mState, mTempState))
          {
            mState = mTempState;

            _this->TryToChangeAudioDriverType();
            _this->ProbeAudioIO();
            _this->TryToChangeAudio();
          }

          break;

        case IDC_COMBO_AUDIO_DRIVER:
          if (HIWORD(wParam) == CBN_SELCHANGE)
          {
            v = (int) SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_DRIVER, CB_GETCURSEL, 0, 0);

            if(v != mState.mAudioDriverType)
            {
              mState.mAudioDriverType = v;

              _this->TryToChangeAudioDriverType();
              _this->ProbeAudioIO();

              if (_this->mAudioInputDevs.size())
                mState.mAudioInDev.Set(_this->GetAudioDeviceName(_this->mAudioInputDevs[0]).c_str());

              if (_this->mAudioOutputDevs.size())
                mState.mAudioOutDev.Set(_this->GetAudioDeviceName(_this->mAudioOutputDevs[0]).c_str());

              // Reset IO
              mState.mAudioOutChanL = 1;
              mState.mAudioOutChanR = 2;

              _this->PopulateAudioDialogs(hwndDlg);
            }
          }
          break;

        case IDC_COMBO_AUDIO_IN_DEV:
          if (HIWORD(wParam) == CBN_SELCHANGE)
          {
            int idx = (int) SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_IN_DEV, CB_GETCURSEL, 0, 0);
            GetComboStringUTF8(mState.mAudioInDev, hwndDlg, IDC_COMBO_AUDIO_IN_DEV, idx);

            // Reset IO
            mState.mAudioInChanL = 1;
            mState.mAudioInChanR = 2;

            _this->PopulateDriverSpecificControls(hwndDlg);
          }
          break;

        case IDC_COMBO_AUDIO_OUT_DEV:
          if (HIWORD(wParam) == CBN_SELCHANGE)
          {
            int idx = (int) SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_OUT_DEV, CB_GETCURSEL, 0, 0);
            GetComboStringUTF8(mState.mAudioOutDev, hwndDlg, IDC_COMBO_AUDIO_OUT_DEV, idx);

            // Reset IO
            mState.mAudioOutChanL = 1;
            mState.mAudioOutChanR = 2;

            _this->PopulateDriverSpecificControls(hwndDlg);
          }
          break;

        case IDC_COMBO_AUDIO_IN_L:
          if (HIWORD(wParam) == CBN_SELCHANGE)
          {
            mState.mAudioInChanL = (int) SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_IN_L, CB_GETCURSEL, 0, 0) + 1;

            //TEMP
            mState.mAudioInChanR = mState.mAudioInChanL + 1;
            SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_IN_R, CB_SETCURSEL, mState.mAudioInChanR - 1, 0);
            //
          }
          break;

        case IDC_COMBO_AUDIO_IN_R:
          if (HIWORD(wParam) == CBN_SELCHANGE)
            SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_IN_R, CB_SETCURSEL, mState.mAudioInChanR - 1, 0);  // TEMP
                mState.mAudioInChanR = (int) SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_IN_R, CB_GETCURSEL, 0, 0);
          break;

        case IDC_COMBO_AUDIO_OUT_L:
          if (HIWORD(wParam) == CBN_SELCHANGE)
          {
            mState.mAudioOutChanL = (int) SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_OUT_L, CB_GETCURSEL, 0, 0) + 1;

            //TEMP
            mState.mAudioOutChanR = mState.mAudioOutChanL + 1;
            SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_OUT_R, CB_SETCURSEL, mState.mAudioOutChanR - 1, 0);
            //
          }
          break;

        case IDC_COMBO_AUDIO_OUT_R:
          if (HIWORD(wParam) == CBN_SELCHANGE)
            SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_OUT_R, CB_SETCURSEL, mState.mAudioOutChanR - 1, 0);  // TEMP
                mState.mAudioOutChanR = (int) SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_OUT_R, CB_GETCURSEL, 0, 0);
          break;

//        case IDC_CB_MONO_INPUT:
//          if (SendDlgItemMessageUTF8(hwndDlg, IDC_CB_MONO_INPUT, BM_GETCHECK, 0, 0) == BST_CHECKED)
//            mState.mAudioInIsMono = 1;
//          else
//            mState.mAudioInIsMono = 0;
//          break;

        case IDC_COMBO_AUDIO_BUF_SIZE: // follow through
          if (HIWORD(wParam) == CBN_SELCHANGE)
          {
            int iovsidx = (int) SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_BUF_SIZE, CB_GETCURSEL, 0, 0);
            mState.mBufferSize = atoi(kBufferSizeOptions[iovsidx].c_str());
          }
          break;
        case IDC_COMBO_AUDIO_SR:
          if (HIWORD(wParam) == CBN_SELCHANGE)
          {
            int idx = (int) SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_SR, CB_GETCURSEL, 0, 0);
            mState.mAudioSR = (uint32_t) SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_AUDIO_SR, CB_GETITEMDATA, idx, 0);
          }
          break;

        case IDC_BUTTON_OS_DEV_SETTINGS:
          if (HIWORD(wParam) == BN_CLICKED) {
            #ifdef OS_WIN
            if( (_this->mState.mAudioDriverType == kDeviceASIO) && (_this->mDAC->isStreamRunning() == true)) // TODO: still not right
              ASIOControlPanel();
            #elif defined OS_MAC
            if(SWELL_GetOSXVersion() >= 0x1200) {
              system("open \"/System/Applications/Utilities/Audio MIDI Setup.app\"");
            } else {
              system("open \"/Applications/Utilities/Audio MIDI Setup.app\"");
            }
            #else
              #error NOT IMPLEMENTED
            #endif
          }
          break;

        case IDC_COMBO_MIDI_IN_DEV:
          if (HIWORD(wParam) == CBN_SELCHANGE)
          {
            int idx = (int) SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_MIDI_IN_DEV, CB_GETCURSEL, 0, 0);
            GetComboStringUTF8(mState.mMidiInDev, hwndDlg, IDC_COMBO_MIDI_IN_DEV, idx);
            _this->SelectMIDIDevice(ERoute::kInput, mState.mMidiInDev.Get());
          }
          break;

        case IDC_COMBO_MIDI_OUT_DEV:
          if (HIWORD(wParam) == CBN_SELCHANGE)
          {
            int idx = (int) SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_MIDI_OUT_DEV, CB_GETCURSEL, 0, 0);
            GetComboStringUTF8(mState.mMidiOutDev, hwndDlg, IDC_COMBO_MIDI_OUT_DEV, idx);
            _this->SelectMIDIDevice(ERoute::kOutput, mState.mMidiOutDev.Get());
          }
          break;

        case IDC_COMBO_MIDI_IN_CHAN:
          if (HIWORD(wParam) == CBN_SELCHANGE)
            mState.mMidiInChan = (int) SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_MIDI_IN_CHAN, CB_GETCURSEL, 0, 0);
          break;

        case IDC_COMBO_MIDI_OUT_CHAN:
          if (HIWORD(wParam) == CBN_SELCHANGE)
            mState.mMidiOutChan = (int) SendDlgItemMessageUTF8(hwndDlg, IDC_COMBO_MIDI_OUT_CHAN, CB_GETCURSEL, 0, 0);
          break;

        default:
          break;
      }
      break;
    default:
      return FALSE;
  }
  return TRUE;
}

static void ClientResize(HWND hWnd, int nWidth, int nHeight)
{
  RECT rcClient, rcWindow;
  POINT ptDiff;
  int screenwidth, screenheight;
  int x, y;
  
  screenwidth  = GetSystemMetrics(SM_CXSCREEN);
  screenheight = GetSystemMetrics(SM_CYSCREEN);
  x = (screenwidth / 2) - (nWidth / 2);
  y = (screenheight / 2) - (nHeight / 2);
  
  GetClientRect(hWnd, &rcClient);
  GetWindowRect(hWnd, &rcWindow);

  ptDiff.x = (rcWindow.right - rcWindow.left) - rcClient.right;
  ptDiff.y = (rcWindow.bottom - rcWindow.top) - rcClient.bottom;
  
  SetWindowPos(hWnd, 0, x, y, nWidth + ptDiff.x, nHeight + ptDiff.y, 0);
}

#ifdef OS_WIN 
extern float GetScaleForHWND(HWND hWnd);
#endif

//static
WDL_DLGRET IPlugAPPHost::MainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  IPlugAPPHost* pAppHost = IPlugAPPHost::sInstance.get();

  int width = 0;
  int height = 0;

  switch (uMsg)
  {
    case WM_INITDIALOG:
    {
      gHWND = hwndDlg;
      IPlugAPP* pPlug = pAppHost->GetPlug();

      if (!pAppHost->OpenWindow(gHWND))
        DBGMSG("couldn't attach gui\n");

      width = pPlug->GetEditorWidth();
      height = pPlug->GetEditorHeight();

      ClientResize(hwndDlg, width, height);

      ShowWindow(hwndDlg, SW_SHOW);
      return 1;
    }
    case WM_DESTROY:
      pAppHost->CloseWindow();
      gHWND = NULL;
      IPlugAPPHost::sInstance = nullptr;
      
      #ifdef OS_WIN
      PostQuitMessage(0);
      #else
      SWELL_PostQuitMessage(hwndDlg);
      #endif

      return 0;
    case WM_CLOSE:
      DestroyWindow(hwndDlg);
      return 0;
    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case ID_QUIT:
        {
          DestroyWindow(hwndDlg);
          return 0;
        }
        case ID_ABOUT:
        {
          IPlugAPP* pPlug = pAppHost->GetPlug();
          
          bool pluginOpensAboutBox = pPlug->OnHostRequestingAboutBox();
          
          if (pluginOpensAboutBox == false)
          {
            WDL_String info;
            info.Append(PLUG_COPYRIGHT_STR"\nBuilt on " __DATE__);
            MessageBoxUTF8(hwndDlg, info.Get(), PLUG_NAME, MB_OK);
          }

          return 0;
        }
        case ID_HELP:
        {
          IPlugAPP* pPlug = pAppHost->GetPlug();

          bool pluginOpensHelp = pPlug->OnHostRequestingProductHelp();

          if (pluginOpensHelp == false)
          {
            MessageBoxUTF8(hwndDlg, "See the manual", PLUG_NAME, MB_OK);
          }
          return 0;
        }
        case ID_PREFERENCES:
        {
          INT_PTR ret = DialogBox(gHINSTANCE, MAKEINTRESOURCE(IDD_DIALOG_PREF), hwndDlg, IPlugAPPHost::PreferencesDlgProc);

          if(ret == IDOK)
            pAppHost->UpdateINI();

          return 0;
        }
#if defined _DEBUG && !defined NO_IGRAPHICS
        case ID_LIVE_EDIT:
        {
          IGEditorDelegate* pPlug = dynamic_cast<IGEditorDelegate*>(pAppHost->GetPlug());
        
          if(pPlug)
          {
            IGraphics* pGraphics = pPlug->GetUI();
            
            if(pGraphics)
            {
              bool enabled = pGraphics->LiveEditEnabled();
              pGraphics->EnableLiveEdit(!enabled);
              CheckMenuItem(GetCurrentMenu(gHWND), ID_LIVE_EDIT, (MF_BYCOMMAND | enabled) ? MF_UNCHECKED : MF_CHECKED);
            }
          }
          
          return 0;
        }
        case ID_SHOW_DRAWN:
        {
          IGEditorDelegate* pPlug = dynamic_cast<IGEditorDelegate*>(pAppHost->GetPlug());
          
          if(pPlug)
          {
            IGraphics* pGraphics = pPlug->GetUI();
            
            if(pGraphics)
            {
              bool enabled = pGraphics->ShowAreaDrawnEnabled();
              pGraphics->ShowAreaDrawn(!enabled);
              CheckMenuItem(GetCurrentMenu(gHWND), ID_SHOW_DRAWN, (MF_BYCOMMAND | enabled) ? MF_UNCHECKED : MF_CHECKED);
            }
          }
          
          return 0;
        }
        case ID_SHOW_BOUNDS:
        {
          IGEditorDelegate* pPlug = dynamic_cast<IGEditorDelegate*>(pAppHost->GetPlug());
          
          if(pPlug)
          {
            IGraphics* pGraphics = pPlug->GetUI();
            
            if(pGraphics)
            {
              bool enabled = pGraphics->ShowControlBoundsEnabled();
              pGraphics->ShowControlBounds(!enabled);
              CheckMenuItem(GetCurrentMenu(gHWND), ID_SHOW_BOUNDS, (MF_BYCOMMAND | enabled) ? MF_UNCHECKED : MF_CHECKED);
            }
          }
          
          return 0;
        }
        case ID_SHOW_FPS:
        {
          IGEditorDelegate* pPlug = dynamic_cast<IGEditorDelegate*>(pAppHost->GetPlug());
          
          if(pPlug)
          {
            IGraphics* pGraphics = pPlug->GetUI();
            
            if(pGraphics)
            {
              bool enabled = pGraphics->ShowingFPSDisplay();
              pGraphics->ShowFPSDisplay(!enabled);
              CheckMenuItem(GetCurrentMenu(gHWND), ID_SHOW_FPS, (MF_BYCOMMAND | enabled) ? MF_UNCHECKED : MF_CHECKED);
            }
          }
          
          return 0;
        }
#endif
      }
      return 0;
    case WM_GETMINMAXINFO:
    {
      if(!pAppHost)
        return 1;
      
      IPlugAPP* pPlug = pAppHost->GetPlug();

      MINMAXINFO* mmi = (MINMAXINFO*) lParam;
      mmi->ptMinTrackSize.x = pPlug->GetMinWidth();
      mmi->ptMinTrackSize.y = pPlug->GetMinHeight();
      mmi->ptMaxTrackSize.x = pPlug->GetMaxWidth();
      mmi->ptMaxTrackSize.y = pPlug->GetMaxHeight();

#ifdef OS_WIN 
      float scale = GetScaleForHWND(hwndDlg);
      mmi->ptMinTrackSize.x = static_cast<LONG>(static_cast<float>(mmi->ptMinTrackSize.x) * scale);
      mmi->ptMinTrackSize.y = static_cast<LONG>(static_cast<float>(mmi->ptMinTrackSize.y) * scale);
      mmi->ptMaxTrackSize.x = static_cast<LONG>(static_cast<float>(mmi->ptMaxTrackSize.x) * scale);
      mmi->ptMaxTrackSize.y = static_cast<LONG>(static_cast<float>(mmi->ptMaxTrackSize.y) * scale);
#endif
      
      return 0;
    }
#ifdef OS_WIN
    case WM_DPICHANGED:
    {
      WORD dpi = HIWORD(wParam);
      RECT* rect = (RECT*)lParam;
      float scale = GetScaleForHWND(hwndDlg);

      POINT ptDiff;
      RECT rcClient;
      RECT rcWindow;

      GetClientRect(hwndDlg, &rcClient);
      GetWindowRect(hwndDlg, &rcWindow);

      ptDiff.x = (rcWindow.right - rcWindow.left) - rcClient.right;
      ptDiff.y = (rcWindow.bottom - rcWindow.top) - rcClient.bottom;

#ifndef NO_IGRAPHICS
      IGEditorDelegate* pPlug = dynamic_cast<IGEditorDelegate*>(pAppHost->GetPlug());

      if (pPlug)
      {
        IGraphics* pGraphics = pPlug->GetUI();

        if (pGraphics)
        {
          pGraphics->SetScreenScale(scale);
        }
      }
#else
      IEditorDelegate* pPlug = dynamic_cast<IEditorDelegate*>(pAppHost->GetPlug());
#endif

      int w = pPlug->GetEditorWidth(); 
      int h = pPlug->GetEditorHeight();

      SetWindowPos(hwndDlg, 0, rect->left, rect->top, w + ptDiff.x, h + ptDiff.y, 0);

      return 0;
    }
#endif
    case WM_SIZE:
    {
      IPlugAPP* pPlug = pAppHost->GetPlug();

      switch (LOWORD(wParam))
      {
      case SIZE_RESTORED:
      case SIZE_MAXIMIZED:
      {
        RECT r;
        GetClientRect(hwndDlg, &r);
        float scale = 1.f;
        #ifdef OS_WIN 
        scale = GetScaleForHWND(hwndDlg);
        #endif
        pPlug->OnParentWindowResize(static_cast<int>(r.right / scale), static_cast<int>(r.bottom / scale));
        return 1;
      }
      default:
        return 0;
      }
    }
  }
  return 0;
}
