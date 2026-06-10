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
#include "wdlstring.h"

struct reaper_plugin_info_t;

BEGIN_IPLUG_NAMESPACE

#pragma pack(push, 4)
/** State structure for dock window persistence - matches SWS pattern */
struct ReaperExtDockState
{
  RECT r;          // Window rect when floating
  int state;       // Bit 0 = visible, Bit 1 = docked
  int whichdock;   // Docker index when docked
};
#pragma pack(pop)

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

  /** Called during idle processing - override to perform periodic tasks */
  virtual void OnIdle() {}; // NO-OP

  /** Registers an action with the REAPER extension system
   * @param actionName The name of the action to register
   * @param func The function to call when the action is executed
   * @param addMenuItem If true, adds a menu item for this action
   * @param pToggle Optional pointer to an int for toggle state */
  void RegisterAction(const char* actionName, std::function<void()> func, bool addMenuItem = false, int* pToggle = nullptr/*, IKeyPress keyCmd*/);

  /** Toggles the visibility of the main extension window */
  void ShowHideMainWindow();

  /** Toggles between docked and floating state */
  void ToggleDocking();

  /** Returns true if the window is currently docked */
  bool IsDocked() const { return (mDockState.state & 2) == 2; }

  /** Sets the unique identifier used for dock state persistence
   * @param id Unique identifier string (defaults to PLUG_CLASS_NAME) */
  void SetDockId(const char* id) { mDockId.Set(id); }

public:
  // Reaper calls back to this when it wants to execute an action registered by the extension plugin
  static bool HookCommandProc(int command, int flag);
  
  // Reaper calls back to this when it wants to know an actions toggle state
  static int ToggleActionCallback(int command);
  
private:
  static WDL_DLGRET MainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
  
  void OnTimer(Timer& t);
  void CreateMainWindow();
  void DestroyMainWindow();
  void SaveDockState();
  void LoadDockState();

  reaper_plugin_info_t* mRec = nullptr;
  std::unique_ptr<Timer> mTimer;
  ReaperExtDockState mDockState = {};
  WDL_FastString mDockId;
  bool mSaveStateOnDestroy = true;
  bool mStateLoaded = false;
};

END_IPLUG_NAMESPACE
