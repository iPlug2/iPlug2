/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include <stddef.h>
#include <wdlutf8.h>

#include "IPlugParameter.h"
#include "IGraphicsLinux.h"
#include "IPlugPaths.h"
#include <unistd.h>
#include <sys/wait.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xfixes.h>

#ifdef OS_LINUX
  #ifdef IGRAPHICS_GL
    #include "glad.c"
    #include <glad/glad_glx.h>
  #endif
  #include <fontconfig/fontconfig.h>
#endif

#define NOT_IMPLEMENTED printf("%s: not implemented\n", __FUNCTION__);

using namespace iplug;
using namespace igraphics;

#define IPLUG_TIMER_ID 2

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

  void* ctx = xcbt_window_draw_begin(mPlugWnd);

  if (ctx)
  {
    Draw(rects);
    xcbt_window_draw_end(mPlugWnd);
  }
}

void IGraphicsLinux::DrawResize()
{
  void* ctx = xcbt_window_draw_begin(mPlugWnd);
  
  if (ctx)
  {
    IGRAPHICS_DRAW_CLASS::DrawResize();
    xcbt_window_draw_stop(mPlugWnd); // WARNING: in CAN BE reentrant!!! (f.e. it is called from SetScreenScale during initialization)
  }
  // WARNING: IPlug call it on resize, but at the end. When should we call Paint() ?
  // In Windows version "Update window" is called from PlatformResize, so BEFORE DrawResize...
}

inline IMouseInfo IGraphicsLinux::GetMouseInfo(int16_t x, int16_t y, int16_t state)
{
  IMouseInfo info;
  info.x = mCursorX = x / (GetDrawScale() * GetScreenScale());
  info.y = mCursorY = y / (GetDrawScale() * GetScreenScale());
  info.ms = IMouseMod((state & XCB_BUTTON_MASK_1), (state & XCB_BUTTON_MASK_3), // Note "2" is the middle button 
    (state & XCB_KEY_BUT_MASK_SHIFT), (state & XCB_KEY_BUT_MASK_CONTROL), (state & XCB_KEY_BUT_MASK_MOD_1) // shift, ctrl, alt
  );

  return info;
}

inline IMouseInfo IGraphicsLinux::GetMouseInfoDeltas(float& dX, float& dY, int16_t x, int16_t y, int16_t state)
{
  float oldX = mCursorX;
  float oldY = mCursorY;
  
  IMouseInfo info = GetMouseInfo(x, y, state);
  
  dX = info.x - oldX;
  dY = info.y - oldY;
  // dX = oldX - info.x;
  // dY = oldY - info.y;
  
  return info;
}

void IGraphicsLinux::TimerHandler(int timerID)
{
  if (timerID == IPLUG_TIMER_ID)
  {
    IRECTList rects;
    if (IsDirty(rects))
    {
      Paint();
      SetAllControlsClean();
    }
    xcbt_timer_set(mX, IPLUG_TIMER_ID, 6, (xcbt_timer_cb) TimerHandlerProxy, this);
  }
}

enum KeyboardCode {
  VKEY_CANCEL = 0x03,
  VKEY_BACK = 0x08,
  VKEY_TAB = 0x09,
  VKEY_BACKTAB = 0x0A,
  VKEY_CLEAR = 0x0C,
  VKEY_RETURN = 0x0D,
  VKEY_SHIFT = 0x10,
  VKEY_CONTROL = 0x11,
  VKEY_MENU = 0x12,
  VKEY_PAUSE = 0x13,
  VKEY_CAPITAL = 0x14,
  VKEY_KANA = 0x15,
  VKEY_HANGUL = 0x15,
  VKEY_JUNJA = 0x17,
  VKEY_FINAL = 0x18,
  VKEY_HANJA = 0x19,
  VKEY_KANJI = 0x19,
  VKEY_ESCAPE = 0x1B,
  VKEY_CONVERT = 0x1C,
  VKEY_NONCONVERT = 0x1D,
  VKEY_ACCEPT = 0x1E,
  VKEY_MODECHANGE = 0x1F,
  VKEY_SPACE = 0x20,
  VKEY_PRIOR = 0x21,
  VKEY_NEXT = 0x22,
  VKEY_END = 0x23,
  VKEY_HOME = 0x24,
  VKEY_LEFT = 0x25,
  VKEY_UP = 0x26,
  VKEY_RIGHT = 0x27,
  VKEY_DOWN = 0x28,
  VKEY_SELECT = 0x29,
  VKEY_PRINT = 0x2A,
  VKEY_EXECUTE = 0x2B,
  VKEY_SNAPSHOT = 0x2C,
  VKEY_INSERT = 0x2D,
  VKEY_DELETE = 0x2E,
  VKEY_HELP = 0x2F,
  VKEY_0 = 0x30,
  VKEY_1 = 0x31,
  VKEY_2 = 0x32,
  VKEY_3 = 0x33,
  VKEY_4 = 0x34,
  VKEY_5 = 0x35,
  VKEY_6 = 0x36,
  VKEY_7 = 0x37,
  VKEY_8 = 0x38,
  VKEY_9 = 0x39,
  VKEY_A = 0x41,
  VKEY_B = 0x42,
  VKEY_C = 0x43,
  VKEY_D = 0x44,
  VKEY_E = 0x45,
  VKEY_F = 0x46,
  VKEY_G = 0x47,
  VKEY_H = 0x48,
  VKEY_I = 0x49,
  VKEY_J = 0x4A,
  VKEY_K = 0x4B,
  VKEY_L = 0x4C,
  VKEY_M = 0x4D,
  VKEY_N = 0x4E,
  VKEY_O = 0x4F,
  VKEY_P = 0x50,
  VKEY_Q = 0x51,
  VKEY_R = 0x52,
  VKEY_S = 0x53,
  VKEY_T = 0x54,
  VKEY_U = 0x55,
  VKEY_V = 0x56,
  VKEY_W = 0x57,
  VKEY_X = 0x58,
  VKEY_Y = 0x59,
  VKEY_Z = 0x5A,
  VKEY_LWIN = 0x5B,
  VKEY_COMMAND = VKEY_LWIN,  // Provide the Mac name for convenience.
  VKEY_RWIN = 0x5C,
  VKEY_APPS = 0x5D,
  VKEY_SLEEP = 0x5F,
  VKEY_NUMPAD0 = 0x60,
  VKEY_NUMPAD1 = 0x61,
  VKEY_NUMPAD2 = 0x62,
  VKEY_NUMPAD3 = 0x63,
  VKEY_NUMPAD4 = 0x64,
  VKEY_NUMPAD5 = 0x65,
  VKEY_NUMPAD6 = 0x66,
  VKEY_NUMPAD7 = 0x67,
  VKEY_NUMPAD8 = 0x68,
  VKEY_NUMPAD9 = 0x69,
  VKEY_MULTIPLY = 0x6A,
  VKEY_ADD = 0x6B,
  VKEY_SEPARATOR = 0x6C,
  VKEY_SUBTRACT = 0x6D,
  VKEY_DECIMAL = 0x6E,
  VKEY_DIVIDE = 0x6F,
  VKEY_F1 = 0x70,
  VKEY_F2 = 0x71,
  VKEY_F3 = 0x72,
  VKEY_F4 = 0x73,
  VKEY_F5 = 0x74,
  VKEY_F6 = 0x75,
  VKEY_F7 = 0x76,
  VKEY_F8 = 0x77,
  VKEY_F9 = 0x78,
  VKEY_F10 = 0x79,
  VKEY_F11 = 0x7A,
  VKEY_F12 = 0x7B,
  VKEY_F13 = 0x7C,
  VKEY_F14 = 0x7D,
  VKEY_F15 = 0x7E,
  VKEY_F16 = 0x7F,
  VKEY_F17 = 0x80,
  VKEY_F18 = 0x81,
  VKEY_F19 = 0x82,
  VKEY_F20 = 0x83,
  VKEY_F21 = 0x84,
  VKEY_F22 = 0x85,
  VKEY_F23 = 0x86,
  VKEY_F24 = 0x87,
  VKEY_NUMLOCK = 0x90,
  VKEY_SCROLL = 0x91,
  VKEY_LSHIFT = 0xA0,
  VKEY_RSHIFT = 0xA1,
  VKEY_LCONTROL = 0xA2,
  VKEY_RCONTROL = 0xA3,
  VKEY_LMENU = 0xA4,
  VKEY_RMENU = 0xA5,
  VKEY_BROWSER_BACK = 0xA6,
  VKEY_BROWSER_FORWARD = 0xA7,
  VKEY_BROWSER_REFRESH = 0xA8,
  VKEY_BROWSER_STOP = 0xA9,
  VKEY_BROWSER_SEARCH = 0xAA,
  VKEY_BROWSER_FAVORITES = 0xAB,
  VKEY_BROWSER_HOME = 0xAC,
  VKEY_VOLUME_MUTE = 0xAD,
  VKEY_VOLUME_DOWN = 0xAE,
  VKEY_VOLUME_UP = 0xAF,
  VKEY_MEDIA_NEXT_TRACK = 0xB0,
  VKEY_MEDIA_PREV_TRACK = 0xB1,
  VKEY_MEDIA_STOP = 0xB2,
  VKEY_MEDIA_PLAY_PAUSE = 0xB3,
  VKEY_MEDIA_LAUNCH_MAIL = 0xB4,
  VKEY_MEDIA_LAUNCH_MEDIA_SELECT = 0xB5,
  VKEY_MEDIA_LAUNCH_APP1 = 0xB6,
  VKEY_MEDIA_LAUNCH_APP2 = 0xB7,
  VKEY_OEM_1 = 0xBA,
  VKEY_OEM_PLUS = 0xBB,
  VKEY_OEM_COMMA = 0xBC,
  VKEY_OEM_MINUS = 0xBD,
  VKEY_OEM_PERIOD = 0xBE,
  VKEY_OEM_2 = 0xBF,
  VKEY_OEM_3 = 0xC0,
  VKEY_OEM_4 = 0xDB,
  VKEY_OEM_5 = 0xDC,
  VKEY_OEM_6 = 0xDD,
  VKEY_OEM_7 = 0xDE,
  VKEY_OEM_8 = 0xDF,
  VKEY_OEM_102 = 0xE2,
  VKEY_OEM_103 = 0xE3,  // GTV KEYCODE_MEDIA_REWIND
  VKEY_OEM_104 = 0xE4,  // GTV KEYCODE_MEDIA_FAST_FORWARD
  VKEY_PROCESSKEY = 0xE5,
  VKEY_PACKET = 0xE7,
  VKEY_OEM_ATTN = 0xF0,      // JIS DomKey::ALPHANUMERIC
  VKEY_OEM_FINISH = 0xF1,    // JIS DomKey::KATAKANA
  VKEY_OEM_COPY = 0xF2,      // JIS DomKey::HIRAGANA
  VKEY_DBE_SBCSCHAR = 0xF3,  // JIS DomKey::HANKAKU
  VKEY_DBE_DBCSCHAR = 0xF4,  // JIS DomKey::ZENKAKU
  VKEY_OEM_BACKTAB = 0xF5,   // JIS DomKey::ROMAJI
  VKEY_ATTN = 0xF6,          // DomKey::ATTN or JIS DomKey::KANA_MODE
  VKEY_CRSEL = 0xF7,
  VKEY_EXSEL = 0xF8,
  VKEY_EREOF = 0xF9,
  VKEY_PLAY = 0xFA,
  VKEY_ZOOM = 0xFB,
  VKEY_NONAME = 0xFC,
  VKEY_PA1 = 0xFD,
  VKEY_OEM_CLEAR = 0xFE,
  VKEY_UNKNOWN = 0,
  // POSIX specific VKEYs. Note that as of Windows SDK 7.1, 0x97-9F, 0xD8-DA,
  // and 0xE8 are unassigned.
  VKEY_WLAN = 0x97,
  VKEY_POWER = 0x98,
  VKEY_BRIGHTNESS_DOWN = 0xD8,
  VKEY_BRIGHTNESS_UP = 0xD9,
  VKEY_KBD_BRIGHTNESS_DOWN = 0xDA,
  VKEY_KBD_BRIGHTNESS_UP = 0xE8,
  // Windows does not have a specific key code for AltGr. We use the unused 0xE1
  // (VK_OEM_AX) code to represent AltGr, matching the behaviour of Firefox on
  // Linux.
  VKEY_ALTGR = 0xE1,
  // Windows does not have a specific key code for Compose. We use the unused
  // 0xE6 (VK_ICO_CLEAR) code to represent Compose.
  VKEY_COMPOSE = 0xE6,
};

#define XK_3270  // for XK_3270_BackTab
#include <X11/XF86keysym.h>
#include <X11/keysym.h>
#define VKEY_UNSUPPORTED VKEY_UNKNOWN

KeyboardCode KeyboardCodeFromXKeysym(unsigned int keysym) {
  // TODO(sad): Have |keysym| go through the X map list?
  // [a-z] cases.
  if (keysym >= XK_a && keysym <= XK_z)
    return static_cast<KeyboardCode>(VKEY_A + keysym - XK_a);
  // [0-9] cases.
  if (keysym >= XK_0 && keysym <= XK_9)
    return static_cast<KeyboardCode>(VKEY_0 + keysym - XK_0);

  switch (keysym) {
    case XK_BackSpace:
      return VKEY_BACK;
    case XK_Delete:
    case XK_KP_Delete:
      return VKEY_DELETE;
    case XK_Tab:
    case XK_KP_Tab:
    case XK_ISO_Left_Tab:
    case XK_3270_BackTab:
      return VKEY_TAB;
    case XK_Linefeed:
    case XK_Return:
    case XK_KP_Enter:
    case XK_ISO_Enter:
      return VKEY_RETURN;
    case XK_Clear:
    case XK_KP_Begin:  // NumPad 5 without Num Lock, for crosbug.com/29169.
      return VKEY_CLEAR;
    case XK_KP_Space:
    case XK_space:
      return VKEY_SPACE;
    case XK_Home:
    case XK_KP_Home:
      return VKEY_HOME;
    case XK_End:
    case XK_KP_End:
      return VKEY_END;
    case XK_Page_Up:
    case XK_KP_Page_Up:  // aka XK_KP_Prior
      return VKEY_PRIOR;
    case XK_Page_Down:
    case XK_KP_Page_Down:  // aka XK_KP_Next
      return VKEY_NEXT;
    case XK_Left:
    case XK_KP_Left:
      return VKEY_LEFT;
    case XK_Right:
    case XK_KP_Right:
      return VKEY_RIGHT;
    case XK_Down:
    case XK_KP_Down:
      return VKEY_DOWN;
    case XK_Up:
    case XK_KP_Up:
      return VKEY_UP;
    case XK_Escape:
      return VKEY_ESCAPE;
    case XK_Kana_Lock:
    case XK_Kana_Shift:
      return VKEY_KANA;
    case XK_Hangul:
      return VKEY_HANGUL;
    case XK_Hangul_Hanja:
      return VKEY_HANJA;
    case XK_Kanji:
      return VKEY_KANJI;
    case XK_Henkan:
      return VKEY_CONVERT;
    case XK_Muhenkan:
      return VKEY_NONCONVERT;
    case XK_Zenkaku_Hankaku:
      return VKEY_DBE_DBCSCHAR;
    case XK_KP_0:
    case XK_KP_1:
    case XK_KP_2:
    case XK_KP_3:
    case XK_KP_4:
    case XK_KP_5:
    case XK_KP_6:
    case XK_KP_7:
    case XK_KP_8:
    case XK_KP_9:
      return static_cast<KeyboardCode>(VKEY_NUMPAD0 + (keysym - XK_KP_0));
    case XK_multiply:
    case XK_KP_Multiply:
      return VKEY_MULTIPLY;
    case XK_KP_Add:
      return VKEY_ADD;
    case XK_KP_Separator:
      return VKEY_SEPARATOR;
    case XK_KP_Subtract:
      return VKEY_SUBTRACT;
    case XK_KP_Decimal:
      return VKEY_DECIMAL;
    case XK_KP_Divide:
      return VKEY_DIVIDE;
    case XK_KP_Equal:
    case XK_equal:
    case XK_plus:
      return VKEY_OEM_PLUS;
    case XK_comma:
    case XK_less:
      return VKEY_OEM_COMMA;
    case XK_minus:
    case XK_underscore:
      return VKEY_OEM_MINUS;
    case XK_greater:
    case XK_period:
      return VKEY_OEM_PERIOD;
    case XK_colon:
    case XK_semicolon:
      return VKEY_OEM_1;
    case XK_question:
    case XK_slash:
      return VKEY_OEM_2;
    case XK_asciitilde:
    case XK_quoteleft:
      return VKEY_OEM_3;
    case XK_bracketleft:
    case XK_braceleft:
      return VKEY_OEM_4;
    case XK_backslash:
    case XK_bar:
      return VKEY_OEM_5;
    case XK_bracketright:
    case XK_braceright:
      return VKEY_OEM_6;
    case XK_quoteright:
    case XK_quotedbl:
      return VKEY_OEM_7;
    case XK_ISO_Level5_Shift:
      return VKEY_OEM_8;
    case XK_Shift_L:
    case XK_Shift_R:
      return VKEY_SHIFT;
    case XK_Control_L:
    case XK_Control_R:
      return VKEY_CONTROL;
    case XK_Meta_L:
    case XK_Meta_R:
    case XK_Alt_L:
    case XK_Alt_R:
      return VKEY_MENU;
    case XK_ISO_Level3_Shift:
    case XK_Mode_switch:
      return VKEY_ALTGR;
    case XK_Multi_key:
      return VKEY_COMPOSE;
    case XK_Pause:
      return VKEY_PAUSE;
    case XK_Caps_Lock:
      return VKEY_CAPITAL;
    case XK_Num_Lock:
      return VKEY_NUMLOCK;
    case XK_Scroll_Lock:
      return VKEY_SCROLL;
    case XK_Select:
      return VKEY_SELECT;
    case XK_Print:
      return VKEY_PRINT;
    case XK_Execute:
      return VKEY_EXECUTE;
    case XK_Insert:
    case XK_KP_Insert:
      return VKEY_INSERT;
    case XK_Help:
      return VKEY_HELP;
    case XK_Super_L:
      return VKEY_LWIN;
    case XK_Super_R:
      return VKEY_RWIN;
    case XK_Menu:
      return VKEY_APPS;
    case XK_F1:
    case XK_F2:
    case XK_F3:
    case XK_F4:
    case XK_F5:
    case XK_F6:
    case XK_F7:
    case XK_F8:
    case XK_F9:
    case XK_F10:
    case XK_F11:
    case XK_F12:
    case XK_F13:
    case XK_F14:
    case XK_F15:
    case XK_F16:
    case XK_F17:
    case XK_F18:
    case XK_F19:
    case XK_F20:
    case XK_F21:
    case XK_F22:
    case XK_F23:
    case XK_F24:
      return static_cast<KeyboardCode>(VKEY_F1 + (keysym - XK_F1));
    case XK_KP_F1:
    case XK_KP_F2:
    case XK_KP_F3:
    case XK_KP_F4:
      return static_cast<KeyboardCode>(VKEY_F1 + (keysym - XK_KP_F1));
    case XK_guillemotleft:
    case XK_guillemotright:
    case XK_degree:
    // In the case of canadian multilingual keyboard layout, VKEY_OEM_102 is
    // assigned to ugrave key.
    case XK_ugrave:
    case XK_Ugrave:
    case XK_brokenbar:
      return VKEY_OEM_102;  // international backslash key in 102 keyboard.
    // When evdev is in use, /usr/share/X11/xkb/symbols/inet maps F13-18 keys
    // to the special XF86XK symbols to support Microsoft Ergonomic keyboards:
    // https://bugs.freedesktop.org/show_bug.cgi?id=5783
    // In Chrome, we map these X key symbols back to F13-18 since we don't have
    // VKEYs for these XF86XK symbols.
    case XF86XK_Launch5:
      return VKEY_F14;
    case XF86XK_Launch6:
      return VKEY_F15;
    case XF86XK_Launch7:
      return VKEY_F16;
    case XF86XK_Launch8:
      return VKEY_F17;
    case XF86XK_Launch9:
      return VKEY_F18;
    // For supporting multimedia buttons on a USB keyboard.
    case XF86XK_Back:
      return VKEY_BROWSER_BACK;
    case XF86XK_Forward:
      return VKEY_BROWSER_FORWARD;
    case XF86XK_Reload:
      return VKEY_BROWSER_REFRESH;
    case XF86XK_Stop:
      return VKEY_BROWSER_STOP;
    case XF86XK_Search:
      return VKEY_BROWSER_SEARCH;
    case XF86XK_Favorites:
      return VKEY_BROWSER_FAVORITES;
    case XF86XK_HomePage:
      return VKEY_BROWSER_HOME;
    case XF86XK_AudioMute:
      return VKEY_VOLUME_MUTE;
    case XF86XK_AudioLowerVolume:
      return VKEY_VOLUME_DOWN;
    case XF86XK_AudioRaiseVolume:
      return VKEY_VOLUME_UP;
    case XF86XK_AudioNext:
      return VKEY_MEDIA_NEXT_TRACK;
    case XF86XK_AudioPrev:
      return VKEY_MEDIA_PREV_TRACK;
    case XF86XK_AudioStop:
      return VKEY_MEDIA_STOP;
    case XF86XK_AudioPlay:
      return VKEY_MEDIA_PLAY_PAUSE;
    case XF86XK_Mail:
      return VKEY_MEDIA_LAUNCH_MAIL;
    case XF86XK_LaunchA:  // F3 on an Apple keyboard.
      return VKEY_MEDIA_LAUNCH_APP1;
    case XF86XK_LaunchB:  // F4 on an Apple keyboard.
    case XF86XK_Calculator:
      return VKEY_MEDIA_LAUNCH_APP2;
    // XF86XK_Tools is generated from HID Usage AL_CONSUMER_CONTROL_CONFIG
    // (Usage 0x0183, Page 0x0C) and most commonly launches the OS default
    // media player (see crbug.com/398345).
    case XF86XK_Tools:
      return VKEY_MEDIA_LAUNCH_MEDIA_SELECT;
    case XF86XK_WLAN:
      return VKEY_WLAN;
    case XF86XK_PowerOff:
      return VKEY_POWER;
    case XF86XK_MonBrightnessDown:
      return VKEY_BRIGHTNESS_DOWN;
    case XF86XK_MonBrightnessUp:
      return VKEY_BRIGHTNESS_UP;
    case XF86XK_KbdBrightnessDown:
      return VKEY_KBD_BRIGHTNESS_DOWN;
    case XF86XK_KbdBrightnessUp:
      return VKEY_KBD_BRIGHTNESS_UP;
   // TODO(sad): some keycodes are still missing.
  }
  return VKEY_UNKNOWN;
}

#include <X11/XKBlib.h>

static bool altPressed{}, shiftPressed{}, controlPressed{};

void IGraphicsLinux::WindowHandler(xcb_generic_event_t* evt)
{
  static struct timeval pt = {0}, ct;

  if (!evt)
  {
    mBaseWindowHandler(mPlugWnd, NULL, mBaseWindowData);
    mPlugWnd = nullptr;
  }
  else
  { 
    auto type = evt->response_type & ~0x80;
    switch(type)
    {
      case XCB_EXPOSE:
      {
        xcb_expose_event_t *ee = (xcb_expose_event_t *)evt;

        if (!ee->count) // MAYBE: can collect and use invalidated areas
        {
          Paint();
        }
      }
      break;
      case XCB_BUTTON_PRESS:
      {
        xcb_button_press_event_t* bp = (xcb_button_press_event_t*) evt;

        bool btnLeft = bp->detail == 1;
        bool btnRight = bp->detail == 3;

        if (btnLeft) // check for double-click
        { 
          if (!mLastLeftClickStamp)
          {
            mLastLeftClickStamp = bp->time;
          } 
          else
          {
            if ((bp->time - mLastLeftClickStamp) < mDblClickTimeout)
            {
              IMouseInfo info = GetMouseInfo(bp->event_x, bp->event_y, bp->state | XCB_BUTTON_MASK_1); // convert button to state mask

              if (OnMouseDblClick(info.x, info.y, info.ms))
              {
                // TODO: SetCapture(hWnd);
              }
              mLastLeftClickStamp = 0;
              xcbt_flush(mX);
              break;
            }
            mLastLeftClickStamp = bp->time;
          }
        }
        else
        {
          mLastLeftClickStamp = 0;
        }

        // TODO: end parameter editing (if in progress, and return then)
        // TODO: set focus
        xcb_set_input_focus_checked(xcbt_conn(mX), XCB_INPUT_FOCUS_POINTER_ROOT, mPlugWnd->wnd, XCB_CURRENT_TIME);

        // TODO: detect double click
        
        // TODO: set capture (or after capture...) (but check other buttons first)
        if ((bp->detail == 1) || (bp->detail == 3)) // left/right
        { 
          uint16_t state = bp->state | (0x80<<bp->detail); // merge state before with pressed button
          IMouseInfo info = GetMouseInfo(bp->event_x, bp->event_y, state); // convert button to state mask
          std::vector<IMouseInfo> list{ info };
          OnMouseDown(list);
          RequestFocus();
        } 
        else if ((bp->detail == 4) || (bp->detail == 5)) // wheel
        { 
          IMouseInfo info = GetMouseInfo(bp->event_x, bp->event_y, bp->state);
          OnMouseWheel(info.x, info.y, info.ms, bp->detail == 4 ? 1. : -1);
        }
        xcbt_flush(mX);
        break;
      }
      case XCB_BUTTON_RELEASE:
      {
        xcb_button_release_event_t* br = (xcb_button_release_event_t*) evt;
        // TODO: release capture (but check other buttons first...)
        if ((br->detail == 1) || (br->detail == 3))
        { // we do not process other buttons, at least not yet
          uint16_t state = br->state & ~(0x80<<br->detail); // merge state before with released button
          IMouseInfo info = GetMouseInfo(br->event_x, br->event_y, state); // convert button to state mask
          std::vector<IMouseInfo> list{ info };
          OnMouseUp(list);
          RequestFocus();
        }
        xcbt_flush(mX);
        break;
      }
      case XCB_MOTION_NOTIFY:
      {
        xcb_motion_notify_event_t* mn = (xcb_motion_notify_event_t*) evt;
        if (mCursorLock && (float)mn->event_x == mMouseLockPos.x && (float)mn->event_y == mMouseLockPos.y)
        {
          break;
        }

        mLastLeftClickStamp = 0;
        if (mn->same_screen && (mn->event == xcbt_window_xwnd(mPlugWnd)))
        {
          // can use event_x/y
          if (!(mn->state & (XCB_BUTTON_MASK_1 | XCB_BUTTON_MASK_3))) // Not left/right drag
          {
            IMouseInfo info = GetMouseInfo(mn->event_x, mn->event_y, mn->state);
            OnMouseOver(info.x, info.y, info.ms);
          } 
          else 
          {
            // NOTE: this also updates mCursorX and mCursorY
            float dX, dY;
            IMouseInfo info = GetMouseInfoDeltas(dX, dY, mn->event_x, mn->event_y, mn->state);

            if (dX || dY)
            {
              info.dX = dX;
              info.dY = dY;
              std::vector<IMouseInfo> list{ info };

              OnMouseDrag(list);
              if (mCursorLock && (mCursorX != mMouseLockPos.x || mCursorY != mMouseLockPos.y))
              {
                MoveMouseCursor(mMouseLockPos.x, mMouseLockPos.y);
              }
            }
          }
        }
        xcbt_flush(mX);
        break;
      }
      case XCB_PROPERTY_NOTIFY:
      {
        xcb_property_notify_event_t* pn = (xcb_property_notify_event_t*) evt;
        if (pn->atom == XCBT_XEMBED_INFO(mX))
        {
          // TODO: check we really have to, but getting XEMBED_MAPPED and compare with current mapping status
          xcbt_window_map(mPlugWnd);
        }
        break;
      }
      case XCB_ENTER_NOTIFY:
      {
        RequestFocus();
        break;
      }
      case XCB_LEAVE_NOTIFY:
      {
        OnMouseOut();
        break;
      }
      case XCB_FOCUS_IN:
      {
        break;
      }
      case XCB_FOCUS_OUT:
      {
        break;
      }
      case XCB_KEY_PRESS: 
      case XCB_KEY_RELEASE: 
      {
        auto kp = (xcb_key_press_event_t*) evt;
        XKeyEvent keyev;
        keyev.display = (Display*) xcbt_display(mX); 
        keyev.keycode = kp->detail;
        keyev.state = kp->state;
        char buf[16]{};
        KeySym keysym;
     	if (XLookupString(&keyev, buf, 16, &keysym, nullptr)) {
	  // buf[1] = '\0';
           IKeyPress keyPress{buf, KeyboardCodeFromXKeysym(keysym),
                            shiftPressed,
                            controlPressed,
                            altPressed};
	   if(type == XCB_KEY_PRESS)
               OnKeyDown(kp->event_x, kp->event_y, keyPress);
           else
               OnKeyUp(kp->event_x, kp->event_y, keyPress);
        }
        else {
	   auto x = KeyboardCodeFromXKeysym(keysym);
	   if(x == VKEY_SHIFT){
	       shiftPressed = type == XCB_KEY_PRESS;
	   }
	   else if(x == VKEY_CONTROL){
	       controlPressed = type == XCB_KEY_PRESS;
	   }
	   else if(x == VKEY_MENU){
	       altPressed = type == XCB_KEY_PRESS;
	   }
        }
        break;
      }
     default:
        break;
    }
  }
  mBaseWindowHandler(mPlugWnd, evt, mBaseWindowData);
}

void IGraphicsLinux::SetIntegration(void* mainLoop)
{
  xcbt_embed* e = static_cast<xcbt_embed*>(mainLoop);

  if (!e)
  {
    if (mEmbed)
    {
      if (mX)
      {
        // DBGMSG("asked to unset embedding, but X is still active\n"); that in fact how it goes, frame is unset before CloseWindow TODO: check why
        xcbt_embed_set(mX, nullptr);
      }
      xcbt_embed_dtor(mEmbed);
      mEmbed = nullptr;
    }
  }
  else
  {
    if (mEmbed)
      DBGMSG("BUG: embed is already set\n");
    else
      mEmbed = e;
  }
}

void* IGraphicsLinux::OpenWindow(void* pParent)
{
  xcbt_rect r = {0, 0, static_cast<int16_t>(WindowWidth()), static_cast<int16_t>(WindowHeight())};
  xcb_window_t xprt = (intptr_t) pParent;
  
#ifdef APP_API
  if (!mEmbed)
  {
    SetIntegration(xcbt_embed_glib());
  }
#endif

  if (!mEmbed)
  {
    DBGMSG("BUG: embed is not defined\n");
    return NULL;
  }

#ifdef IGRAPHICS_GL
  mX = xcbt_connect(XCBT_USE_GL|XCBT_INIT_ATOMS);
#else
  mX = xcbt_connect(0);
#endif
  if (!mX)
  {
    return NULL;
  }

#ifdef LV2_API
  if (!xprt)
  {
    // LV2 UI is created without parent by default, it may be found and even required with ui:parent feature, but the documentation
    // say that is not a good idea.
    xcb_screen_t* si = xcbt_screen_info(mX, xcbt_default_screen(mX));
    if (si)
    {
      xprt = si->root;
    }
  }
#endif

  // NOTE: In case plug-in report REAPER extension in REAPER, pParent is NOT XID (SWELL HWND? I have not checked yet)

#ifdef IGRAPHICS_GL
#ifdef IGRAPHICS_GL2
  mPlugWnd = xcbt_window_gl_create(mX, xprt, &r, 2, 1, 0);
#elif defined IGRAPHICS_GL3
  mPlugWnd = xcbt_window_gl_create(mX, xprt, &r, 3, 0, 0);
#else
  #error "Unsupported GL version"
#endif
#else

  mPlugWnd = xcbt_window_create(mX, xprt, &r);
#endif
  if (!mPlugWnd)
  {
    xcbt_disconnect(mX);
    mX = NULL;
    return NULL;
  }

  xcbt_window_set_handler(mPlugWnd, (xcbt_window_handler) WindowHandlerProxy, this, &mBaseWindowHandler, &mBaseWindowData);

  if (mEmbed && !xcbt_embed_set(mX, mEmbed))
  {
    DBGMSG("Could not embed into main event loop\n");
    xcbt_window_destroy(mPlugWnd);
    mPlugWnd = NULL;
    xcbt_disconnect(mX);
    mX = NULL;
    return NULL;
  }

  if (xcbt_window_draw_begin(mPlugWnd)) // GL context set
  { 
    OnViewInitialized(nullptr);
    SetScreenScale(1); // resizes draw context, calls DrawResize

    GetDelegate()->LayoutUI(this);
    SetAllControlsDirty();
    GetDelegate()->OnUIOpen();

    xcbt_window_draw_stop(mPlugWnd);
  }

  xcbt_timer_set(mX, IPLUG_TIMER_ID, 10, (xcbt_timer_cb) TimerHandlerProxy, this);

#ifdef APP_API
  xcbt_window_map(mPlugWnd);
  //xcbt_window_set_xembed_info(mPlugWnd);
#elif defined VST2_API
  xcbt_window_set_xembed_info(mPlugWnd);
#elif defined VST3_API
  xcbt_window_set_xembed_info(mPlugWnd);
#elif defined LV2_API
  xcbt_window_set_xembed_info(mPlugWnd);
#else
  #error "Map or not to map... that is the question"
#endif
  xcbt_sync(mX); // make sure everything is ready before reporting it is

  // Reset some state
  mCursorLock = false;
  mMouseVisible = true;

  return reinterpret_cast<void*>(xcbt_window_xwnd(mPlugWnd));
}

void IGraphicsLinux::CloseWindow()
{
  if (mPlugWnd)
  {
    OnViewDestroyed();

    SetPlatformContext(nullptr);

    xcbt_window_destroy(mPlugWnd);
    mPlugWnd = NULL;
    xcbt_disconnect(mX);
    mX = NULL;
  }

  if (mEmbed)
  {
    mEmbed->dtor(mEmbed);
    mEmbed = nullptr;
  }
}

void IGraphicsLinux::GetMouseLocation(float& x, float& y) const
{
  x = mCursorX;
  y = mCursorY;
}

void IGraphicsLinux::HideMouseCursor(bool hide, bool lock)
{
  if (mCursorHidden != hide)
  {
    mCursorHidden = hide;
    // https://stackoverflow.com/questions/57841785/how-to-hide-cursor-in-xcb

    if (mCursorHidden)
    {
      xcb_grab_pointer(xcbt_conn(mX), 1, mPlugWnd->wnd,
        XCB_EVENT_MASK_BUTTON_MOTION | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE,
        XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, mPlugWnd->wnd, XCB_NONE, XCB_CURRENT_TIME);
      xcb_xfixes_hide_cursor_checked(xcbt_conn(mX), mPlugWnd->wnd);
    }
    else
    {
      xcb_ungrab_pointer(xcbt_conn(mX), XCB_CURRENT_TIME);
      xcb_xfixes_show_cursor_checked(xcbt_conn(mX), mPlugWnd->wnd);
    }
  }

  if (mCursorLock != lock)
  {
    mCursorLock = lock;

    if (mCursorLock)
      mMouseLockPos = IVec2(mCursorX, mCursorY);
    else
      mMouseLockPos = IVec2(0, 0);
  }
}

void IGraphicsLinux::MoveMouseCursor(float x, float y)
{
  xcbt_move_cursor(mPlugWnd, XCBT_WINDOW, (int)x, (int)y);
  mCursorX = mMouseLockPos.x;
  mCursorY = mMouseLockPos.y;
}

EMsgBoxResult IGraphicsLinux::ShowMessageBox(const char* text, const char* caption, EMsgBoxType type, IMsgBoxCompletionHanderFunc completionHandler)
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

void IGraphicsLinux::PromptForFile(WDL_String& fileName, WDL_String& path, EFileAction action, const char* extensions)
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
}

void IGraphicsLinux::PromptForDirectory(WDL_String& dir)
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
}

bool IGraphicsLinux::OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure)
{
  WDL_String args;
  args.SetFormatted(strlen(url) + 40, "xdg-open \"%s\" ", url);

  WDL_String sOut, sIn;
  int status;

  if (RunSubprocess(args.Get(), sOut, sIn, &status) != 0)
  {
    return false;
  }

  return true;
}

bool IGraphicsLinux::GetTextFromClipboard(WDL_String& str)
{
  int length = 0;
  const char* data = xcbt_clipboard_get_utf8(mPlugWnd, &length);

  if (data)
  {
    str.Set(data, length);
    return true;
  }
  else
  {
    return false;
  }
}

bool IGraphicsLinux::SetTextInClipboard(const char* str)
{
  return xcbt_clipboard_set_utf8(mPlugWnd, str) == 1;
}

void IGraphicsLinux::PlatformResize(bool parentHasResized)
{
  if (WindowIsOpen())
  {
    xcb_connection_t *conn = xcbt_conn(mX);
    xcb_window_t w = xcbt_window_xwnd(mPlugWnd);
    uint32_t values[] = { static_cast<uint32_t>(WindowWidth() * GetScreenScale()), static_cast<uint32_t>(WindowHeight() * GetScreenScale()) };
    xcb_configure_window(conn, w, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);
    //DBGMSG("INFO: resized to %ux%u\n", values[0], values[1]);
    if (!parentHasResized)
    {
      DBGMSG("WARNING: parent is not resized, but I (should) have no control on it on X... XEMBED?\n");
      xcb_window_t prt = xcbt_window_xprt(mPlugWnd);
      if (prt)
      {
        xcb_configure_window(conn, prt, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);
      }
    }
    xcbt_flush(mX);
  }
}

void IGraphicsLinux::RequestFocus()
{
  xcb_set_input_focus_checked(xcbt_conn(mX), XCB_INPUT_FOCUS_POINTER_ROOT, mPlugWnd->wnd, XCB_CURRENT_TIME);
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

  if ((fontLocation == kNotFound) || (fontLocation != kAbsolutePath) )
  {
    return nullptr;
  }

  return PlatformFontPtr(new Font(fullPath));
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

uint32_t IGraphicsLinux::GetUserDblClickTimeout()
{
  // Default to 400
  uint32_t timeout = 400;

  // Source: forum.kde.org/viewtopic.php?f=289&t=153755
  // Read $HOME/.gtkrc-2.0; Var: gtk-double-click-time
  // Read $HOME/.config/gtk-3.0/settings.ini ; Var: gtk-double-click-time
  // Read $HOME/.config/kdeglobals, [KDE] section, Var: DoubleClickInterval
  return timeout;
}

IGraphicsLinux::IGraphicsLinux(IGEditorDelegate& dlg, int w, int h, int fps, float scale)
  : IGRAPHICS_DRAW_CLASS(dlg, w, h, fps, scale)
{
}

IGraphicsLinux::~IGraphicsLinux()
{
  CloseWindow();
  xcbt_embed_dtor(mEmbed);
}

#ifndef NO_IGRAPHICS
  #if defined IGRAPHICS_SKIA
    #include "IGraphicsSkia.cpp"
  #elif defined IGRAPHICS_NANOVG
    #include "IGraphicsNanoVG.cpp"
    #ifdef IGRAPHICS_FREETYPE
      #define FONS_USE_FREETYPE
    #endif
      #include "nanovg.c"
  #else
    #error
  #endif
#endif
