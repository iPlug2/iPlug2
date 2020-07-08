/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

/**
 * @file ReaperExtBase.h
 * @brief Reaper extension base class interface
 * Include this file in the main header for your reaper extension
*/

#include "IPlugTimer.h"
#include "IPlugDelegate_select.h"

struct reaper_plugin_info_t;

BEGIN_IPLUG_NAMESPACE

/** Reaper extension base class interface */
class ReaperExtBase : public EDITOR_DELEGATE_CLASS
{
public:
  ReaperExtBase(reaper_plugin_info_t* pRec);
  
  virtual ~ReaperExtBase();

  //IEditorDelegate
  void BeginInformHostOfParamChangeFromUI(int paramIdx) override {}; // NO-OP
  
  void EndInformHostOfParamChangeFromUI(int paramIdx) override {}; // NO-OP
  
  bool EditorResizeFromUI(int viewWidth, int viewHeight, bool needsPlatformResize) override;

  /** /todo */
  virtual void OnIdle() {}; // NO-OP
  
  /** /todo
   * @param actionName /todo
   * @param func /todo */
  void RegisterAction(const char* actionName, std::function<void()> func, bool addMenuItem = false, int* pToggle = nullptr/*, IKeyPress keyCmd*/);
  
  /** /todo */
  void ShowHideMainWindow();
  
  void ToggleDocking();

public:
  // Reaper calls back to this when it wants to execute an action registered by the extension plugin
  static bool HookCommandProc(int command, int flag);
  
  // Reaper calls back to this when it wants to know an actions toggle state
  static int ToggleActionCallback(int command);
  
private:
  static WDL_DLGRET MainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
  
  void OnTimer(Timer& t);

  reaper_plugin_info_t* mRec = nullptr;
  std::unique_ptr<Timer> mTimer;
  bool mDocked = false;
};

END_IPLUG_NAMESPACE
