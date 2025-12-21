/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

#include "IPlugAPP_host.h"

BEGIN_IPLUG_NAMESPACE

class WebViewSettingsDialog final : public IPlugAPPHost::ISettingsDialog
{
public:
  WebViewSettingsDialog(IPlugAPPHost& host)
  : IPlugAPPHost::ISettingsDialog(host)
  {}
  
  static WDL_DLGRET SettingsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
  
  DLGPROC GetDlgProc() override { return SettingsDlgProc; }
  
  void Refresh() override;
  
private:
  WDL_DLGRET SettingsProcess(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

  /** When the preferences dialog is opened the existing state is cached here, and restored if cancel is pressed */
  IPlugAPPHost::AppSettings mPreviousSettings;
};

END_IPLUG_NAMESPACE
