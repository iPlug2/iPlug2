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

void ReaperExtBase::EditorPropertiesChangedFromUI(int viewWidth, int viewHeight, const IByteChunk& data)
{
  //TODO: handle iplug corner resizer
}

void ReaperExtBase::ShowHideMainWindow()
{
  if(gHWND == NULL)
  {
    gHWND = CreateDialog(NULL, MAKEINTRESOURCE(IDD_DIALOG_MAIN), 0, ReaperExtBase::MainDlgProc); // TODO: why is this not finding the dialog on windows?
  }
  else
    DestroyWindow(gHWND);
}

void ReaperExtBase::RegisterAction(const char* actionName, std::function<void()> func, bool addMenuItem, int* pToggle/*, IKeyPress keyCmd*/)
{
  action action;
  
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
  std::vector<action>::iterator it = std::find_if (gActions.begin(), gActions.end(), [&](const auto& e) { return e.accel.accel.cmd == command; });

  if(it != gActions.end())
  {
    it->func();
  }
  
  return false;
}

//static
int ReaperExtBase::ToggleActionCallback(int command)
{
  std::vector<action>::iterator it = std::find_if (gActions.begin(), gActions.end(), [&](const auto& e) { return e.accel.accel.cmd == command; });
  
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
  auto Resize = [&]()
  {
    RECT r;
    GetWindowRect(hwnd, &r);
    if(memcmp((void*) &r, (void*) &gPrevBounds, sizeof(RECT)) > 0)
      gPlug->GetUI()->Resize(r.right-r.left, r.bottom-r.top, 1);
    
    gPrevBounds = r;
  };
  
  switch (uMsg)
  {
    case WM_INITDIALOG:
    {
      AttachWindowTopmostButton(hwnd);
      gPlug->OpenWindow(hwnd);
      ShowWindow(hwnd, SW_SHOW);
      GetWindowRect(hwnd, &gPrevBounds);
      
      return 0;
    }
    case WM_DESTROY:
      gHWND = NULL;
      return 0;
    case WM_CLOSE:
      KillTimer(hwnd, -1);
      gPlug->CloseWindow();
      DestroyWindow(hwnd);
      return 0;
    case WM_SIZE:
    {
      Resize();
      return 0;
    }
  }
  return 1;
}
