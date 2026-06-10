// Helper to stringify macro
#define IPLUG_STRINGIFY_HELPER(x) #x
#define IPLUG_STRINGIFY(x) IPLUG_STRINGIFY_HELPER(x)

ReaperExtBase::ReaperExtBase(reaper_plugin_info_t* pRec)
: EDITOR_DELEGATE_CLASS(0) // zero params
, mRec(pRec)
{
  mTimer = std::unique_ptr<Timer>(Timer::Create(std::bind(&ReaperExtBase::OnTimer, this, std::placeholders::_1), IDLE_TIMER_RATE));
  mDockId.Set(IPLUG_STRINGIFY(PLUG_CLASS_NAME));
  memset(&mDockState, 0, sizeof(ReaperExtDockState));
  // Note: LoadDockState() is called lazily in CreateMainWindow() after API imports
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

void ReaperExtBase::CreateMainWindow()
{
  if (gHWND != NULL)
    return;

  // Lazy load state on first window creation (after API imports are done)
  if (!mStateLoaded)
  {
    LoadDockState();
    mStateLoaded = true;
  }

  gHWND = CreateDialog(gHINSTANCE, MAKEINTRESOURCE(IDD_DIALOG_MAIN), gParent, ReaperExtBase::MainDlgProc);
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
  if (gHWND == NULL)
    return;

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

void ReaperExtBase::RegisterAction(const char* actionName, std::function<void()> func, bool addMenuItem, int* pToggle/*, IKeyPress keyCmd*/)
{
  ReaperAction action;
  
  int commandID = mRec->Register("command_id", (void*) actionName /* ?? */);
  
  assert(commandID);
  
  action.func = func;
  action.accel.accel.cmd = commandID;
  action.accel.desc = actionName;
  action.addMenuItem = addMenuItem;
  action.pToggle = pToggle;
  
  gActions.push_back(action);
  
  mRec->Register("gaccel", (void*) &gActions.back().accel);
}

//static
bool ReaperExtBase::HookCommandProc(int command, int flag)
{
  std::vector<ReaperAction>::iterator it = std::find_if (gActions.begin(), gActions.end(), [&](const auto& e) { return e.accel.accel.cmd == command; });

  if(it != gActions.end())
  {
    it->func();
  }
  
  return false;
}

//static
int ReaperExtBase::ToggleActionCallback(int command)
{
  std::vector<ReaperAction>::iterator it = std::find_if (gActions.begin(), gActions.end(), [&](const auto& e) { return e.accel.accel.cmd == command; });
  
  if(it != gActions.end())
  {
    if(it->pToggle == nullptr)
      return -1;
    else
      return *it->pToggle;
  }
  
  return 0;
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
      return 0;
    }
    case WM_SIZE:
    {
      if (gPlug->GetUI())
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
