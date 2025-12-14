/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include <stddef.h>
#include <cstdlib>
#include <wdlutf8.h>

#include "IPlugParameter.h"
#include "IGraphicsLinux.h"
#include "IPlugTaskThread.h"
#include "IControl.h"
#include "IPlugPaths.h"
#include <unistd.h>
#include <sys/wait.h>
#include <mutex.h>
#include <thread>
#include <future>

#ifdef OS_LINUX
  #ifdef IGRAPHICS_GL
    #include <glad/glad_glx.h>
  #endif
  #include <fontconfig/fontconfig.h>
#endif

#define NOT_IMPLEMENTED printf("%s: not implemented\n", __FUNCTION__);

using namespace iplug;
using namespace igraphics;

/// @brief Global platform backend instance
static PlatformX11* gPlatform = nullptr;
/// @brief Protects the creation of gPlatform, does not protect use
static WDL_Mutex gPlatformLock;

class IGraphicsLinux::Font : public PlatformFont
{
public:
  Font(WDL_String &fileName)
  : PlatformFont(false)
  , mFileName(fileName)
  {
    mFontData.Resize(0);
  }

  Font(WDL_String &fontID, const void* pData, int dataSize)
  : PlatformFont(false)
  , mFileName(fontID)
  {
    mFontData.Set((const uint8_t*)pData, dataSize);
  }

  IFontDataPtr GetFontData() override;

private:
  WDL_String mFileName;
  WDL_TypedBuf<uint8_t> mFontData;
};

/** Run a child process with some input to stdin, and retrieve its stdout and exit status.
 * @param command Command-line to be passed to /bin/sh
 * @param subStdout Stores the stdout of the subprocess
 * @param subStdin The contents of stdin for the subprocess (may be empty)
 * @param exitStatus Output, exit status of the child process
 * @return 0 on success, a negative value on failure */
static int RunSubprocess(char* command, WDL_String& subStdout, const WDL_String& subStdin, int* exitStatus)
{
  int pipeOut[2];
  int pipeIn[2];
  pid_t cpid;

  auto closePipeOut = [&]() {
    close(pipeOut[0]);
    close(pipeOut[1]);
  };
  auto closePipeIn = [&]() {
    close(pipeIn[0]);
    close(pipeIn[1]);
  };

  if (pipe(pipeOut) == -1)
  {
    return -1;
  }

  if (pipe(pipeIn) == -1)
  {
    closePipeOut();
    return -2;
  }

  cpid = fork();
  if (cpid == -1)
  {
    closePipeOut();
    closePipeIn();
    return -3;
  }

  if (cpid == 0)
  {
    // Child process
    // Close unneeded pipes
    close(pipeIn[1]);
    close(pipeOut[0]);
    // Replace stdout/stderr and stdin
    dup2(pipeOut[1], STDOUT_FILENO);
    dup2(pipeOut[1], STDERR_FILENO);
    dup2(pipeIn[0], STDIN_FILENO);
    // Replace the child process with the new process
    WDL_String arg0 { "/bin/sh" };
    WDL_String arg1 { "-c" };
    char* args[] = { arg0.Get(), arg1.Get(), command, nullptr };
    int status = execv("/bin/sh", args);
    close(pipeIn[0]);
    close(pipeOut[1]);
    exit(status);
  }
  else
  {
    // Parent process

    // Close unneeded pipes
    close(pipeIn[0]);
    close(pipeOut[1]);

    // We write all contents to stdin and read from stdout until it's empty.
    ssize_t written = 0;
    while (written < subStdin.GetLength())
    {
      write(pipeIn[1], subStdin.Get() + written, subStdin.GetLength() - written);
    }

    int status;
    if (waitpid(cpid, &status, 0) == -1)
    {
      closePipeOut();
      closePipeIn();
      return -4;
    }
    *exitStatus = WEXITSTATUS(status);

    WDL_HeapBuf hb;
    hb.Resize(4096);
    while (true)
    {
      ssize_t sz = read(pipeOut[0], hb.Get(), hb.GetSize());
      if (sz == 0)
      {
        break;
      }
      subStdout.Append((const char*)hb.Get(), sz);
    }

    closePipeOut();
    closePipeIn();
  }
  return 0;
}

static uint64_t GetTimeMs()
{
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC_RAW, &t);
  return (t.tv_sec * 1000) + (t.tv_nsec / 1000000);
}

void IGraphicsLinux::Paint()
{
  IRECT ir = {0, 0, static_cast<float>(WindowWidth()), static_cast<float>(WindowHeight())};
  IRECTList rects;
  rects.Add(ir.GetScaled(1.f / GetBackingPixelScale()));

  if (mWindow->DrawBegin())
  {
    Draw(rects);
    mWindow->DrawEnd();
  }
}

void IGraphicsLinux::DrawResize()
{
  // WARNING: in CAN BE reentrant!!! (f.e. it is called from SetScreenScale during initialization)
  if (mWindow->DrawBegin())
  {
    IGRAPHICS_DRAW_CLASS::DrawResize();
    mWindow->DrawEnd();
  }
  // WARNING: IPlug call it on resize, but at the end. When should we call Paint() ?
  // In Windows version "Update window" is called from PlatformResize, so BEFORE DrawResize...
}

void IGraphicsLinux::LoopEvents()
{
  bool resetMouse = false;
  SDL_Event event;
  while (mWindow->PollEvent(&event)) {
    switch (event.type) {
      case SDL_EVENT_WINDOW_EXPOSED:
      {
        mShouldPaint = true;
        break;
      }
      case SDL_EVENT_MOUSE_BUTTON_DOWN:
      case SDL_EVENT_MOUSE_BUTTON_UP:
      {
        IMouseInfo info;
        info.x = event.button.x;
        info.y = event.button.y;
        info.dX = 0;
        info.dX = 0;
        info.ms.L = event.button.button == SDL_BUTTON_LEFT;
        info.ms.R = event.button.button == SDL_BUTTON_RIGHT;
        // TODO add modifiers to button event
        info.ms.C = false;
        info.ms.A = false;
        info.ms.S = false;

        // Keeping these TODOs around in case I need them later
        // TODO: end parameter editing (if in progress, and return then)
        // TODO: set focus
        // TODO: set capture (or after capture...) (but check other buttons first)

        if (event.button.down) {
          if (event.button.clicks > 1) {
            if (OnMouseDblClick(info.x, info.y, info.ms)) {
              // TODO: SetCapture(hWnd);
            }
          } else {
            std::vector<IMouseInfo> items {info};
            OnMouseDown(items);
            // TODO: is this necessary?
            // RequestFocus();
          }
        } else {
          std::vector<IMouseInfo> items {info};
          OnMouseUp(items);
        }
        break;
      }
      case SDL_EVENT_KEY_DOWN:
      case SDL_EVENT_KEY_UP:
      {
        char buf[16];
        // TODO new events should have space for character string
        IKeyPress kp {buf, (int)event.key.key};
        kp.A = event.key.mod & SDL_KMOD_ALT;
        kp.S = event.key.mod & SDL_KMOD_SHIFT;
        kp.C = event.key.mod & SDL_KMOD_CTRL;
        if (event.type == SDL_EVENT_KEY_DOWN) {
          OnKeyDown(0, 0, kp);
        } else {
          OnKeyUp(0, 0, kp);
        }
        break;
      }
      case SDL_EVENT_MOUSE_MOTION:
      {
        IMouseInfo info;
        info.x = event.motion.x;
        info.y = event.motion.y;
        info.dX = event.motion.xrel;
        info.dX = event.motion.yrel;
        info.ms.L = event.motion.state == SDL_BUTTON_LEFT;
        info.ms.R = event.motion.state == SDL_BUTTON_RIGHT;
        // TODO add modifiers to button event
        info.ms.C = false;
        info.ms.A = false;
        info.ms.S = false;

        mCursorX = info.x;
        mCursorY = info.y;

        // check if we have to move the mouse back to the known position
        if (mCursorLock && (mMouseLockPos.x != mCursorX || mMouseLockPos.y != mCursorY)) {
          resetMouse = true;
        }

        OnMouseOver(info.x, info.y, info.ms);

        if (info.ms.L || info.ms.R) {
          std::vector<IMouseInfo> list{ info };
          OnMouseDrag(list);
        }
        break;
      }
      case SDL_EVENT_MOUSE_WHEEL:
      {
        IMouseMod mod;
        OnMouseWheel(event.wheel.mouse_x, event.wheel.mouse_y, mod, event.wheel.y);
        break;
      }
      case SDL_EVENT_WINDOW_MOUSE_ENTER:
        break;
      case SDL_EVENT_WINDOW_MOUSE_LEAVE:
      {
        OnMouseOut();
        break;
      }
    }
  }
  // if (resetMouse) {
  //   mWindow->MoveMouse(MOUSE_MOVE_WINDOW, mMouseLockPos.x, mMouseLockPos.y);
  // }
}

void* IGraphicsLinux::OpenWindow(void* pParent)
{
  WindowOptions opts;
  opts.bounds = WRect{};
  opts.bounds.w = (uint32_t)WindowWidth();
  opts.bounds.h = (uint32_t)WindowHeight();
  // NOTE: In case plug-in report REAPER extension in REAPER, pParent is NOT XID (SWELL HWND? I have not checked yet)
  opts.parent = pParent;
  #ifdef IGRAPHICS_GL
  #ifdef IGRAPHICS_GL2
  opts.glMajor = 2;
  opts.glMinor = 1;
  #elif defined IGRAPHICS_GL3
  opts.glMajor = 3;
  opts.glMinor = 3;
  #else
  #error "Unsupported GL version"
  #endif
  #endif

  mWindow = gPlatform->CreateWindow(opts);
  if (!mWindow)
  {
    return NULL;
  }

#if defined(VST3_API)
  mWindow->SetVisible(true);
#elif defined(APP_API) || defined(CLAP_API) || defined(VST2_API)
  mWindow->SetVisible(true);
  mTaskId = IPlugTaskThread::instance()->Push(Task::FromMs(0, 10, [this](uint64_t) {
    this->UpdateUI();
    return true;
  }));
#else
  #error "IGraphicsLinux:OpenWindow: unknown api. Map or not to map... that is the question"
#endif

  std::promise<bool> done1;
  AddUiTask([this, opts, &done1]() {
    // GL context set
    if (mWindow->DrawBegin())
    {
      OnViewInitialized(nullptr);
      SetScreenScale(1); // resizes draw context, calls DrawResize

      GetDelegate()->LayoutUI(this);
      SetAllControlsDirty();
      GetDelegate()->OnUIOpen();

      mWindow->DrawEnd();
    }
    done1.set_value(true);
  });
  done1.get_future().wait();

  // Reset some state
  mCursorLock = false;
  mNextDrawTime = 0;

  return mWindow->GetHandle();
}

void IGraphicsLinux::CloseWindow()
{
  if (mWindow) {
    std::promise<bool> closeWait;
    AddUiTask([this, &closeWait]() {
      OnViewDestroyed();
      SetPlatformContext(nullptr);
      mWindow->Close();
      mWindow = nullptr;
      closeWait.set_value(true);
    });
    closeWait.get_future().wait();
  }

  if (mTaskId) {
    IPlugTaskThread::instance()->Cancel(mTaskId);
    mTaskId = 0;
  }
}

void IGraphicsLinux::GetMouseLocation(float& x, float& y) const
{
  x = mCursorX;
  y = mCursorY;
}

void IGraphicsLinux::HideMouseCursor(bool hide, bool lock)
{
  mWindow->SetCursorVisible(!hide);
  mWindow->SetCursorGrabbed(lock);
  mCursorHidden = hide;
  mCursorLock = lock;
  mMouseLockPos.x = mCursorX;
  mMouseLockPos.y = mCursorY;
}

void IGraphicsLinux::MoveMouseCursor(float x, float y)
{
  mWindow->MoveMouse(MOUSE_MOVE_WINDOW, (int)x, (int)y);
}

EMsgBoxResult IGraphicsLinux::ShowMessageBox(const char* text, const char* caption, EMsgBoxType type, IMsgBoxCompletionHandlerFunc completionHandler)
{
  WDL_String command;
  WDL_String argument;

  auto pushArgument = [&]() {
    command.Append(&argument);
    command.Append(" ", 2);
  };

  argument.Set("zenity");
  pushArgument();

  argument.Set("--modal");
  pushArgument();

  argument.SetFormatted(strlen(caption) + 10, "\"--title=%s\"", caption);
  pushArgument();

  argument.SetFormatted(strlen(text) + 10, "\"--text=%s\"", text);
  pushArgument();

  switch (type)
  {
    case EMsgBoxType::kMB_OK:
      argument.Set("--info");
      pushArgument();
      break;
    case EMsgBoxType::kMB_OKCANCEL:
      argument.SetFormatted(64, "--ok-label=Ok");
      pushArgument();
      argument.SetFormatted(64, "--cancel-label=Cancel");
      pushArgument();
      argument.Set("--question");
      pushArgument();
      break;
    case EMsgBoxType::kMB_RETRYCANCEL:
      argument.SetFormatted(64, "--ok-label=Retry");
      pushArgument();
      argument.SetFormatted(64, "--cancel-label=Cancel");
      pushArgument();
      argument.Set("--question");
      pushArgument();
      break;
    case EMsgBoxType::kMB_YESNO:
      argument.Set("--question");
      pushArgument();
      break;
    case EMsgBoxType::kMB_YESNOCANCEL:
      argument.Set("--question");
      pushArgument();
      argument.Set("--extra-button=Cancel");
      pushArgument();
      break;
  }

  EMsgBoxResult result = kNoResult;
  WDL_String sStdout;
  WDL_String sStdin;
  int status;

  if (RunSubprocess(command.Get(), sStdout, sStdin, &status) != 0)
  {
    if (completionHandler)
      completionHandler(result);

    return result;
  }

  switch (type)
  {
    case EMsgBoxType::kMB_OK:
      result = kOK;
      break;
    case EMsgBoxType::kMB_OKCANCEL:
      switch (status)
      {
        case 0:
          result = kOK;
          break;
        default:
          result = kCANCEL;
          break;
      }
      break;
    case EMsgBoxType::kMB_RETRYCANCEL:
      switch (status)
      {
        case 0:
          result = kRETRY;
          break;
        default:
          result = kCANCEL;
          break;
      }
      break;
    case EMsgBoxType::kMB_YESNO:
      switch (status)
      {
        case 0:
          result = kYES;
          break;
        default:
          result = kNO;
          break;
      }
      break;
    case EMsgBoxType::kMB_YESNOCANCEL:
      switch (status)
      {
        case 0:
          result = kYES;
          break;
        case 1:
          // zenity output our extra button text
          if (sStdout.GetLength() > 0)
            result = kCANCEL;
          else
            result = kNO;
          break;
        default:
          result = kCANCEL;
          break;
      }
      break;
  }

  if (completionHandler)
    completionHandler(result);

  return result;
}

bool IGraphicsLinux::RevealPathInExplorerOrFinder(WDL_String& path, bool select)
{
  WDL_String args;
  args.SetFormatted(path.GetLength() + 40, "xdg-open \"%s\" ", path.Get());

  WDL_String sOut, sIn;
  int status;

  if (RunSubprocess(args.Get(), sOut, sIn, &status) != 0)
  {
    return false;
  }

  return true;
}

void IGraphicsLinux::PromptForFile(
  WDL_String& fileName,
  WDL_String& path,
  EFileAction action,
  const char* extensions,
  IFileDialogCompletionHandlerFunc completionHandler)
{
  if (!WindowIsOpen())
  {
    fileName.Set("");
    return;
  }

  WDL_String tmp;
  WDL_String args;
  args.AppendFormatted(path.GetLength() + 10, "cd \"%s\"; ", path.Get());
  args.Append("zenity --file-selection ");

  if (action == EFileAction::Save)
  {
    args.Append("--save --confirm-overwrite ");
  }
  if (fileName.GetLength() > 0)
  {
    args.AppendFormatted(fileName.GetLength() + 20, "\"--filename=%s\" ", fileName.Get());
  }

  if (extensions)
  {
    // Split the string at commas and then append each format specifier
    const char* ext = extensions;
    while (*ext)
    {
      const char* start = ext;
      while (*ext && *ext != ',')
        ext++;
      tmp.Set(start, ext - start);
      args.AppendFormatted(256, "\"--file-filter=%s | %s\" ", tmp.Get(), tmp.Get());
      if (!(*ext))
        ext++;
    }
  }

  WDL_String sStdout;
  WDL_String sStdin;
  int status;

  if (RunSubprocess(args.Get(), sStdout, sStdin, &status) != 0)
  {
    fileName.Set("");
    return;
  }

  if (status == 0)
  {
    fileName.Set(sStdout.Get());
  }
  else
  {
    fileName.Set("");
  }

  // TODO call completionHandler
}

void IGraphicsLinux::PromptForDirectory(WDL_String& dir, IFileDialogCompletionHandlerFunc completionHandler)
{
  if (!WindowIsOpen())
  {
    dir.Set("");
    return;
  }

  WDL_String args;
  args.Append("zenity --file-selection --directory ");
  if (dir.GetLength() > 0)
  {
    args.AppendFormatted(dir.GetLength() + 20, "\"--filename=%s\"", dir.Get());
  }

  WDL_String sStdout;
  WDL_String sStdin;
  int status;
  if (RunSubprocess(args.Get(), sStdout, sStdin, &status) != 0)
  {
    dir.Set("");
    return;
  }

  if (status == 0)
  {
    dir.Set(sStdout.Get());
  }
  else
  {
    dir.Set("");
  }

  // TODO call completionHandler
}

bool IGraphicsLinux::PromptForColor(IColor& color, const char* str, IColorPickerHandlerFunc func)
{
  WDL_String args;
  args.Append("zenity --color-selection ");

  if (str)
    args.AppendFormatted(strlen(str) + 20, "\"--title=%s\" ", str);

  args.AppendFormatted(100, "\"--color=rgba(%d,%d,%d,%f)\" ", color.R, color.G, color.B, (float)color.A / 255.f);

  bool ok = false;

  WDL_String sOut;
  WDL_String sIn;
  int status;

  if (RunSubprocess(args.Get(), sOut, sIn, &status) != 0)
  {
    return false;
  }
  else
  {
    if (strncmp(sOut.Get(), "rgba", 4) == 0)
    {
      // Parse RGBA
      int cr, cg, cb;
      float ca;
      int ct = sscanf(sOut.Get(), "rgba(%d,%d,%d,%f)", &cr, &cg, &cb, &ca);
      if (ct == 4)
      {
        color = IColor((int)(255.f * ca), cr, cg, cb);
        ok = true;
      }
    }
    else if (strncmp(sOut.Get(), "rgb", 3) == 0)
    {
      // Parse RGB
      int cr, cg, cb;
      int ct = sscanf(sOut.Get(), "rgb(%d,%d,%d)", &color.R, &color.G, &color.B);
      if (ct == 3)
      {
        color = IColor(255, cr, cg, cb);
        ok = true;
      }
    }

    if (ok && func)
      func(color);

    return ok;
  }

  // TODO call func
}

IPopupMenu* IGraphicsLinux::CreatePlatformPopupMenu(IPopupMenu& menu, const IRECT bounds, bool& isAsync)
{
  // Not implemented yet
  return nullptr;
}

void IGraphicsLinux::CreatePlatformTextEntry(int paramIdx, const IText& text, const IRECT& bounds, int length, const char* str)
{
  (void)text;
  (void)bounds;
  (void)length;

  WDL_String args;
  uint32_t windowId = (uint32_t)reinterpret_cast<uintptr_t>(mWindow->GetHandle());
  const char* paramName = GetDelegate()->GetParam(paramIdx)->GetName();

  args.Append("zenity --modal --entry ");
  args.AppendFormatted(100, "--title='Edit %s' ", paramName);
  args.AppendFormatted(50, "--attach=%u ", windowId);
  args.AppendFormatted(30 + strlen(str), "--entry-text='%s' ", str);

  FILE* fd = popen(args.Get(), "r");
  if (!fd) {
    return;
  }

  auto self = this;
  auto popup = std::thread([fd, paramIdx, self]() {
    char buf[2048];
    size_t sz = fread(buf, 1, sizeof(buf), fd);
    buf[sz] = 0;
    int code = pclose(fd);
    if (code == 0) {
      IPlugTaskThread::instance()->AddOnce([buf, paramIdx, self](uint64_t) {
        auto param = self->GetDelegate()->GetParam(paramIdx);
        param->SetString(buf);
        self->GetControlWithParamIdx(paramIdx)->SetValueFromUserInput(param->GetNormalized());
        return false;
      });
    }
  });
  popup.detach();
}

bool IGraphicsLinux::OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure)
{
  (void)msgWindowTitle;
  (void)confirmMsg;
  (void)errMsgOnFailure;

  // TODO inform the user of the failure via GUI popup instead of just returning false.
  WDL_String args;
  args.SetFormatted(strlen(url) + 40, "xdg-open \"%s\" ", url);
  FILE* fd = popen(args.Get(), "w");
  if (!fd) {
    return false;
  }
  if (pclose(fd) != 0) {
    return false;
  }
  return true;
}

bool IGraphicsLinux::GetTextFromClipboard(WDL_String& str)
{
  EClipboardFormat format = CLIPBOARD_FORMAT_UTF8;
  WDL_TypedBuf<uint8_t> data {1024};
  if (gPlatform->GetClipboard(&format, &data)) {
    str.Append((char*)data.Get(), data.GetSize());
    return true;
  } else {
    return false;
  }
}

bool IGraphicsLinux::SetTextInClipboard(const char* str)
{
  gPlatform->SetClipboard(CLIPBOARD_FORMAT_UTF8, str, strlen(str));
  return true;
}

void IGraphicsLinux::PlatformResize(bool parentHasResized)
{
  if (WindowIsOpen()) {
    uint32_t w = (uint32_t)(WindowWidth() * GetScreenScale());
    uint32_t h = (uint32_t)(WindowHeight() * GetScreenScale());
    mWindow->Resize(w, h);
    if (!parentHasResized) {
      DBGMSG("WARNING: parent is not resized, but I (should) have no control on it on X... XEMBED?\n");
    }
  }
}

void IGraphicsLinux::RequestFocus()
{
  if (mWindow) {
    mWindow->RequestFocus();
  }
}

uint32_t IGraphicsLinux::GetUserDblClickTimeout()
{
  // Default to 400
  uint32_t timeout = 400;

  // Source: forum.kde.org/viewtopic.php?f=289&t=153755
  // Read $HOME/.gtkrc-2.0; Var: gtk-double-click-time
  // Read $HOME/.config/gtk-3.0/settings.ini ; Var: gtk-double-click-time
  // Read $HOME/.config/kdeglobals, [KDE] section, Var: DoubleClickInterval

  // Read $HOME/.config/qt5ct/qt5ct.conf ; Var: double_click_interval=400
  // Best option: DBus config path "/org/gnome/desktop/peripherals/mouse/double-click"
  //   but I'd have to learn DBus first
  return timeout;
}

void IGraphicsLinux::AddUiTask(std::function<void()>&& task)
{
  mUiTasks.Add(std::move(task));
}

void IGraphicsLinux::UpdateUI()
{
  // allow the platform to process things
  gPlatform->ProcessEvents();

  // Run all tasks that have to happen on this thread.
  mUiTasks.Process();

  if (WindowIsOpen())
  {
    uint64_t timeStart = GetTimeMs();
    LoopEvents();

    if (timeStart >= mNextDrawTime) {
      mShouldPaint = true;
    }

    IRECTList rects;
    if (mShouldPaint && IsDirty(rects))
    {
      Paint();
      SetAllControlsClean();

      // set next draw time
      // TODO use more precision (e.g. struct timespec or double?) to, hopefully, get more precision
      mNextDrawTime = GetTimeMs() + (uint64_t)(1000.0 / (double)FPS());
      // clear this so we're not rendering constantly
      mShouldPaint = false;
    }
  }
}


//TODO: move these
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

IFontDataPtr IGraphicsLinux::Font::GetFontData()
{
  if (mFontData.GetSize() == 0)
  {
    IFontDataPtr pData;
    int file = open(mFileName.Get(), O_RDONLY);
    if (file >= 0)
    {
      struct stat sb;
      if (fstat(file, &sb) == 0)
      {
        int fontSize = static_cast<int>(sb.st_size);
        void* pFontMem = mmap(NULL, fontSize, PROT_READ, MAP_PRIVATE | MAP_POPULATE, file, 0);
        if (pFontMem != MAP_FAILED)
        {
          pData = std::make_unique<IFontData>(pFontMem, fontSize, 0);
          munmap(pFontMem, fontSize);
        }
      }
      close(file);
    }
    return pData;
  }
  else
  {
    return std::make_unique<IFontData>((const void*)mFontData.Get(), mFontData.GetSize(), 0);
  }
}

PlatformFontPtr IGraphicsLinux::LoadPlatformFont(const char* fontID, const char* fileNameOrResID)
{
  WDL_String fullPath;
  const EResourceLocation fontLocation = LocateResource(fileNameOrResID, "ttf", fullPath, GetBundleID(), GetWinModuleHandle(), nullptr);
  if (fontLocation == kAbsolutePath)
  {
    return PlatformFontPtr(new Font(fullPath));
  }
  auto res = LoadRcResource(fileNameOrResID, "ttf");
  if (res)
  {
    return LoadPlatformFont(fontID, (void*)res->data(), res->size());
  }

  return nullptr;
}

PlatformFontPtr IGraphicsLinux::LoadPlatformFont(const char* fontID, void* pData, int dataSize)
{
  WDL_String name;
  name.Set(fontID);
  return PlatformFontPtr(new Font(name, pData, dataSize));
}

PlatformFontPtr IGraphicsLinux::LoadPlatformFont(const char* fontID, const char* fontName, ETextStyle style)
{
  WDL_String fullPath;
  const char* styleString;

  switch (style)
  {
    case ETextStyle::Bold: styleString = "bold"; break;
    case ETextStyle::Italic: styleString = "italic"; break;
    default: styleString = "regular";
  }

  FcConfig* config = FcInitLoadConfigAndFonts(); // TODO: init/fini for plug-in lifetime
  FcPattern* pat = FcPatternBuild(nullptr, FC_FAMILY, FcTypeString, fontName, FC_STYLE, FcTypeString, styleString, nullptr);
  FcConfigSubstitute(config, pat, FcMatchPattern);
  FcResult result;
  FcPattern* font = FcFontMatch(config, pat, &result);

  if (font)
  {
    FcChar8* file;
    if (FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch)
    {
      fullPath.Set((const char*) file);
    }
    FcPatternDestroy(font);
  }

  FcPatternDestroy(pat);
  FcConfigDestroy(config);

  return PlatformFontPtr(fullPath.Get()[0] ? new Font(fullPath) : nullptr);
}

IGraphicsLinux::IGraphicsLinux(IGEditorDelegate& dlg, int w, int h, int fps, float scale)
  : IGRAPHICS_DRAW_CLASS(dlg, w, h, fps, scale)
{
  gPlatformLock.Enter();
  if (!gPlatform)
  {
    gPlatform = PlatformX11::Create();
  }
  gPlatformLock.Leave();

  mNextDrawTime = 0;
  mCursorLock = false;
}

IGraphicsLinux::~IGraphicsLinux()
{
  CloseWindow();
}

#if !defined(NO_IGRAPHICS)
  #if defined IGRAPHICS_SKIA
    #include "Drawing/IGraphicsSkia.cpp"
  #elif defined IGRAPHICS_NANOVG
    #include "Drawing/IGraphicsNanoVG.cpp"
    #ifdef IGRAPHICS_FREETYPE
      #define FONS_USE_FREETYPE
    #endif
      #include "nanovg.c"
  #else
    #error
  #endif
#endif
