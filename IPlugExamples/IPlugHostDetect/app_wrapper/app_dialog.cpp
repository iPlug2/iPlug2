#include "app_main.h"

#ifdef OS_WIN
#include "asio.h"
#endif

const int kNumIOVSOptions = 9;
const int kNumSIGVSOptions = 7;

const std::string kIOVSOptions[kNumIOVSOptions] = {"32", "64", "128", "256", "512", "1024", "2048", "4096", "8192" };
const std::string kSIGVSOptions[kNumSIGVSOptions] = {"16", "32", "64", "128", "256", "512", "1024" };

// check the input and output devices, find matching srs
void PopulateSampleRateList(HWND hwndDlg, RtAudio::DeviceInfo* inputDevInfo, RtAudio::DeviceInfo* outputDevInfo)
{
  char buf[20];

  SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_SR,CB_RESETCONTENT,0,0);

  std::vector<int> matchedSRs;

  for (int i=0; i<inputDevInfo->sampleRates.size(); i++)
  {
    for (int j=0; j<outputDevInfo->sampleRates.size(); j++)
    {
      if(inputDevInfo->sampleRates[i] == outputDevInfo->sampleRates[j])
        matchedSRs.push_back(inputDevInfo->sampleRates[i]);
    }
  }

  for (int k=0; k<matchedSRs.size(); k++)
  {
    wsprintf(buf,"%i",matchedSRs[k]);
    SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_SR,CB_ADDSTRING,0,(LPARAM)buf);
  }

  LRESULT sridx = SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_SR, CB_FINDSTRINGEXACT, -1, (LPARAM)gState->mAudioSR);
  SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_SR,CB_SETCURSEL, sridx, 0);
}

void PopulateAudioInputList(HWND hwndDlg, RtAudio::DeviceInfo* info)
{
  char buf[20];

  SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_IN_L,CB_RESETCONTENT,0,0);
  SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_IN_R,CB_RESETCONTENT,0,0);

  int i;

  for (i=0; i<info->inputChannels -1; i++)
  {
//  for (int i=0; i<info.inputChannels; i++) {
    wsprintf(buf,"%i",i+1);
    SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_IN_L,CB_ADDSTRING,0,(LPARAM)buf);
    SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_IN_R,CB_ADDSTRING,0,(LPARAM)buf);
  }

  // TEMP
  wsprintf(buf,"%i",i+1);
  SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_IN_R,CB_ADDSTRING,0,(LPARAM)buf);

  SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_IN_L,CB_SETCURSEL, gState->mAudioInChanL - 1, 0);
  SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_IN_R,CB_SETCURSEL, gState->mAudioInChanR - 1, 0);
}

void PopulateAudioOutputList(HWND hwndDlg, RtAudio::DeviceInfo* info)
{
  char buf[20];

  SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_OUT_L,CB_RESETCONTENT,0,0);
  SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_OUT_R,CB_RESETCONTENT,0,0);

  int i;

//  for (int i=0; i<info.outputChannels; i++) {
  for (i=0; i<info->outputChannels -1; i++)
  {

    wsprintf(buf,"%i",i+1);
    SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_OUT_L,CB_ADDSTRING,0,(LPARAM)buf);
    SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_OUT_R,CB_ADDSTRING,0,(LPARAM)buf);
  }

  // TEMP
  wsprintf(buf,"%i",i+1);
  SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_OUT_R,CB_ADDSTRING,0,(LPARAM)buf);

  SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_OUT_L,CB_SETCURSEL, gState->mAudioOutChanL - 1, 0);
  SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_OUT_R,CB_SETCURSEL, gState->mAudioOutChanR - 1, 0);
}

// This has to get called after any change to audio driver/in dev/out dev
void PopulateDriverSpecificControls(HWND hwndDlg)
{
#ifdef OS_WIN
  int driverType = (int) SendDlgItemMessage(hwndDlg, IDC_COMBO_AUDIO_DRIVER, CB_GETCURSEL, 0, 0);
  if(driverType)   //ASIO
  {
    ComboBox_Enable(GetDlgItem(hwndDlg, IDC_COMBO_AUDIO_IN_DEV), FALSE);
    Button_Enable(GetDlgItem(hwndDlg, IDC_BUTTON_ASIO), TRUE);
  }
  else
  {
    ComboBox_Enable(GetDlgItem(hwndDlg, IDC_COMBO_AUDIO_IN_DEV), TRUE);
    Button_Enable(GetDlgItem(hwndDlg, IDC_BUTTON_ASIO), FALSE);
  }
#endif

  int indevidx = 0;
  int outdevidx = 0;

  SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_IN_DEV,CB_RESETCONTENT,0,0);
  SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_OUT_DEV,CB_RESETCONTENT,0,0);

  for (int i = 0; i<gAudioInputDevs.size(); i++)
  {
    SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_IN_DEV,CB_ADDSTRING,0,(LPARAM)GetAudioDeviceName(gAudioInputDevs[i]).c_str());

    if(!strcmp(GetAudioDeviceName(gAudioInputDevs[i]).c_str(), gState->mAudioInDev))
      indevidx = i;
  }

  for (int i = 0; i<gAudioOutputDevs.size(); i++)
  {
    SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_OUT_DEV,CB_ADDSTRING,0,(LPARAM)GetAudioDeviceName(gAudioOutputDevs[i]).c_str());

    if(!strcmp(GetAudioDeviceName(gAudioOutputDevs[i]).c_str(), gState->mAudioOutDev))
      outdevidx = i;
  }

#ifdef OS_WIN
  if(driverType)
    SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_IN_DEV,CB_SETCURSEL, outdevidx, 0);
  else
#endif
    SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_IN_DEV,CB_SETCURSEL, indevidx, 0);

  SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_OUT_DEV,CB_SETCURSEL, outdevidx, 0);

  RtAudio::DeviceInfo inputDevInfo = gDAC->getDeviceInfo(gAudioInputDevs[indevidx]);
  RtAudio::DeviceInfo outputDevInfo = gDAC->getDeviceInfo(gAudioOutputDevs[outdevidx]);

  PopulateAudioInputList(hwndDlg, &inputDevInfo);
  PopulateAudioOutputList(hwndDlg, &outputDevInfo);
  PopulateSampleRateList(hwndDlg, &inputDevInfo, &outputDevInfo);
}

void PopulateAudioDialogs(HWND hwndDlg)
{
  PopulateDriverSpecificControls(hwndDlg);

  if (gState->mAudioInIsMono)
  {
    SendDlgItemMessage(hwndDlg,IDC_CB_MONO_INPUT,BM_SETCHECK, BST_CHECKED,0);
  }
  else
  {
    SendDlgItemMessage(hwndDlg,IDC_CB_MONO_INPUT,BM_SETCHECK, BST_UNCHECKED,0);
  }

  //Populate IOVS combobox
  for (int i = 0; i< kNumIOVSOptions; i++)
  {
    SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_IOVS,CB_ADDSTRING,0,(LPARAM)kIOVSOptions[i].c_str());
  }

  LRESULT iovsidx = SendDlgItemMessage(hwndDlg, IDC_COMBO_AUDIO_IOVS, CB_FINDSTRINGEXACT, -1, (LPARAM)gState->mAudioIOVS);
  SendDlgItemMessage(hwndDlg, IDC_COMBO_AUDIO_IOVS, CB_SETCURSEL, iovsidx, 0);

  //Populate SIGVS combobox
  for (int i = 0; i< kNumSIGVSOptions; i++)
  {
    SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_SIGVS,CB_ADDSTRING,0,(LPARAM)kSIGVSOptions[i].c_str());
  }

  LRESULT sigvsidx = SendDlgItemMessage(hwndDlg, IDC_COMBO_AUDIO_SIGVS, CB_FINDSTRINGEXACT, -1, (LPARAM)gState->mAudioSigVS);
  SendDlgItemMessage(hwndDlg, IDC_COMBO_AUDIO_SIGVS, CB_SETCURSEL, sigvsidx, 0);
}

bool PopulateMidiDialogs(HWND hwndDlg)
{
  if ( !gMidiIn || !gMidiOut )
    return false;
  else
  {
    for (int i=0; i<gMIDIInputDevNames.size(); i++ )
    {
      SendDlgItemMessage(hwndDlg,IDC_COMBO_MIDI_IN_DEV,CB_ADDSTRING,0,(LPARAM)gMIDIInputDevNames[i].c_str());
    }

    LRESULT indevidx = SendDlgItemMessage(hwndDlg,IDC_COMBO_MIDI_IN_DEV,CB_FINDSTRINGEXACT, -1, (LPARAM)gState->mMidiInDev);

    // if the midi port name wasn't found update the ini file, and set to off
    if(indevidx == -1)
    {
      strcpy(gState->mMidiInDev, "off");
      UpdateINI();
      indevidx = 0;
    }

    SendDlgItemMessage(hwndDlg,IDC_COMBO_MIDI_IN_DEV,CB_SETCURSEL, indevidx, 0);

    for (int i=0; i<gMIDIOutputDevNames.size(); i++ )
    {
      SendDlgItemMessage(hwndDlg,IDC_COMBO_MIDI_OUT_DEV,CB_ADDSTRING,0,(LPARAM)gMIDIOutputDevNames[i].c_str());
    }

    LRESULT outdevidx = SendDlgItemMessage(hwndDlg,IDC_COMBO_MIDI_OUT_DEV,CB_FINDSTRINGEXACT, -1, (LPARAM)gState->mMidiOutDev);

    // if the midi port name wasn't found update the ini file, and set to off
    if(outdevidx == -1)
    {
      strcpy(gState->mMidiOutDev, "off");
      UpdateINI();
      outdevidx = 0;
    }

    SendDlgItemMessage(hwndDlg,IDC_COMBO_MIDI_OUT_DEV,CB_SETCURSEL, outdevidx, 0);

    // Populate MIDI channel dialogs

    SendDlgItemMessage(hwndDlg,IDC_COMBO_MIDI_IN_CHAN,CB_ADDSTRING,0,(LPARAM)"all");
    SendDlgItemMessage(hwndDlg,IDC_COMBO_MIDI_OUT_CHAN,CB_ADDSTRING,0,(LPARAM)"all");

    char buf[20];

    for (int i=0; i<16; i++)
    {
      wsprintf(buf,"%i",i+1);
      SendDlgItemMessage(hwndDlg,IDC_COMBO_MIDI_IN_CHAN,CB_ADDSTRING,0,(LPARAM)buf);
      SendDlgItemMessage(hwndDlg,IDC_COMBO_MIDI_OUT_CHAN,CB_ADDSTRING,0,(LPARAM)buf);
    }

    SendDlgItemMessage(hwndDlg,IDC_COMBO_MIDI_IN_CHAN,CB_SETCURSEL, (LPARAM)gState->mMidiInChan, 0);
    SendDlgItemMessage(hwndDlg,IDC_COMBO_MIDI_OUT_CHAN,CB_SETCURSEL, (LPARAM)gState->mMidiOutChan, 0);

    return true;
  }
}

#ifdef OS_WIN
void PopulatePreferencesDialog(HWND hwndDlg)
{
  SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_DRIVER,CB_ADDSTRING,0,(LPARAM)"DirectSound");
  SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_DRIVER,CB_ADDSTRING,0,(LPARAM)"ASIO");
  SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_DRIVER,CB_SETCURSEL, gState->mAudioDriverType, 0);

  PopulateAudioDialogs(hwndDlg);
  PopulateMidiDialogs(hwndDlg);
}

#else if defined OS_OSX
void PopulatePreferencesDialog(HWND hwndDlg)
{
  SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_DRIVER,CB_ADDSTRING,0,(LPARAM)"CoreAudio");
  //SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_DRIVER,CB_ADDSTRING,0,(LPARAM)"Jack");
  SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_DRIVER,CB_SETCURSEL, gState->mAudioDriverType, 0);

  PopulateAudioDialogs(hwndDlg);
  PopulateMidiDialogs(hwndDlg);
}
#endif

WDL_DLGRET PreferencesDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  int v = 0;
  switch(uMsg)
  {
    case WM_INITDIALOG:

      PopulatePreferencesDialog(hwndDlg);
      memcpy(gTempState, gState, sizeof(AppState)); // copy state to temp state

      return TRUE;

    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDOK:
          if(memcmp(gActiveState, gState, sizeof(AppState)) != 0) // if state is different try to change audio
          {
            TryToChangeAudio();
          }
          EndDialog(hwndDlg, IDOK); // INI file will be changed see MainDialogProc
          break;
        case IDAPPLY:
          TryToChangeAudio();
          break;
        case IDCANCEL:
          EndDialog(hwndDlg, IDCANCEL);

          // if state has been changed reset to previous state, INI file won't be changed
          if (!AudioSettingsInStateAreEqual(gState, gTempState)
              || !MIDISettingsInStateAreEqual(gState, gTempState))
          {
            memcpy(gState, gTempState, sizeof(AppState));

            TryToChangeAudioDriverType();
            ProbeAudioIO();
            TryToChangeAudio();
          }

          break;

        case IDC_COMBO_AUDIO_DRIVER:
          if (HIWORD(wParam) == CBN_SELCHANGE)
          {

            v = (int) SendDlgItemMessage(hwndDlg, IDC_COMBO_AUDIO_DRIVER, CB_GETCURSEL, 0, 0);

            if(v != gState->mAudioDriverType)
            {
              gState->mAudioDriverType = v;

              TryToChangeAudioDriverType();
              ProbeAudioIO();

              strcpy(gState->mAudioInDev,GetAudioDeviceName(gAudioInputDevs[0]).c_str());
              strcpy(gState->mAudioOutDev,GetAudioDeviceName(gAudioOutputDevs[0]).c_str());

              // Reset IO
              gState->mAudioOutChanL = 1;
              gState->mAudioOutChanR = 2;

              PopulateAudioDialogs(hwndDlg);
            }
          }
          break;

        case IDC_COMBO_AUDIO_IN_DEV:
          if (HIWORD(wParam) == CBN_SELCHANGE)
          {
            int idx = (int) SendDlgItemMessage(hwndDlg, IDC_COMBO_AUDIO_IN_DEV, CB_GETCURSEL, 0, 0);
            SendDlgItemMessage(hwndDlg, IDC_COMBO_AUDIO_IN_DEV, CB_GETLBTEXT, idx, (LPARAM) gState->mAudioInDev);

            // Reset IO
            gState->mAudioInChanL = 1;
            gState->mAudioInChanR = 2;

            PopulateDriverSpecificControls(hwndDlg);
          }
          break;

        case IDC_COMBO_AUDIO_OUT_DEV:
          if (HIWORD(wParam) == CBN_SELCHANGE)
          {
            int idx = (int) SendDlgItemMessage(hwndDlg, IDC_COMBO_AUDIO_OUT_DEV, CB_GETCURSEL, 0, 0);
            SendDlgItemMessage(hwndDlg, IDC_COMBO_AUDIO_OUT_DEV, CB_GETLBTEXT, idx, (LPARAM) gState->mAudioOutDev);

            // Reset IO
            gState->mAudioOutChanL = 1;
            gState->mAudioOutChanR = 2;

            PopulateDriverSpecificControls(hwndDlg);
          }
          break;

        case IDC_COMBO_AUDIO_IN_L:
          if (HIWORD(wParam) == CBN_SELCHANGE)
          {
            gState->mAudioInChanL = (int) SendDlgItemMessage(hwndDlg, IDC_COMBO_AUDIO_IN_L, CB_GETCURSEL, 0, 0) + 1;

            //TEMP
            gState->mAudioInChanR = gState->mAudioInChanL + 1;
            SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_IN_R,CB_SETCURSEL, gState->mAudioInChanR - 1, 0);
            //
          }
          break;

        case IDC_COMBO_AUDIO_IN_R:
          if (HIWORD(wParam) == CBN_SELCHANGE)
            SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_IN_R,CB_SETCURSEL, gState->mAudioInChanR - 1, 0);  // TEMP
//                gState->mAudioInChanR = (int) SendDlgItemMessage(hwndDlg, IDC_COMBO_AUDIO_IN_R, CB_GETCURSEL, 0, 0);
          break;

        case IDC_COMBO_AUDIO_OUT_L:
          if (HIWORD(wParam) == CBN_SELCHANGE)
          {
            gState->mAudioOutChanL = (int) SendDlgItemMessage(hwndDlg, IDC_COMBO_AUDIO_OUT_L, CB_GETCURSEL, 0, 0) + 1;

            //TEMP
            gState->mAudioOutChanR = gState->mAudioOutChanL + 1;
            SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_OUT_R,CB_SETCURSEL, gState->mAudioOutChanR - 1, 0);
            //
          }
          break;

        case IDC_COMBO_AUDIO_OUT_R:
          if (HIWORD(wParam) == CBN_SELCHANGE)
            SendDlgItemMessage(hwndDlg,IDC_COMBO_AUDIO_OUT_R,CB_SETCURSEL, gState->mAudioOutChanR - 1, 0);  // TEMP
//                gState->mAudioOutChanR = (int) SendDlgItemMessage(hwndDlg, IDC_COMBO_AUDIO_OUT_R, CB_GETCURSEL, 0, 0);
          break;

        case IDC_CB_MONO_INPUT:
          if (SendDlgItemMessage(hwndDlg,IDC_CB_MONO_INPUT, BM_GETCHECK, 0, 0) == BST_CHECKED)
            gState->mAudioInIsMono = 1;
          else
            gState->mAudioInIsMono = 0;
          break;

        case IDC_COMBO_AUDIO_IOVS: // follow through
        case IDC_COMBO_AUDIO_SIGVS:
          //TODO: FIX
          if (HIWORD(wParam) == CBN_SELCHANGE)
          {
            int iovsidx = (int) SendDlgItemMessage(hwndDlg, IDC_COMBO_AUDIO_IOVS, CB_GETCURSEL, 0, 0);
            int sigvsidx = (int) SendDlgItemMessage(hwndDlg, IDC_COMBO_AUDIO_SIGVS, CB_GETCURSEL, 0, 0);

            if (atoi(kIOVSOptions[iovsidx].c_str()) < atoi(kSIGVSOptions[sigvsidx].c_str()))   // if iovs < sigvs
            {
              SendDlgItemMessage(hwndDlg, IDC_COMBO_AUDIO_IOVS, CB_GETLBTEXT, iovsidx, (LPARAM) gState->mAudioIOVS);
              strcpy(gState->mAudioSigVS, kSIGVSOptions[0].c_str()); // set sigvs to minimum
              SendDlgItemMessage(hwndDlg, IDC_COMBO_AUDIO_SIGVS, CB_SETCURSEL, -1, 0);
            }
            else
            {
              SendDlgItemMessage(hwndDlg, IDC_COMBO_AUDIO_IOVS, CB_GETLBTEXT, iovsidx, (LPARAM) gState->mAudioIOVS);
              SendDlgItemMessage(hwndDlg, IDC_COMBO_AUDIO_SIGVS, CB_GETLBTEXT, sigvsidx, (LPARAM) gState->mAudioSigVS);
            }
          }
          break;
        case IDC_COMBO_AUDIO_SR:
          if (HIWORD(wParam) == CBN_SELCHANGE)
          {
            int idx = (int) SendDlgItemMessage(hwndDlg, IDC_COMBO_AUDIO_SR, CB_GETCURSEL, 0, 0);
            SendDlgItemMessage(hwndDlg, IDC_COMBO_AUDIO_SR, CB_GETLBTEXT, idx, (LPARAM) gState->mAudioSR);
          }
          break;

        case IDC_BUTTON_ASIO:
          if (HIWORD(wParam) == BN_CLICKED)
            #ifdef OS_OSX
            system("open \"/Applications/Utilities/Audio MIDI Setup.app\"");
            #elif defined OS_WIN
            if( gState->mAudioDriverType == DAC_ASIO && gDAC->isStreamRunning()) // TODO: still not right
              ASIOControlPanel();
            #endif
          break;

        case IDC_COMBO_MIDI_IN_DEV:
          if (HIWORD(wParam) == CBN_SELCHANGE)
          {
            int idx = (int) SendDlgItemMessage(hwndDlg, IDC_COMBO_MIDI_IN_DEV, CB_GETCURSEL, 0, 0);
            SendDlgItemMessage(hwndDlg, IDC_COMBO_MIDI_IN_DEV, CB_GETLBTEXT, idx, (LPARAM) gState->mMidiInDev);
            ChooseMidiInput( gState->mMidiInDev );
          }
          break;

        case IDC_COMBO_MIDI_OUT_DEV:
          if (HIWORD(wParam) == CBN_SELCHANGE)
          {
            int idx = (int) SendDlgItemMessage(hwndDlg, IDC_COMBO_MIDI_OUT_DEV, CB_GETCURSEL, 0, 0);
            SendDlgItemMessage(hwndDlg, IDC_COMBO_MIDI_OUT_DEV, CB_GETLBTEXT, idx, (LPARAM) gState->mMidiOutDev);
            ChooseMidiOutput( gState->mMidiOutDev );
          }
          break;

        case IDC_COMBO_MIDI_IN_CHAN:
          if (HIWORD(wParam) == CBN_SELCHANGE)
            gState->mMidiInChan = (int) SendDlgItemMessage(hwndDlg, IDC_COMBO_MIDI_IN_CHAN, CB_GETCURSEL, 0, 0);
          break;

        case IDC_COMBO_MIDI_OUT_CHAN:
          if (HIWORD(wParam) == CBN_SELCHANGE)
            gState->mMidiOutChan = (int) SendDlgItemMessage(hwndDlg, IDC_COMBO_MIDI_OUT_CHAN, CB_GETCURSEL, 0, 0);
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

#ifdef _WIN32
void ClientResize(HWND hWnd, int nWidth, int nHeight)
{
  RECT rcClient, rcWindow;
  POINT ptDiff;
  int screenwidth, screenheight;
  int x, y;

  screenwidth  = GetSystemMetrics(SM_CXSCREEN);
  screenheight = GetSystemMetrics(SM_CYSCREEN);
  x = (screenwidth / 2) - (nWidth/2);
  y = (screenheight / 2) - (nHeight/2);

  GetClientRect(hWnd, &rcClient);
  GetWindowRect(hWnd, &rcWindow);
  ptDiff.x = (rcWindow.right - rcWindow.left) - rcClient.right;
  ptDiff.y = (rcWindow.bottom - rcWindow.top) - rcClient.bottom;
  MoveWindow(hWnd, x, y, nWidth + ptDiff.x, nHeight + ptDiff.y, FALSE);
}
#endif

WDL_DLGRET MainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_INITDIALOG:

      gHWND=hwndDlg;

      if(!AttachGUI()) printf("couldn't attach gui\n");

#ifdef _WIN32
      ClientResize(hwndDlg, GUI_WIDTH, GUI_HEIGHT);
      //SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)LoadIcon(NULL, MAKEINTRESOURCE(IDI_ICON1)));
      //SendMessage(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)LoadIcon(NULL, MAKEINTRESOURCE(IDI_ICON1)));

#else // OSX
      CenterWindow(hwndDlg);
#endif

      ShowWindow(hwndDlg,SW_SHOW);
      return 1;
    case WM_DESTROY:
      gHWND=NULL;

#ifdef _WIN32
      PostQuitMessage(0);
#else
      SWELL_PostQuitMessage(hwndDlg);
#endif

      return 0;
    case WM_CLOSE:
      DestroyWindow(hwndDlg);
      return 0;
//    case WM_GETDLGCODE: {
//        LPARAM lres;
//        lres = CallWindowProc(/*TODO GET PROC */, hWnd, WM_GETDLGCODE, wParam, lParam);
//        if (lParam && ((MSG*)lParam)->message == WM_KEYDOWN  &&  wParam == VK_LEFT) {
//          lres |= DLGC_WANTMESSAGE;
//        }
//        return lres;
//      }
    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case ID_QUIT:
          DestroyWindow(hwndDlg);
          return 0;
        case ID_ABOUT:
          if(!gPluginInstance->HostRequestingAboutBox())
          {
            char version[50];
            sprintf(version, BUNDLE_MFR"\nBuilt on "__DATE__);
            MessageBox(hwndDlg,version, BUNDLE_NAME, MB_OK);
          }
          return 0;
        case ID_PREFERENCES:
        {
          INT_PTR ret = DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG_PREF), hwndDlg, PreferencesDlgProc);

          if(ret == IDOK)
          {
            UpdateINI();
          }

          return 0;
        }
      }
      return 0;
  }
  return 0;
}
