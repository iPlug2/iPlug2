/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

#include "IPlugAPP_host.h"

BEGIN_IPLUG_NAMESPACE

class IPlugAPPDialog final : public IPlugAPPHost::IPlugAPPSettingsDialog
{
public:
    
  IPlugAPPDialog(IPlugAPPHost& host)
  : IPlugAPPHost::IPlugAPPSettingsDialog(host)
  {}
  
  static WDL_DLGRET SettingsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
  
  DLGPROC GetDlgProc() override { return SettingsDlgProc; }
  
private:
  
  void PopulateSampleRateList(HWND hwndDlg, RtAudio::DeviceInfo* pInputDevInfo, RtAudio::DeviceInfo* pOutputDevInfo);
  void PopulateAudioInputList(HWND hwndDlg, RtAudio::DeviceInfo* pInfo);
  void PopulateAudioOutputList(HWND hwndDlg, RtAudio::DeviceInfo* pInfo);
  void PopulateDriverSpecificControls(HWND hwndDlg);
  void PopulateAudioDialogs(HWND hwndDlg);
  bool PopulateMidiDialogs(HWND hwndDlg);
  void PopulatePreferencesDialog(HWND hwndDlg);
  
  WDL_DLGRET SettingsProcess(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

  /** When the preferences dialog is opened the existing state is cached here, and restored if cancel is pressed */
  IPlugAPPHost::AppSettings mPreviousSettings;
};

END_IPLUG_NAMESPACE
