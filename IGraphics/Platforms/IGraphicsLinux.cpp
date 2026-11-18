/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <mutex>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <dlfcn.h>
#include <poll.h>
#include <cerrno>
#include <climits>
#include <limits>
#include <chrono>
#include <string>
// Fontconfig is loaded via dlopen so the plugin works without it installed.
// Only the font-by-name lookup (LoadPlatformFont) needs it; embedded fonts work regardless.

// X11 headers first — they define macros (None, Bool, True, False, Status, Complex...)
// that conflict with C++ identifiers. IGraphicsLinux.h undefines them.
#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>

// glad must come before GL/glx.h: glad sets __gl_h_ so that glx.h's own
// #include <GL/gl.h> is skipped. If glx.h included gl.h first, glad would
// detect __gl_h_ and throw a compile-time error.
// Note: IGRAPHICS_GL is defined inside IGraphics_select.h (not yet processed here),
// so we use the CMake-passed IGRAPHICS_GL2/GL3 defines directly.
#if defined(IGRAPHICS_GL2) || defined(IGRAPHICS_GL3)
  #include <glad/glad.h>
  // After glad, Bool is still defined (= int from X11/Xlib.h) so GLX
  // function declarations that use Bool compile correctly.
  #include <GL/glx.h>
#endif

// IGraphicsLinux.h undefs the conflicting X11 macros (None, Bool, etc.) and
// then includes IGraphics_select.h. The glad and GLX_H guards prevent those
// headers from being re-included.
#include "IGraphicsLinux.h"
#include "IGraphicsStructs.h"
#include "IControl.h"
#include "wdlutf8.h"

#if defined APP_API || defined REAPER_EXT_API
  #include "IPlugSWELLTimer.h"
#endif

using namespace iplug;
using namespace igraphics;

// Timer interval in microseconds (60 fps = ~16667 us)
static constexpr int TIMER_INTERVAL_US = 16667;

namespace
{
// Match WindowWidth()/WindowHeight() followed by the X11/display-scale conversion.
double PhysicalWindowDimension(int designDimension, float drawScale, float screenScale)
{
  // Probe large candidates without overflowing int; actual window sizes fit in int.
  const float logical = std::trunc(designDimension * drawScale);
  return std::trunc(logical * screenScale);
}

unsigned long HoverMillis() { return static_cast<unsigned long>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()); }

std::vector<std::string> ParseDroppedFiles(const unsigned char* data, size_t length)
{
  std::vector<std::string> paths;
  const std::string list(reinterpret_cast<const char*>(data), length);
  for (size_t start = 0; start < list.size();)
  {
    const size_t end = list.find('\n', start);
    const std::string line = list.substr(start, end == std::string::npos ? end : end - start);
    start = end == std::string::npos ? list.size() : end + 1;
    if (line.empty() || line[0] == '#')
      continue;
    const std::string uri = line.back() == '\r' ? line.substr(0, line.size() - 1) : line;
    if (uri.compare(0, 7, "file://") != 0)
      continue;
    size_t pathStart = 7;
    if (uri.compare(pathStart, 10, "localhost/") == 0)
      pathStart += 9;
    if (pathStart >= uri.size() || uri[pathStart] != '/')
      continue;
    std::string path;
    bool valid = true;
    for (size_t i = pathStart; i < uri.size(); ++i)
    {
      if (uri[i] != '%')
      {
        path += uri[i];
        continue;
      }
      if (i + 2 >= uri.size())
      {
        valid = false;
        break;
      }
      auto hex = [](char c) -> int {
        if (c >= '0' && c <= '9')
          return c - '0';
        if (c >= 'a' && c <= 'f')
          return c - 'a' + 10;
        if (c >= 'A' && c <= 'F')
          return c - 'A' + 10;
        return -1;
      };
      const int hi = hex(uri[i + 1]), lo = hex(uri[i + 2]);
      if (hi < 0 || lo < 0 || (hi == 0 && lo == 0))
      {
        valid = false;
        break;
      }
      path += static_cast<char>((hi << 4) | lo);
      i += 2;
    }
    if (valid && path.find('\0') == std::string::npos)
      paths.push_back(std::move(path));
  }
  return paths;
}

class LinuxClipboard
{
public:
  ~LinuxClipboard()
  {
    mRunning = false;
    if (mThread)
      pthread_join(mThread, nullptr);
    if (mDisplay)
    {
      XDestroyWindow(mDisplay, mWindow);
      XCloseDisplay(mDisplay);
    }
  }

  bool SetText(const char* text, const char* displayName)
  {
    std::lock_guard<std::mutex> lock(mMutex);
    if (!mDisplay)
    {
      mDisplay = XOpenDisplay(displayName);
      if (!mDisplay)
      {
        fprintf(stderr, "Could not open the X11 clipboard display\n");
        return false;
      }
      mConnection = XGetXCBConnection(mDisplay);
      if (!mConnection)
      {
        fprintf(stderr, "Could not access the XCB clipboard connection\n");
        XCloseDisplay(mDisplay);
        mDisplay = nullptr;
        return false;
      }
      mWindow = XCreateSimpleWindow(mDisplay, DefaultRootWindow(mDisplay), 0, 0, 1, 1, 0, 0, 0);
      mClipboard = XInternAtom(mDisplay, "CLIPBOARD", 0);
      mUTF8 = XInternAtom(mDisplay, "UTF8_STRING", 0);
      mTargets = XInternAtom(mDisplay, "TARGETS", 0);
      mRunning = true;
      const int error = pthread_create(&mThread, nullptr, Run, this);
      if (error)
      {
        fprintf(stderr, "Could not start the X11 clipboard service: %s\n", strerror(error));
        mRunning = false;
        mThread = 0;
        XDestroyWindow(mDisplay, mWindow);
        XCloseDisplay(mDisplay);
        mDisplay = nullptr;
        mConnection = nullptr;
        return false;
      }
    }
    if (strcmp(DisplayString(mDisplay), displayName) != 0)
    {
      fprintf(stderr, "The clipboard service is already connected to another X11 display\n");
      return false;
    }
    mText.Set(text);
    XSetSelectionOwner(mDisplay, mClipboard, mWindow, 0);
    XSync(mDisplay, 0);
    return XGetSelectionOwner(mDisplay, mClipboard) == mWindow;
  }

  bool GetOwnedText(Window owner, const char* displayName, WDL_String& text)
  {
    std::lock_guard<std::mutex> lock(mMutex);
    if (!mDisplay || owner != mWindow || strcmp(DisplayString(mDisplay), displayName) != 0)
      return false;
    text.Set(mText.Get());
    return true;
  }

private:
  static void* Run(void* context)
  {
    auto* self = static_cast<LinuxClipboard*>(context);
    while (self->mRunning)
    {
      {
        std::lock_guard<std::mutex> lock(self->mMutex);
        while (XPending(self->mDisplay))
        {
          XEvent event;
          XNextEvent(self->mDisplay, &event);
          if (event.type == SelectionRequest)
            self->Reply(event.xselectionrequest);
          else if (event.type == SelectionClear && XGetSelectionOwner(self->mDisplay, self->mClipboard) != self->mWindow)
            self->mText.Set("");
        }
      }
      pollfd fd{XConnectionNumber(self->mDisplay), POLLIN, 0};
      const int result = poll(&fd, 1, 20);
      if (result < 0 && errno != EINTR)
      {
        fprintf(stderr, "Could not poll the X11 clipboard display: %s\n", strerror(errno));
        break;
      }
    }
    return nullptr;
  }

  void Reply(const XSelectionRequestEvent& request)
  {
    XFlush(mDisplay);
    xcb_selection_notify_event_t response{};
    response.response_type = XCB_SELECTION_NOTIFY;
    response.requestor = request.requestor;
    response.selection = request.selection;
    response.target = request.target;
    response.time = request.time;
    const Atom property = request.property ? request.property : request.target;
    if (request.target == mTargets)
    {
      const xcb_atom_t targets[] = {static_cast<xcb_atom_t>(mTargets), static_cast<xcb_atom_t>(mUTF8), XCB_ATOM_STRING};
      if (!CheckReply(xcb_change_property_checked(mConnection, XCB_PROP_MODE_REPLACE, request.requestor, property, XCB_ATOM_ATOM, 32, 3, targets), "property write"))
        return;
      response.property = property;
    }
    else if (request.target == mUTF8 || request.target == XA_STRING)
    {
      if (!CheckReply(xcb_change_property_checked(mConnection, XCB_PROP_MODE_REPLACE, request.requestor, property, request.target, 8, mText.GetLength(), mText.Get()), "property write"))
        return;
      response.property = property;
    }
    char event[32]{};
    static_assert(sizeof(response) <= sizeof(event), "X11 selection event must fit the wire event");
    memcpy(event, &response, sizeof(response));
    CheckReply(xcb_send_event_checked(mConnection, 0, request.requestor, XCB_EVENT_MASK_NO_EVENT, event), "selection notification");
  }

  bool CheckReply(xcb_void_cookie_t cookie, const char* operation)
  {
    xcb_generic_error_t* error = xcb_request_check(mConnection, cookie);
    if (error)
    {
      fprintf(stderr, "X11 clipboard %s failed (error %u, resource 0x%x)\n", operation, error->error_code, error->resource_id);
      free(error);
      return false;
    }
    if (xcb_connection_has_error(mConnection))
    {
      fprintf(stderr, "The XCB clipboard connection failed during %s\n", operation);
      return false;
    }
    return true;
  }

  Display* mDisplay = nullptr;
  xcb_connection_t* mConnection = nullptr;
  Window mWindow = 0;
  Atom mClipboard = 0;
  Atom mUTF8 = 0;
  Atom mTargets = 0;
  WDL_String mText;
  std::mutex mMutex;
  std::atomic<bool> mRunning{false};
  pthread_t mThread = 0;
};

static LinuxClipboard& GetLinuxClipboard()
{
  // A separate connection serves selections even while the host UI thread waits for a paste.
  static LinuxClipboard clipboard;
  return clipboard;
}
} // namespace

// ---- Font helpers -----------------------------------------------------------

/** Loads a TTF font from a file path; GetFontData() reads the bytes on demand. */
class LinuxFileFont final : public PlatformFont
{
public:
  LinuxFileFont(const char* filePath, bool isSystem)
    : PlatformFont(isSystem)
  {
    mPath.Set(filePath);
  }

  IFontDataPtr GetFontData() override
  {
    IFontDataPtr fontData(new IFontData());
    FILE* fp = fopen(mPath.Get(), "rb");
    if (!fp)
      return fontData;

    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    if (fileSize <= 0)
    {
      fclose(fp);
      return fontData;
    }
    fontData = std::make_unique<IFontData>((int)fileSize);

    if (!fontData->GetSize())
    {
      fclose(fp);
      return fontData;
    }

    fseek(fp, 0, SEEK_SET);
    size_t readSize = fread(fontData->Get(), 1, fontData->GetSize(), fp);
    fclose(fp);

    if (readSize && readSize == (size_t)fontData->GetSize())
      fontData->SetFaceIdx(0);

    return fontData;
  }

private:
  WDL_String mPath;
};

/** Wraps pre-loaded raw font bytes; takes a copy so the caller can free its buffer. */
class LinuxMemFont final : public PlatformFont
{
public:
  LinuxMemFont(const void* pData, int dataSize)
    : PlatformFont(false)
  {
    mData.Set((const unsigned char*)pData, dataSize);
  }

  IFontDataPtr GetFontData() override { return IFontDataPtr(new IFontData(mData.Get(), mData.GetSize(), 0)); }

private:
  WDL_TypedBuf<unsigned char> mData;
};

