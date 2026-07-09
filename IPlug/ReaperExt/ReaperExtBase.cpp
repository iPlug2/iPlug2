// Helper to stringify macro
#define IPLUG_STRINGIFY_HELPER(x) #x
#define IPLUG_STRINGIFY(x) IPLUG_STRINGIFY_HELPER(x)

ReaperExtBase::ReaperExtBase(reaper_plugin_info_t* pRec)
: EDITOR_DELEGATE_CLASS(0) // zero params
, mRec(pRec)
{
  mTimer = std::unique_ptr<Timer>(Timer::Create(std::bind(&ReaperExtBase::OnTimer, this, std::placeholders::_1), IDLE_TIMER_RATE));
  mDockId.Set(IPLUG_STRINGIFY(PLUG_CLASS_NAME));
  mMenuName.Set(IPLUG_STRINGIFY(PLUG_CLASS_NAME));
  memset(&mDockState, 0, sizeof(ReaperExtDockState));
  // Note: LoadDockState() is called lazily via EnsureStateLoaded() after API imports
}

ReaperExtBase::~ReaperExtBase()
{
  mTimer->Stop();
  if (gHWND)
  {
    mSaveStateOnDestroy = false;
    DestroyWindow(gHWND);
  }
}

void ReaperExtBase::OnTimer(Timer& t)
{
  OnIdle();
}

auto ClientResize = [](HWND hWnd, int nWidth, int nHeight) {
  RECT rcClient, rcWindow;
  POINT ptDiff;
  int screenwidth, screenheight;
  int x, y;
  
  screenwidth = GetSystemMetrics(SM_CXSCREEN);
  screenheight = GetSystemMetrics(SM_CYSCREEN);
  x = (screenwidth / 2) - (nWidth / 2);
  y = (screenheight / 2) - (nHeight / 2);
  
  GetClientRect(hWnd, &rcClient);
  GetWindowRect(hWnd, &rcWindow);
  ptDiff.x = (rcWindow.right - rcWindow.left) - rcClient.right;
  ptDiff.y = (rcWindow.bottom - rcWindow.top) - rcClient.bottom;
  
  SetWindowPos(hWnd, 0, x, y, nWidth + ptDiff.x, nHeight + ptDiff.y, 0);
};

bool ReaperExtBase::EditorResizeFromUI(int viewWidth, int viewHeight, bool needsPlatformResize)
{
  if (viewWidth != GetEditorWidth() || viewHeight != GetEditorHeight())
  {
    // Don't resize the window when docked — REAPER controls the dock size
    if (!IsDocked() && needsPlatformResize)
    {
#ifdef OS_MAC
#define TITLEBAR_BODGE 22 //TODO: sort this out
      RECT r;
      GetWindowRect(gHWND, &r);
      SetWindowPos(gHWND, 0, r.left, r.bottom - viewHeight - TITLEBAR_BODGE, viewWidth, viewHeight + TITLEBAR_BODGE, 0);
#endif
    }

    return true;
  }

  return false;
}

/** Reads the persisted dock state on first use. Deferred out of the constructor because
 * the REAPER API function pointers aren't imported yet at that point */
void ReaperExtBase::EnsureStateLoaded()
{
  if (mStateLoaded)
    return;

  LoadDockState();
  mStateLoaded = true;
  UpdateToggleStates();
}

void ReaperExtBase::CreateMainWindow()
{
  if (gHWND != NULL)
    return;

  EnsureStateLoaded();

  gHWND = CreateDialog(gHINSTANCE, MAKEINTRESOURCE(IDD_DIALOG_MAIN), gParent, ReaperExtBase::MainDlgProc);

  UpdateToggleStates();
}

/** Keeps the toggle ints in sync with the real state, so any action registered with
 * GetWindowTogglePtr()/GetDockTogglePtr() reports the truth to REAPER. Dockedness is a
 * persisted preference that applies whether or not the window is currently open */
void ReaperExtBase::UpdateToggleStates()
{
  mWindowToggle = (gHWND != NULL) ? 1 : 0;
  mDockToggle = IsDocked() ? 1 : 0;
}

void ReaperExtBase::DestroyMainWindow()
{
  if (gHWND == NULL)
    return;

  SaveDockState();
  gPlug->CloseWindow();
  DockWindowRemove(gHWND);
  DestroyWindow(gHWND);
  gHWND = NULL;

  UpdateToggleStates();
}

void ReaperExtBase::ShowHideMainWindow()
{
  if (gHWND == NULL)
  {
    CreateMainWindow();
    if (IsDocked())
      DockWindowActivate(gHWND);
  }
  else
  {
    DestroyMainWindow();
  }
}

void ReaperExtBase::ToggleDocking()
{
  EnsureStateLoaded();

  // With no window open, just flip the persisted preference so the next open honours it
  if (gHWND == NULL)
  {
    mDockState.state ^= 2;
    SaveDockState();
    UpdateToggleStates();
    return;
  }

  // Save floating position before toggling
  if (!IsDocked())
    GetWindowRect(gHWND, &mDockState.r);

  // Destroy and recreate - this is the SWS pattern for reliable dock toggling
  mSaveStateOnDestroy = false;
  gPlug->CloseWindow();
  DockWindowRemove(gHWND);
  DestroyWindow(gHWND);
  gHWND = NULL;

  // Toggle docked bit
  mDockState.state ^= 2;
  mSaveStateOnDestroy = true;

  // Recreate window with new state
  CreateMainWindow();
  if (IsDocked())
    DockWindowActivate(gHWND);
}

void ReaperExtBase::SaveDockState()
{
  const char* iniFile = get_ini_file();
  if (!iniFile)
    return;

  if (gHWND != NULL)
  {
    int dockIdx = DockIsChildOfDock(gHWND, NULL);
    if (dockIdx >= 0)
      mDockState.whichdock = dockIdx;
    else
      GetWindowRect(gHWND, &mDockState.r);
  }

  // Set visible bit based on window state
  if (gHWND != NULL && IsWindowVisible(gHWND))
    mDockState.state |= 1;
  else
    mDockState.state &= ~1;

  // Convert to little-endian for cross-platform compatibility
  ReaperExtDockState stateLE;
  memcpy(&stateLE, &mDockState, sizeof(ReaperExtDockState));
  for (int i = 0; i < (int)(sizeof(ReaperExtDockState) / sizeof(int)); i++)
    REAPER_MAKELEINTMEM(&((int*)&stateLE)[i]);

  WritePrivateProfileStruct("iPlug2", mDockId.Get(), &stateLE, sizeof(ReaperExtDockState), iniFile);
}

void ReaperExtBase::LoadDockState()
{
  const char* iniFile = get_ini_file();
  if (!iniFile)
    return;

  ReaperExtDockState stateLE;
  if (GetPrivateProfileStruct("iPlug2", mDockId.Get(), &stateLE, sizeof(ReaperExtDockState), iniFile))
  {
    // Convert from little-endian
    for (int i = 0; i < (int)(sizeof(ReaperExtDockState) / sizeof(int)); i++)
      REAPER_MAKELEINTMEM(&((int*)&stateLE)[i]);
    memcpy(&mDockState, &stateLE, sizeof(ReaperExtDockState));
  }
}

void ReaperExtBase::RegisterAction(const char* actionName, std::function<void()> func, bool addMenuItem, int* pToggle, const char* contextMenuId, const char* menuLabel/*, IKeyPress keyCmd*/)
{
  ReaperAction action;

  int commandID = mRec->Register("command_id", (void*) actionName /* ?? */);

  assert(commandID);

  action.func = func;
  action.accel.accel.cmd = commandID;
  action.accel.desc = actionName;
  action.addMenuItem = addMenuItem;
  action.pToggle = pToggle;

  action.contextMenuId = contextMenuId;
  action.menuLabel = menuLabel ? menuLabel : actionName;
  gActions.push_back(action);

  mRec->Register("gaccel", (void*) &gActions.back().accel);
}

/** Sets the check state of the item with the given command id, searching nested submenus.
 * SWELL's CheckMenuItem() searches submenus when passed MF_BYCOMMAND, but the native Win32
 * one is not documented to, and our items live in a submenu we create. So find the item
 * explicitly and check it by position, which is unambiguous on both platforms.
 * @return true if the item was found */
static bool CheckActionMenuItem(HMENU hMenu, int commandId, bool checked)
{
  const int nItems = GetMenuItemCount(hMenu);

  for (int i = 0; i < nItems; i++)
  {
    MENUITEMINFO mi = { sizeof(MENUITEMINFO), };
    mi.fMask = MIIM_ID | MIIM_SUBMENU;

    if (!GetMenuItemInfo(hMenu, i, TRUE, &mi))
      continue;

    if (mi.hSubMenu)
    {
      if (CheckActionMenuItem(mi.hSubMenu, commandId, checked))
        return true;
    }
    else if (static_cast<int>(mi.wID) == commandId)
    {
      CheckMenuItem(hMenu, i, MF_BYPOSITION | (checked ? MF_CHECKED : MF_UNCHECKED));
      return true;
    }
  }

  return false;
}

static void AppendActionMenuItem(HMENU hMenu, const ReaperAction& action)
{
  MENUITEMINFO mi = { sizeof(MENUITEMINFO), };
  mi.fMask = MIIM_TYPE | MIIM_ID;
  mi.fType = MFT_STRING;
  mi.dwTypeData = LPSTR(action.menuLabel);
  mi.wID = action.accel.accel.cmd;
  // Append to the end of the menu (works regardless of user customization)
  InsertMenuItem(hMenu, GetMenuItemCount(hMenu), TRUE, &mi);
}

//static
void ReaperExtBase::MenuHook(const char* menuidstr, void* menu, int flag)
{
  if (menuidstr == nullptr || menu == nullptr)
    return;

  HMENU hMenu = (HMENU) menu;

  const bool isExtensionsMenu = strcmp(menuidstr, "Main extensions") == 0;

  // flag==1: the menu is about to be shown. Per the SDK this - not flag==0 - is where
  // check/grayed states are set, so toggle actions get a tick when they're on.
  if (flag == 1)
  {
    for (auto& action : gActions)
    {
      const bool inThisMenu = (isExtensionsMenu && action.addMenuItem) ||
                              (action.contextMenuId && strcmp(action.contextMenuId, menuidstr) == 0);

      if (!inThisMenu || action.pToggle == nullptr)
        continue;

      CheckActionMenuItem(hMenu, action.accel.accel.cmd, *action.pToggle != 0);
    }

    return;
  }

  // flag==0: the default menu is being initialized; this is when we may add items.
  if (flag != 0)
    return;

  // The main Extensions menu, added by AddExtensionsMainMenu(). Give this extension its
  // own submenu rather than adding items directly, so several extensions can coexist.
  if (isExtensionsMenu)
  {
    HMENU hSubMenu = CreatePopupMenu();

    for (auto& action : gActions)
    {
      if (action.addMenuItem)
        AppendActionMenuItem(hSubMenu, action);
    }

    if (GetMenuItemCount(hSubMenu) == 0)
    {
      DestroyMenu(hSubMenu);
      return;
    }

    MENUITEMINFO mi = { sizeof(MENUITEMINFO), };
    mi.fMask = MIIM_TYPE | MIIM_SUBMENU;
    mi.fType = MFT_STRING;
    mi.dwTypeData = LPSTR(gPlug->mMenuName.Get());
    mi.hSubMenu = hSubMenu; // owned by hMenu from here on; don't retain the handle
    InsertMenuItem(hMenu, GetMenuItemCount(hMenu), TRUE, &mi);

    return;
  }

  for (auto& action : gActions)
  {
    if (action.contextMenuId && strcmp(action.contextMenuId, menuidstr) == 0)
      AppendActionMenuItem(hMenu, action);
  }
}

//static
void ReaperExtBase::PostCommandProc(int command, int flag)
{
  if (gPlug)
    gPlug->OnActionRun(command, flag);
}

//static
void ReaperExtBase::BeginLoadProjectState(bool isUndo, project_config_extension_t* reg)
{
  if (gPlug)
    gPlug->OnBeginLoadProjectState(isUndo);
}

//static
bool ReaperExtBase::ProcessExtensionLine(const char* line, ProjectStateContext* ctx, bool isUndo, project_config_extension_t* reg)
{
  return gPlug ? gPlug->LoadProjectStateLine(line) : false;
}

//static
void ReaperExtBase::SaveExtensionConfig(ProjectStateContext* ctx, bool isUndo, project_config_extension_t* reg)
{
  if (gPlug)
    gPlug->SaveProjectState(ctx);
}

//static
bool ReaperExtBase::HookCommandProc(int command, int flag)
{
  auto it = std::find_if (gActions.begin(), gActions.end(), [&](const auto& e) { return e.accel.accel.cmd == command; });

  if (it == gActions.end())
    return false; // not ours - let REAPER pass it to the next hook

  it->func();

  return true; // handled; stop further hooks and the default action from running
}

//static
int ReaperExtBase::ToggleActionCallback(int command)
{
  auto it = std::find_if (gActions.begin(), gActions.end(), [&](const auto& e) { return e.accel.accel.cmd == command; });

  // -1 means "not this extension's action, or it doesn't toggle". Returning 0 here would
  // claim every other extension's commands and report them as off, since REAPER walks the
  // registered toggleaction hooks until one of them answers.
  if (it == gActions.end() || it->pToggle == nullptr)
    return -1;

  return *it->pToggle;
}

//static
WDL_DLGRET ReaperExtBase::MainDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  extern float GetScaleForHWND(HWND hWnd);

  switch (uMsg)
  {
    case WM_INITDIALOG:
    {
      auto scale = GetScaleForHWND(hwnd);

      if (gPlug->IsDocked())
      {
        // Docked: register with dock system
        DockWindowAddEx(hwnd, (char*)gPlug->mDockId.Get(), gPlug->mDockId.Get(), true);
      }
      else
      {
        // Floating: restore position and show
        if (gPlug->mDockState.r.left || gPlug->mDockState.r.top ||
            gPlug->mDockState.r.right || gPlug->mDockState.r.bottom)
        {
          EnsureNotCompletelyOffscreen(&gPlug->mDockState.r);
          SetWindowPos(hwnd, NULL,
                       gPlug->mDockState.r.left, gPlug->mDockState.r.top,
                       gPlug->mDockState.r.right - gPlug->mDockState.r.left,
                       gPlug->mDockState.r.bottom - gPlug->mDockState.r.top,
                       SWP_NOZORDER);
        }
        else
        {
          ClientResize(hwnd, static_cast<int>(PLUG_WIDTH * scale), static_cast<int>(PLUG_HEIGHT * scale));
        }
        AttachWindowTopmostButton(hwnd);
        ShowWindow(hwnd, SW_SHOW);
      }

      gPlug->OpenWindow(hwnd);

      // Trigger initial resize now that IGraphics exists
      // (WM_SIZE during SetWindowPos/DockWindowAddEx above fires before OpenWindow)
      {
        RECT r;
        GetClientRect(hwnd, &r);
        int w = r.right - r.left;
        int h = r.bottom - r.top;
        if (w > 0 && h > 0)
          gPlug->OnParentWindowResize(w, h);
      }

      GetWindowRect(hwnd, &gPrevBounds);
      
      return 0;
    }
    case WM_DESTROY:
    {
      if (gPlug->mSaveStateOnDestroy)
        gPlug->SaveDockState();

      DockWindowRemove(hwnd);
      gHWND = NULL;
      gPlug->UpdateToggleStates(); // also covers the user closing the window directly
      return 0;
    }
    case WM_SIZE:
    {
#ifndef NO_IGRAPHICS
      if (gPlug->GetUI())
#endif
      {
        RECT r;
        GetClientRect(hwnd, &r);
        int w = r.right - r.left;
        int h = r.bottom - r.top;
        if (w > 0 && h > 0)
          gPlug->OnParentWindowResize(w, h);
      }
      return 0;
    }
    case WM_CLOSE:
      gPlug->CloseWindow();
      DestroyWindow(hwnd);
      return 0;
  }
  return 0;
}
