#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#include <alsa/asoundlib.h>
#include <dlfcn.h>
#include <pthread.h>
#include <unistd.h>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>

static bool TestMode(const char* mode)
{
  const char* value = getenv("IPLUG_LINUX_TEST_MODE");
  return value && strcmp(value, mode) == 0;
}

extern "C" int pthread_create(pthread_t* thread, const pthread_attr_t* attributes,
                              void* (*routine)(void*), void* argument)
{
  using Create = int (*)(pthread_t*, const pthread_attr_t*, void* (*)(void*), void*);
  static auto create = reinterpret_cast<Create>(dlsym(RTLD_NEXT, "pthread_create"));
  if (!create)
  {
    fprintf(stderr, "Linux regression hook could not resolve pthread_create\n");
    return EAGAIN;
  }
  if (TestMode("clipboard-thread-fail"))
  {
    fprintf(stderr, "Injected clipboard worker creation failure\n");
    return EAGAIN;
  }

  Dl_info info{};
  const bool graphicsTimer = dladdr(reinterpret_cast<void*>(routine), &info) &&
    info.dli_sname && strstr(info.dli_sname, "TimerThreadProc");
  const bool coreTimer = info.dli_sname && strstr(info.dli_sname, "Timer_impl") &&
    strstr(info.dli_sname, "ThreadProc");
  if (coreTimer && TestMode("core-timer-fail"))
  {
    fprintf(stderr, "Injected core timer creation failure\n");
    return EAGAIN;
  }
  if (graphicsTimer)
  {
    if (TestMode("main-loop") || TestMode("ui-thread") || TestMode("callback-close"))
    {
      fprintf(stderr, "Unexpected graphics worker creation\n");
      return EAGAIN;
    }
    snd_lib_error(__FILE__, __LINE__, __func__, 0, "Linux ALSA diagnostic regression");
    if (TestMode("timer-fail"))
    {
      fprintf(stderr, "Injected graphics timer creation failure\n");
      return EAGAIN;
    }
  }

  const int result = create(thread, attributes, routine, argument);
  if (coreTimer && result == 0 && TestMode("core-timer-early"))
  {
    usleep(100000);
    fprintf(stderr, "Injected early core timer scheduling\n");
  }
  if (graphicsTimer && result == 0 && TestMode("timer-early"))
  {
    // Give the child time to read its running flag before pthread_create returns.
    usleep(100000);
    fprintf(stderr, "Injected early graphics timer scheduling\n");
  }
  return result;
}

extern "C" Display* XOpenDisplay(const char* name)
{
  using Open = Display* (*)(const char*);
  static auto open = reinterpret_cast<Open>(dlsym(RTLD_NEXT, "XOpenDisplay"));
  if (!open)
  {
    fprintf(stderr, "Linux regression hook could not resolve XOpenDisplay\n");
    return nullptr;
  }
  if (TestMode("clipboard-display-fail"))
  {
    fprintf(stderr, "Injected clipboard display connection failure\n");
    return nullptr;
  }

  Dl_info info{};
  if (TestMode("window-fail") && dladdr(__builtin_return_address(0), &info) &&
      info.dli_sname && strstr(info.dli_sname, "IGraphicsLinux10OpenWindow"))
  {
    fprintf(stderr, "Injected graphics display connection failure\n");
    return nullptr;
  }
  return open(name);
}

extern "C" xcb_connection_t* XGetXCBConnection(Display* display)
{
  using GetConnection = xcb_connection_t* (*)(Display*);
  // Plugin dependencies may be in an RTLD_LOCAL scope, outside this preload hook's RTLD_NEXT.
  static void* library = dlopen("libX11-xcb.so.1", RTLD_NOW | RTLD_LOCAL);
  static auto getConnection = library ?
    reinterpret_cast<GetConnection>(dlsym(library, "XGetXCBConnection")) : nullptr;
  if (!getConnection)
  {
    fprintf(stderr, "Linux regression hook could not resolve XGetXCBConnection\n");
    return nullptr;
  }
  if (TestMode("clipboard-connection-fail"))
  {
    fprintf(stderr, "Injected clipboard XCB connection failure\n");
    return nullptr;
  }
  return getConnection(display);
}
