#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <thread>
#include "X11Test.h"

using Clock = std::chrono::steady_clock;

static Window FindWindow(Display* display, Window parent)
{
  char* name = nullptr;
  XFetchName(display, parent, &name);
  const bool found = name && strcmp(name, "IPlugEffect") == 0;
  if (name)
    XFree(name);
  XWindowAttributes attributes{};
  if (found && XGetWindowAttributes(display, parent, &attributes) && attributes.width > 100)
    return parent;

  Window root, owner, *children = nullptr;
  unsigned count = 0;
  if (!XQueryTree(display, parent, &root, &owner, &children, &count))
    return 0;
  Window result = 0;
  for (unsigned i = 0; i < count && !result; ++i)
    result = FindWindow(display, children[i]);
  if (children)
    XFree(children);
  return result;
}

static void CloseWindow(Display* display, Window window)
{
  XEvent event{};
  event.xclient.type = ClientMessage;
  event.xclient.window = window;
  event.xclient.message_type = XInternAtom(display, "WM_PROTOCOLS", False);
  event.xclient.format = 32;
  event.xclient.data.l[0] = XInternAtom(display, "WM_DELETE_WINDOW", False);
  event.xclient.data.l[1] = CurrentTime;
  XSendEvent(display, window, False, NoEventMask, &event);
  XFlush(display);
}

static bool Run(Display* display, const char* app, const char* hooks, const char* mode)
{
  char temporary[] = "/tmp/iplug-linux-test-XXXXXX";
  const char* directory = mkdtemp(temporary);
  if (!directory)
  {
    perror("mkdtemp");
    return false;
  }
  const std::string logPath = std::string(directory) + "/startup.log";
  const pid_t child = fork();
  if (child == 0)
  {
    const int log = open(logPath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (log < 0 || dup2(log, STDERR_FILENO) < 0 || dup2(log, STDOUT_FILENO) < 0)
      _exit(127);
    close(log);
    const char* existing = getenv("LD_PRELOAD");
    const std::string preload = std::string(hooks) + (existing ? ":" + std::string(existing) : "");
    if (setenv("XDG_CONFIG_HOME", directory, 1) ||
        setenv("LD_PRELOAD", preload.c_str(), 1) ||
        setenv("IPLUG_LINUX_TEST_MODE", strcmp(mode, "close-fail") == 0 ? "timer-fail" : mode, 1))
      _exit(127);
    execl(app, app, "--no-io", nullptr);
    _exit(127);
  }
  if (child < 0)
  {
    perror("fork");
    std::filesystem::remove_all(directory);
    return false;
  }

  const bool expectFailure = strcmp(mode, "timer-fail") == 0 || strcmp(mode, "window-fail") == 0 ||
    strcmp(mode, "close-fail") == 0;
  bool rendered = false;
  bool closeSent = false;
  bool exited = false;
  int status = 0;
  const auto deadline = Clock::now() + std::chrono::seconds(8);
  while (Clock::now() < deadline)
  {
    if (waitpid(child, &status, WNOHANG) == child)
    {
      exited = true;
      break;
    }
    if (!expectFailure && !closeSent)
    {
      const bool idleProbe = strcmp(mode, "idle") == 0 || strcmp(mode, "close-order") == 0;
      const Window window = FindWindow(display, DefaultRootWindow(display));
      const Window editor = window ? FindEditor(display, window) : 0;
      if (editor && IsRendered(display, editor))
      {
        rendered = true;
        if (strcmp(mode, "ui-thread") == 0 || strcmp(mode, "callback-close") == 0)
        {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
          XEvent event {};
          event.xbutton.type = ButtonPress;
          event.xbutton.display = display;
          event.xbutton.window = editor;
          event.xbutton.button = Button1;
          event.xbutton.x = 250;
          event.xbutton.y = 250;
          event.xbutton.time = 1000;
          XSendEvent(display, editor, False, ButtonPressMask, &event);
          event.xbutton.type = ButtonRelease;
          XSendEvent(display, editor, False, ButtonReleaseMask, &event);
          XSync(display, False);
          std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
        if (idleProbe)
          std::this_thread::sleep_for(std::chrono::milliseconds(150));
        if (strcmp(mode, "close-order") == 0)
        {
          SendNativeClose(display, editor);
          std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        if (strcmp(mode, "ui-thread") != 0)
          CloseWindow(display, window);
        closeSent = true;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
  if (!exited)
  {
    fprintf(stderr, "%s: application did not finish within eight seconds\n", mode);
    kill(child, SIGKILL);
    waitpid(child, &status, 0);
  }
  std::ifstream file(logPath);
  const std::string log((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  const bool correctExit = exited && WIFEXITED(status) &&
    WEXITSTATUS(status) == (expectFailure ? EXIT_FAILURE : EXIT_SUCCESS);
  bool passed = correctExit && (expectFailure || rendered);
  if (expectFailure)
    passed &= log.find("Could not attach the standalone editor") != std::string::npos;
  if (strcmp(mode, "window-fail") == 0)
    passed &= log.find("Injected graphics display connection failure") != std::string::npos;
  else
    passed &= log.find("Linux ALSA diagnostic regression") != std::string::npos;
  if (strcmp(mode, "timer-fail") == 0)
    passed &= log.find("Could not start Linux graphics main-loop timer") != std::string::npos;
  passed &= log.find("Unexpected graphics worker creation") == std::string::npos;
  if (strcmp(mode, "idle") == 0)
  {
    passed &= log.find("Linux idle probe: main=1") != std::string::npos;
    passed &= log.find("Linux idle message: main=1") != std::string::npos;
    passed &= log.find("wrong-thread=0") != std::string::npos;
  }
  if (strcmp(mode, "ui-thread") == 0 || strcmp(mode, "callback-close") == 0)
  {
    passed &= log.find("Linux UI callback: main=1 alive=1") != std::string::npos;
    passed &= log.find("Linux UI draw: main=1") != std::string::npos;
  }
  if (strcmp(mode, "ui-thread") == 0)
    passed &= log.find("Linux UI dialog: main=1 accepted=1") != std::string::npos;
  if (strcmp(mode, "idle") == 0 || strcmp(mode, "close-order") == 0 || strcmp(mode, "close-fail") == 0 ||
      strcmp(mode, "ui-thread") == 0 || strcmp(mode, "callback-close") == 0)
    passed &= log.find("Linux close probe: opens=1 closes=1 errors=0") != std::string::npos;
  printf("%s: %s\n%s", mode, passed ? "PASS" : "FAIL", log.c_str());
  if (!passed)
    fprintf(stderr, "exited=%d status=%d rendered=%d\n", exited, status, rendered);
  file.close();
  std::filesystem::remove_all(directory);
  return passed;
}

int main(int argc, char** argv)
{
  if (argc != 4)
  {
    fprintf(stderr, "Usage: LinuxGuiTest app startup-hooks mode\n");
    return EXIT_FAILURE;
  }
  Display* display = XOpenDisplay(nullptr);
  if (!display)
  {
    fprintf(stderr, "No X11 display available for GUI regression tests\n");
    return EXIT_FAILURE;
  }
  bool passed = true;
  const int repetitions = strcmp(argv[3], "normal") == 0 ? 5 : 1;
  for (int i = 0; i < repetitions && passed; ++i)
    passed = Run(display, argv[1], argv[2], argv[3]);
  XCloseDisplay(display);
  return passed ? EXIT_SUCCESS : EXIT_FAILURE;
}
