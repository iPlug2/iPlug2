/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#include "IPlugAPP_dialog.h"
#include "config.h"
#include "resource.h"

#ifdef OS_WIN
#include "asio.h"
#endif

using namespace iplug;

// check the input and output devices, find matching srs
void IPlugAPPDialog::PopulateSampleRateList(HWND hwndDlg, RtAudio::DeviceInfo* inputDevInfo, RtAudio::DeviceInfo* outputDevInfo)
{
  WDL_String buf;

  SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_SR,CB_RESETCONTENT,0,0);

  std::vector<int> matchedSRs;

  for (int i=0; i<inputDevInfo->sampleRates.size(); i++)
  {
    for (int j=0; j<outputDevInfo->sampleRates.size(); j++)
    {
      if (inputDevInfo->sampleRates[i] == outputDevInfo->sampleRates[j])
        matchedSRs.push_back(inputDevInfo->sampleRates[i]);
    }
  }

  for (int k=0; k<matchedSRs.size(); k++)
  {
    buf.SetFormatted(20, "%i", matchedSRs[k]);
    SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_SR,CB_ADDSTRING,0,(LPARAM)buf.Get());
    SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_SR,CB_SETITEMDATA,k,(LPARAM)matchedSRs[k]);
  }
  
  WDL_String str;
  str.SetFormatted(32, "%i", mSettings.mAudioSR);

  LRESULT sridx = SendDlgItemMessage(hwndDlg, IDC_COMBO_AUDIO_SR, CB_FINDSTRINGEXACT, -1, (LPARAM) str.Get());
  SendDlgItemMessage(hwndDlg, IDC_COMBO_AUDIO_SR, CB_SETCURSEL, sridx, 0);
}

void IPlugAPPDialog::PopulateAudioInputList(HWND hwndDlg, RtAudio::DeviceInfo* info)
{
  WDL_String buf;

  SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_IN_L,CB_RESETCONTENT,0,0);
  SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_IN_R,CB_RESETCONTENT,0,0);

  int i;

  for (i=0; i<info->inputChannels -1; i++)
  {
    buf.SetFormatted(20, "%i", i+1);
    SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_IN_L,CB_ADDSTRING,0,(LPARAM)buf.Get());
    SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_IN_R,CB_ADDSTRING,0,(LPARAM)buf.Get());
  }

  // TEMP
  buf.SetFormatted(20, "%i", i+1);
  SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_IN_R,CB_ADDSTRING,0,(LPARAM)buf.Get());

  SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_IN_L,CB_SETCURSEL, mSettings.mAudioInChanL - 1, 0);
  SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_IN_R,CB_SETCURSEL, mSettings.mAudioInChanR - 1, 0);
}

void IPlugAPPDialog::PopulateAudioOutputList(HWND hwndDlg, RtAudio::DeviceInfo* info)
{
  WDL_String buf;

  SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_OUT_L,CB_RESETCONTENT,0,0);
  SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_OUT_R,CB_RESETCONTENT,0,0);

  int i;

  for (i=0; i<info->outputChannels -1; i++)
  {
    buf.SetFormatted(20, "%i", i+1);
    SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_OUT_L,CB_ADDSTRING,0,(LPARAM)buf.Get());
    SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_OUT_R,CB_ADDSTRING,0,(LPARAM)buf.Get());
  }

  // TEMP
  buf.SetFormatted(20, "%i", i+1);
  SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_OUT_R,CB_ADDSTRING,0,(LPARAM)buf.Get());

  SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_OUT_L,CB_SETCURSEL, mSettings.mAudioOutChanL - 1, 0);
  SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_OUT_R,CB_SETCURSEL, mSettings.mAudioOutChanR - 1, 0);
}

// This has to get called after any change to audio driver/in dev/out dev
void IPlugAPPDialog::PopulateDriverSpecificControls(HWND hwndDlg)
{
#ifdef OS_WIN
  int driverType = (int) SendDlgItemMessage(hwndDlg, IDC_COMBO_AUDIO_DRIVER, CB_GETCURSEL, 0, 0);
  if (driverType == kDeviceASIO)
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

  SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_IN_DEV,CB_RESETCONTENT,0,0);
  SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_OUT_DEV,CB_RESETCONTENT,0,0);

  for (int i = 0; i<mAudioInputDevIDs.size(); i++)
  {
    SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_IN_DEV,CB_ADDSTRING,0,(LPARAM)mHost.GetAudioDeviceName(mAudioInputDevIDs[i]).c_str());

    if (std::string_view(mHost.GetAudioDeviceName(mAudioInputDevIDs[i])) == mSettings.mAudioInDev.Get())
      indevidx = i;
  }

  for (int i = 0; i<mAudioOutputDevIDs.size(); i++)
  {
    SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_OUT_DEV,CB_ADDSTRING,0,(LPARAM)mHost.GetAudioDeviceName(mAudioOutputDevIDs[i]).c_str());

    if (std::string_view(mHost.GetAudioDeviceName(mAudioOutputDevIDs[i])) == mSettings.mAudioOutDev.Get())
      outdevidx = i;
  }

#ifdef OS_WIN
  if (driverType == kDeviceASIO)
    SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_IN_DEV,CB_SETCURSEL, outdevidx, 0);
  else
#endif
    SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_IN_DEV,CB_SETCURSEL, indevidx, 0);

  SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_OUT_DEV,CB_SETCURSEL, outdevidx, 0);

  RtAudio::DeviceInfo inputDevInfo;
  RtAudio::DeviceInfo outputDevInfo;

  if (mAudioInputDevIDs.size())
  {
    inputDevInfo = mHost.GetDeviceInfo(mAudioInputDevIDs[indevidx]);
    PopulateAudioInputList(hwndDlg, &inputDevInfo);
  }

  if (mAudioOutputDevIDs.size())
  {
    outputDevInfo = mHost.GetDeviceInfo(mAudioOutputDevIDs[outdevidx]);
    PopulateAudioOutputList(hwndDlg, &outputDevInfo);
  }

  PopulateSampleRateList(hwndDlg, &inputDevInfo, &outputDevInfo);
}

void IPlugAPPDialog::PopulateAudioDialogs(HWND hwndDlg)
{
  PopulateDriverSpecificControls(hwndDlg);

//  if (mSettings.mAudioInIsMono)
//  {
//    SendDlgItemMessage(hwndDlg,IDC_CB_MONO_INPUT,BM_SETCHECK, BST_CHECKED,0);
//  }
//  else
//  {
//    SendDlgItemMessage(hwndDlg,IDC_CB_MONO_INPUT,BM_SETCHECK, BST_UNCHECKED,0);
//  }

//  Populate buffer size combobox
  for (int i = 0; i< kNumBufferSizeOptions; i++)
  {
    SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_BUF_SIZE,CB_ADDSTRING,0,(LPARAM)kBufferSizeOptions[i].c_str());
  }
  
  WDL_String str;
  str.SetFormatted(32, "%i", mSettings.mBufferSize);

  LRESULT iovsidx = SendDlgItemMessage(hwndDlg, IDC_COMBO_AUDIO_BUF_SIZE, CB_FINDSTRINGEXACT, -1, (LPARAM) str.Get());
  SendDlgItemMessage(hwndDlg, IDC_COMBO_AUDIO_BUF_SIZE, CB_SETCURSEL, iovsidx, 0);
}

bool IPlugAPPDialog::PopulateMidiDialogs(HWND hwndDlg)
{
  if (!mHost.IsMidiInitialised())
    return false;
  else
  {
    for (int i=0; i<mMidiInputDevNames.size(); i++)
    {
      SendDlgItemMessage(hwndDlg,IDC_COMBO_MIDI_IN_DEV,CB_ADDSTRING,0,(LPARAM)mMidiInputDevNames[i].c_str());
    }

    LRESULT indevidx = SendDlgItemMessage(hwndDlg,IDC_COMBO_MIDI_IN_DEV,CB_FINDSTRINGEXACT, -1, (LPARAM)mSettings.mMidiInDev.Get());

    // if the midi port name wasn't found update the ini file, and set to off
    if (indevidx == -1)
    {
      mSettings.mMidiInDev.Set("off");
      mHost.UpdateINI();
      indevidx = 0;
    }

    SendDlgItemMessage(hwndDlg,IDC_COMBO_MIDI_IN_DEV,CB_SETCURSEL, indevidx, 0);

    for (int i=0; i<mMidiOutputDevNames.size(); i++)
    {
      SendDlgItemMessage(hwndDlg,IDC_COMBO_MIDI_OUT_DEV,CB_ADDSTRING,0,(LPARAM)mMidiOutputDevNames[i].c_str());
    }

    LRESULT outdevidx = SendDlgItemMessage(hwndDlg,IDC_COMBO_MIDI_OUT_DEV,CB_FINDSTRINGEXACT, -1, (LPARAM)mSettings.mMidiOutDev.Get());

    // if the midi port name wasn't found update the ini file, and set to off
    if (outdevidx == -1)
    {
      mSettings.mMidiOutDev.Set("off");
      mHost.UpdateINI();
      outdevidx = 0;
    }

    SendDlgItemMessage(hwndDlg,IDC_COMBO_MIDI_OUT_DEV,CB_SETCURSEL, outdevidx, 0);

    // Populate MIDI channel dialogs

    SendDlgItemMessage(hwndDlg,IDC_COMBO_MIDI_IN_CHAN,CB_ADDSTRING,0,(LPARAM)"all");
    SendDlgItemMessage(hwndDlg,IDC_COMBO_MIDI_OUT_CHAN,CB_ADDSTRING,0,(LPARAM)"all");

    WDL_String buf;

    for (int i=0; i<16; i++)
    {
      buf.SetFormatted(20, "%i", i+1);
      SendDlgItemMessage(hwndDlg,IDC_COMBO_MIDI_IN_CHAN,CB_ADDSTRING,0,(LPARAM)buf.Get());
      SendDlgItemMessage(hwndDlg,IDC_COMBO_MIDI_OUT_CHAN,CB_ADDSTRING,0,(LPARAM)buf.Get());
    }

    SendDlgItemMessage(hwndDlg,IDC_COMBO_MIDI_IN_CHAN,CB_SETCURSEL, (LPARAM)mSettings.mMidiInChan, 0);
    SendDlgItemMessage(hwndDlg,IDC_COMBO_MIDI_OUT_CHAN,CB_SETCURSEL, (LPARAM)mSettings.mMidiOutChan, 0);

    return true;
  }
}

#ifdef OS_WIN
void IPlugAPPDialog::PopulatePreferencesDialog(HWND hwndDlg)
{
  SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_DRIVER,CB_ADDSTRING,0,(LPARAM)"DirectSound");
  SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_DRIVER,CB_ADDSTRING,0,(LPARAM)"ASIO");
  SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_DRIVER,CB_SETCURSEL, mSettings.mAudioDriverType, 0);

  PopulateAudioDialogs(hwndDlg);
  PopulateMidiDialogs(hwndDlg);
}

#elif defined OS_MAC
void IPlugAPPDialog::PopulatePreferencesDialog(HWND hwndDlg)
{
  SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_DRIVER,CB_ADDSTRING,0,(LPARAM)"CoreAudio");
  //SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_DRIVER,CB_ADDSTRING,0,(LPARAM)"Jack");
  SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_DRIVER,CB_SETCURSEL, mSettings.mAudioDriverType, 0);

  PopulateAudioDialogs(hwndDlg);
  PopulateMidiDialogs(hwndDlg);
}
#else
  #error NOT IMPLEMENTED
#endif

WDL_DLGRET IPlugAPPDialog::SettingsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  IPlugAPPDialog* pAppDialog = static_cast<IPlugAPPDialog *>(IPlugAPPHost::GetSettingsDialog());
  
  return pAppDialog->SettingsProcess(hwndDlg, uMsg, wParam, lParam);
}

WDL_DLGRET IPlugAPPDialog::SettingsProcess(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  auto getComboString = [&](WDL_String& str, int item, WPARAM idx) {
    std::string tempString;
    long len = (long) SendDlgItemMessage(hwndDlg, item, CB_GETLBTEXTLEN, idx, 0) + 1;
    tempString.reserve(len);
    SendDlgItemMessage(hwndDlg, item, CB_GETLBTEXT, idx, (LPARAM) tempString.data());
    str.Set(tempString.c_str());
  };
  
  int v = 0;
  switch(uMsg)
  {
    case WM_INITDIALOG:
      PopulatePreferencesDialog(hwndDlg);
      mPreviousSettings = mSettings;
      
      return TRUE;

    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDOK:
          mHost.TryToChangeAudio(false);
          EndDialog(hwndDlg, IDOK); // INI file will be changed see MainDialogProc
          break;
          
        case IDAPPLY:
          mHost.TryToChangeAudio(false);
          break;
          
        case IDCANCEL:
          EndDialog(hwndDlg, IDCANCEL);

          // if state has been changed reset to previous state, INI file won't be changed
          if (!(mSettings == mPreviousSettings))
          {
            mSettings = mPreviousSettings;

            mHost.TryToChangeAudioDriverType();
            mHost.ProbeAudioIO();
            mHost.TryToChangeAudio(false);
          }
          break;

        case IDC_COMBO_AUDIO_DRIVER:
          if (HIWORD(wParam) == CBN_SELCHANGE)
          {
            v = (int) SendDlgItemMessage(hwndDlg, IDC_COMBO_AUDIO_DRIVER, CB_GETCURSEL, 0, 0);

            if (v != mSettings.mAudioDriverType)
            {
              mSettings.mAudioDriverType = v;

              mHost.TryToChangeAudioDriverType();
              mHost.ProbeAudioIO();

              if (mAudioInputDevIDs.size())
                mSettings.mAudioInDev.Set(mHost.GetAudioDeviceName(mAudioInputDevIDs[0]).c_str());

              if (mAudioOutputDevIDs.size())
                mSettings.mAudioOutDev.Set(mHost.GetAudioDeviceName(mAudioOutputDevIDs[0]).c_str());

              // Reset IO
              mSettings.mAudioOutChanL = 1;
              mSettings.mAudioOutChanR = 2;

              PopulateAudioDialogs(hwndDlg);
            }
          }
          break;

        case IDC_COMBO_AUDIO_IN_DEV:
          if (HIWORD(wParam) == CBN_SELCHANGE)
          {
            int idx = (int) SendDlgItemMessage(hwndDlg, IDC_COMBO_AUDIO_IN_DEV, CB_GETCURSEL, 0, 0);
            getComboString(mSettings.mAudioInDev, IDC_COMBO_AUDIO_IN_DEV, idx);

            // Reset IO
            mSettings.mAudioInChanL = 1;
            mSettings.mAudioInChanR = 2;

            PopulateDriverSpecificControls(hwndDlg);
          }
          break;

        case IDC_COMBO_AUDIO_OUT_DEV:
          if (HIWORD(wParam) == CBN_SELCHANGE)
          {
            int idx = (int) SendDlgItemMessage(hwndDlg, IDC_COMBO_AUDIO_OUT_DEV, CB_GETCURSEL, 0, 0);
            getComboString(mSettings.mAudioOutDev, IDC_COMBO_AUDIO_OUT_DEV, idx);

            // Reset IO
            mSettings.mAudioOutChanL = 1;
            mSettings.mAudioOutChanR = 2;

            PopulateDriverSpecificControls(hwndDlg);
          }
          break;

        case IDC_COMBO_AUDIO_IN_L:
          if (HIWORD(wParam) == CBN_SELCHANGE)
          {
            mSettings.mAudioInChanL = (int) SendDlgItemMessage(hwndDlg, IDC_COMBO_AUDIO_IN_L, CB_GETCURSEL, 0, 0) + 1;

            //TEMP
            mSettings.mAudioInChanR = mSettings.mAudioInChanL + 1;
            SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_IN_R,CB_SETCURSEL, mSettings.mAudioInChanR - 1, 0);
            //
          }
          break;

        case IDC_COMBO_AUDIO_IN_R:
          if (HIWORD(wParam) == CBN_SELCHANGE)
            SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_IN_R,CB_SETCURSEL, mSettings.mAudioInChanR - 1, 0);  // TEMP
          mSettings.mAudioInChanR = (int) SendDlgItemMessage(hwndDlg, IDC_COMBO_AUDIO_IN_R, CB_GETCURSEL, 0, 0);
          break;

        case IDC_COMBO_AUDIO_OUT_L:
          if (HIWORD(wParam) == CBN_SELCHANGE)
          {
            mSettings.mAudioOutChanL = (int) SendDlgItemMessage(hwndDlg, IDC_COMBO_AUDIO_OUT_L, CB_GETCURSEL, 0, 0) + 1;

            //TEMP
            mSettings.mAudioOutChanR = mSettings.mAudioOutChanL + 1;
            SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_OUT_R,CB_SETCURSEL, mSettings.mAudioOutChanR - 1, 0);
            //
          }
          break;

        case IDC_COMBO_AUDIO_OUT_R:
          if (HIWORD(wParam) == CBN_SELCHANGE)
            SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_OUT_R,CB_SETCURSEL, mSettings.mAudioOutChanR - 1, 0);  // TEMP
          mSettings.mAudioOutChanR = (int) SendDlgItemMessage(hwndDlg, IDC_COMBO_AUDIO_OUT_R, CB_GETCURSEL, 0, 0);
          break;

//        case IDC_CB_MONO_INPUT:
//          if (SendDlgItemMessage(hwndDlg,IDC_CB_MONO_INPUT, BM_GETCHECK, 0, 0) == BST_CHECKED)
//            mSettings.mAudioInIsMono = 1;
//          else
//            mSettings.mAudioInIsMono = 0;
//          break;

        case IDC_COMBO_AUDIO_BUF_SIZE: // follow through
          if (HIWORD(wParam) == CBN_SELCHANGE)
          {
            int iovsidx = (int) SendDlgItemMessage(hwndDlg, IDC_COMBO_AUDIO_BUF_SIZE, CB_GETCURSEL, 0, 0);
            mSettings.mBufferSize = atoi(kBufferSizeOptions[iovsidx].c_str());
          }
          break;
        case IDC_COMBO_AUDIO_SR:
          if (HIWORD(wParam) == CBN_SELCHANGE)
          {
            int idx = (int) SendDlgItemMessage(hwndDlg, IDC_COMBO_AUDIO_SR, CB_GETCURSEL, 0, 0);
            mSettings.mAudioSR = (uint32_t) SendDlgItemMessage(hwndDlg, IDC_COMBO_AUDIO_SR, CB_GETITEMDATA, idx, 0);
          }
          break;

        case IDC_BUTTON_OS_DEV_SETTINGS:
          if (HIWORD(wParam) == BN_CLICKED)
          {
            #ifdef OS_WIN
            if ((mSettings.mAudioDriverType == kDeviceASIO) && mHost.IsAudioRunning()) // TODO: still not right
              ASIOControlPanel();
            #elif defined OS_MAC
            if (SWELL_GetOSXVersion() >= 0x1200)
            {
              system("open \"/System/Applications/Utilities/Audio MIDI Setup.app\"");
            }
            else
            {
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
            int idx = (int) SendDlgItemMessage(hwndDlg, IDC_COMBO_MIDI_IN_DEV, CB_GETCURSEL, 0, 0);
            getComboString(mSettings.mMidiInDev, IDC_COMBO_MIDI_IN_DEV, idx);
            mHost.SelectMIDIDevice(ERoute::kInput, mSettings.mMidiInDev.Get());
          }
          break;

        case IDC_COMBO_MIDI_OUT_DEV:
          if (HIWORD(wParam) == CBN_SELCHANGE)
          {
            int idx = (int) SendDlgItemMessage(hwndDlg, IDC_COMBO_MIDI_OUT_DEV, CB_GETCURSEL, 0, 0);
            getComboString(mSettings.mMidiOutDev, IDC_COMBO_MIDI_OUT_DEV, idx);
            mHost.SelectMIDIDevice(ERoute::kOutput, mSettings.mMidiOutDev.Get());
          }
          break;

        case IDC_COMBO_MIDI_IN_CHAN:
          if (HIWORD(wParam) == CBN_SELCHANGE)
            mSettings.mMidiInChan = (int) SendDlgItemMessage(hwndDlg, IDC_COMBO_MIDI_IN_CHAN, CB_GETCURSEL, 0, 0);
          break;

        case IDC_COMBO_MIDI_OUT_CHAN:
          if (HIWORD(wParam) == CBN_SELCHANGE)
            mSettings.mMidiOutChan = (int) SendDlgItemMessage(hwndDlg, IDC_COMBO_MIDI_OUT_CHAN, CB_GETCURSEL, 0, 0);
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
