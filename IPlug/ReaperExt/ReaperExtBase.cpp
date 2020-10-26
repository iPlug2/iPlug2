ReaperExtBase::ReaperExtBase(reaper_plugin_info_t* pRec)
: EDITOR_DELEGATE_CLASS(0) // zero params
, mRec(pRec)
{
  mTimer = std::unique_ptr<Timer>(Timer::Create(std::bind(&ReaperExtBase::OnTimer, this, std::placeholders::_1), IDLE_TIMER_RATE));
}

ReaperExtBase::~ReaperExtBase()
{
  mTimer->Stop();
};

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
#ifdef OS_MAC
#define TITLEBAR_BODGE 22 //TODO: sort this out
    RECT r;
    GetWindowRect(gHWND, &r);
    SetWindowPos(gHWND, 0, r.left, r.bottom - viewHeight - TITLEBAR_BODGE, viewWidth, viewHeight + TITLEBAR_BODGE, 0);
#endif

    return true;
  }

  return false;
}

void ReaperExtBase::ShowHideMainWindow()
{
  if(gHWND == NULL)
  {
    gHWND = CreateDialog(gHINSTANCE, MAKEINTRESOURCE(IDD_DIALOG_MAIN), gParent, ReaperExtBase::MainDlgProc);
  }
  else
    DestroyWindow(gHWND);
}

void ReaperExtBase::ToggleDocking()
{
  if (!mDocked)
  {
    mDocked = true;
    ShowWindow(gHWND, SW_HIDE);
    DockWindowAdd(gHWND, (char*) "TEST", 0, false);
    DockWindowActivate(gHWND);
  }
  else
  {
    DestroyWindow(gHWND);
    mDocked = false;
//    Show(false, true);
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
//  auto Resize = [&]()
//  {
//    RECT r;
//    GetWindowRect(hwnd, &r);
//    if(memcmp((void*) &r, (void*) &gPrevBounds, sizeof(RECT)) > 0)
//      gPlug->GetUI()->Resize(r.right-r.left, r.bottom-r.top, 1);
//
//    gPrevBounds = r;
//  };

  switch (uMsg)
  {
    case WM_INITDIALOG:
    {
      AttachWindowTopmostButton(hwnd);
      gPlug->OpenWindow(hwnd);
      ClientResize(hwnd, PLUG_WIDTH, PLUG_HEIGHT);
      ShowWindow(hwnd, SW_SHOW);
      GetWindowRect(hwnd, &gPrevBounds);
      
      return 0;
    }
    case WM_DESTROY:
      gHWND = NULL;
      return 0;
    case WM_CLOSE:
      gPlug->CloseWindow();
      DestroyWindow(hwnd);
      return 0;
//    case WM_SIZE:
//    {
      //Resize();
//      return 0;
//    }
  }
  return 0;
}
