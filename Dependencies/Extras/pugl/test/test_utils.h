// Copyright 2012-2023 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef TEST_TEST_UTILS_H
#define TEST_TEST_UTILS_H

#include "pugl/pugl.h"

#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#ifdef __GNUC__
#  define PUGL_LOG_FUNC(fmt, arg1) __attribute__((format(printf, fmt, arg1)))
#else
#  define PUGL_LOG_FUNC(fmt, arg1)
#endif

typedef struct {
  int  samples;
  int  doubleBuffer;
  int  sync;
  int  glApi;
  int  glMajorVersion;
  int  glMinorVersion;
  bool continuous;
  bool help;
  bool ignoreKeyRepeat;
  bool resizable;
  bool verbose;
  bool errorChecking;
} PuglTestOptions;

PUGL_LOG_FUNC(1, 2)
static int
logError(const char* fmt, ...)
{
  fprintf(stderr, "error: ");

  va_list args; // NOLINT
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);

  return 1;
}

static inline int
printModifiers(const uint32_t mods)
{
  return fprintf(stderr,
                 "Modifiers:%s%s%s%s\n",
                 (mods & PUGL_MOD_SHIFT) ? " Shift" : "",
                 (mods & PUGL_MOD_CTRL) ? " Ctrl" : "",
                 (mods & PUGL_MOD_ALT) ? " Alt" : "",
                 (mods & PUGL_MOD_SUPER) ? " Super" : "");
}

static inline const char*
crossingModeString(const PuglCrossingMode mode)
{
  switch (mode) {
  case PUGL_CROSSING_NORMAL:
    return "normal";
  case PUGL_CROSSING_GRAB:
    return "grab";
  case PUGL_CROSSING_UNGRAB:
    return "ungrab";
  }

  return "unknown";
}

static inline const char*
scrollDirectionString(const PuglScrollDirection direction)
{
  switch (direction) {
  case PUGL_SCROLL_UP:
    return "up";
  case PUGL_SCROLL_DOWN:
    return "down";
  case PUGL_SCROLL_LEFT:
    return "left";
  case PUGL_SCROLL_RIGHT:
    return "right";
  case PUGL_SCROLL_SMOOTH:
    return "smooth";
  }

  return "unknown";
}

static inline const char*
viewStyleFlagString(const PuglViewStyleFlag state)
{
  switch (state) {
  case PUGL_VIEW_STYLE_MODAL:
    return "modal";
  case PUGL_VIEW_STYLE_TALL:
    return "tall";
  case PUGL_VIEW_STYLE_WIDE:
    return "wide";
  case PUGL_VIEW_STYLE_HIDDEN:
    return "hidden";
  case PUGL_VIEW_STYLE_FULLSCREEN:
    return "fullscreen";
  case PUGL_VIEW_STYLE_ABOVE:
    return "above";
  case PUGL_VIEW_STYLE_BELOW:
    return "below";
  case PUGL_VIEW_STYLE_DEMANDING:
    return "demanding";
  case PUGL_VIEW_STYLE_RESIZING:
    return "resizing";
  case PUGL_VIEW_STYLE_MAPPED:
    return "mapped";
  }

  return "unknown";
}

static inline const char*
keyString(const uint32_t key)
{
  switch (key) {
  case PUGL_KEY_BACKSPACE:
    return "BACKSPACE";
  case PUGL_KEY_ENTER:
    return "ENTER";
  case PUGL_KEY_ESCAPE:
    return "ESCAPE";
  case PUGL_KEY_DELETE:
    return "DELETE";
  case PUGL_KEY_SPACE:
    return "SPACE";

  case PUGL_KEY_F1:
    return "F1";
  case PUGL_KEY_F2:
    return "F2";
  case PUGL_KEY_F3:
    return "F3";
  case PUGL_KEY_F4:
    return "F4";
  case PUGL_KEY_F5:
    return "F5";
  case PUGL_KEY_F6:
    return "F6";
  case PUGL_KEY_F7:
    return "F7";
  case PUGL_KEY_F8:
    return "F8";
  case PUGL_KEY_F9:
    return "F9";
  case PUGL_KEY_F10:
    return "F10";
  case PUGL_KEY_F11:
    return "F11";
  case PUGL_KEY_F12:
    return "F12";

  case PUGL_KEY_PAGE_UP:
    return "PAGE_UP";
  case PUGL_KEY_PAGE_DOWN:
    return "PAGE_DOWN";
  case PUGL_KEY_END:
    return "END";
  case PUGL_KEY_HOME:
    return "HOME";
  case PUGL_KEY_LEFT:
    return "LEFT";
  case PUGL_KEY_UP:
    return "UP";
  case PUGL_KEY_RIGHT:
    return "RIGHT";
  case PUGL_KEY_DOWN:
    return "DOWN";

  case PUGL_KEY_PRINT_SCREEN:
    return "PRINT_SCREEN";
  case PUGL_KEY_INSERT:
    return "INSERT";
  case PUGL_KEY_PAUSE:
    return "PAUSE";
  case PUGL_KEY_MENU:
    return "MENU";
  case PUGL_KEY_NUM_LOCK:
    return "NUM_LOCK";
  case PUGL_KEY_SCROLL_LOCK:
    return "SCROLL_LOCK";
  case PUGL_KEY_CAPS_LOCK:
    return "CAPS_LOCK";

  case PUGL_KEY_SHIFT_L:
    return "SHIFT_L";
  case PUGL_KEY_SHIFT_R:
    return "SHIFT_R";
  case PUGL_KEY_CTRL_L:
    return "CTRL_L";
  case PUGL_KEY_CTRL_R:
    return "CTRL_R";
  case PUGL_KEY_ALT_L:
    return "ALT_L";
  case PUGL_KEY_ALT_R:
    return "ALT_R";
  case PUGL_KEY_SUPER_L:
    return "SUPER_L";
  case PUGL_KEY_SUPER_R:
    return "SUPER_R";

  case PUGL_KEY_PAD_0:
    return "PAD_0";
  case PUGL_KEY_PAD_1:
    return "PAD_1";
  case PUGL_KEY_PAD_2:
    return "PAD_2";
  case PUGL_KEY_PAD_3:
    return "PAD_3";
  case PUGL_KEY_PAD_4:
    return "PAD_4";
  case PUGL_KEY_PAD_5:
    return "PAD_5";
  case PUGL_KEY_PAD_6:
    return "PAD_6";
  case PUGL_KEY_PAD_7:
    return "PAD_7";
  case PUGL_KEY_PAD_8:
    return "PAD_8";
  case PUGL_KEY_PAD_9:
    return "PAD_9";
  case PUGL_KEY_PAD_ENTER:
    return "PAD_ENTER";

  case PUGL_KEY_PAD_PAGE_UP:
    return "PAD_PAGE_UP";
  case PUGL_KEY_PAD_PAGE_DOWN:
    return "PAD_PAGE_DOWN";
  case PUGL_KEY_PAD_END:
    return "PAD_END";
  case PUGL_KEY_PAD_HOME:
    return "PAD_HOME";
  case PUGL_KEY_PAD_LEFT:
    return "PAD_LEFT";
  case PUGL_KEY_PAD_UP:
    return "PAD_UP";
  case PUGL_KEY_PAD_RIGHT:
    return "PAD_RIGHT";
  case PUGL_KEY_PAD_DOWN:
    return "PAD_DOWN";

  case PUGL_KEY_PAD_CLEAR:
    return "PAD_CLEAR";
  case PUGL_KEY_PAD_INSERT:
    return "PAD_INSERT";
  case PUGL_KEY_PAD_DELETE:
    return "PAD_DELETE";
  case PUGL_KEY_PAD_EQUAL:
    return "PAD_EQUAL";

  case PUGL_KEY_PAD_MULTIPLY:
    return "PAD_MULTIPLY";
  case PUGL_KEY_PAD_ADD:
    return "PAD_ADD";
  case PUGL_KEY_PAD_SEPARATOR:
    return "PAD_SEPARATOR";
  case PUGL_KEY_PAD_SUBTRACT:
    return "PAD_SUBTRACT";
  case PUGL_KEY_PAD_DECIMAL:
    return "PAD_DECIMAL";
  case PUGL_KEY_PAD_DIVIDE:
    return "PAD_DIVIDE";
  }

  return "";
}

static inline int
printEvent(const PuglEvent* event, const char* prefix, const bool verbose)
{
#define PFFMT "%6.1f %6.1f"
#define PIFMT "%5d %5d"
#define PUFMT "%5u %5u"

#define PRINT(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__)

  switch (event->type) {
  case PUGL_NOTHING:
    return 0;
  case PUGL_REALIZE:
    return fprintf(stderr, "%sRealize\n", prefix);
  case PUGL_UNREALIZE:
    return fprintf(stderr, "%sUnrealize\n", prefix);
  case PUGL_KEY_PRESS:
    return PRINT("%sKey press   code %3u key  U+%04X (%s)\n",
                 prefix,
                 event->key.keycode,
                 event->key.key,
                 keyString(event->key.key));
  case PUGL_KEY_RELEASE:
    return PRINT("%sKey release code %3u key  U+%04X (%s)\n",
                 prefix,
                 event->key.keycode,
                 event->key.key,
                 keyString(event->key.key));
  case PUGL_TEXT:
    return PRINT("%sText entry  code %3u char U+%04X (%s)\n",
                 prefix,
                 event->text.keycode,
                 event->text.character,
                 event->text.string);
  case PUGL_BUTTON_PRESS:
  case PUGL_BUTTON_RELEASE:
    return (PRINT("%sMouse %u %s at " PFFMT " ",
                  prefix,
                  event->button.button,
                  (event->type == PUGL_BUTTON_PRESS) ? "down" : "up  ",
                  event->button.x,
                  event->button.y) +
            printModifiers(event->scroll.state));
  case PUGL_SCROLL:
    return (PRINT("%sScroll %5.1f %5.1f (%s) at " PFFMT " ",
                  prefix,
                  event->scroll.dx,
                  event->scroll.dy,
                  scrollDirectionString(event->scroll.direction),
                  event->scroll.x,
                  event->scroll.y) +
            printModifiers(event->scroll.state));
  case PUGL_POINTER_IN:
    return PRINT("%sMouse enter  at " PFFMT " (%s)\n",
                 prefix,
                 event->crossing.x,
                 event->crossing.y,
                 crossingModeString(event->crossing.mode));
  case PUGL_POINTER_OUT:
    return PRINT("%sMouse leave  at " PFFMT " (%s)\n",
                 prefix,
                 event->crossing.x,
                 event->crossing.y,
                 crossingModeString(event->crossing.mode));
  case PUGL_FOCUS_IN:
    return PRINT(
      "%sFocus in (%s)\n", prefix, crossingModeString(event->crossing.mode));
  case PUGL_FOCUS_OUT:
    return PRINT(
      "%sFocus out (%s)\n", prefix, crossingModeString(event->crossing.mode));
  case PUGL_CLIENT:
    return PRINT("%sClient %" PRIXPTR " %" PRIXPTR "\n",
                 prefix,
                 event->client.data1,
                 event->client.data2);
  case PUGL_LOOP_ENTER:
    return PRINT("%sLoop enter\n", prefix);
  case PUGL_LOOP_LEAVE:
    return PRINT("%sLoop leave\n", prefix);
  case PUGL_DATA_OFFER:
    return PRINT("%sData offer\n", prefix);
  case PUGL_DATA:
    return PRINT("%sData\n", prefix);
  default:
    break;
  }

  if (verbose) {
    switch (event->type) {
    case PUGL_UPDATE:
      return fprintf(stderr, "%sUpdate\n", prefix);
    case PUGL_CONFIGURE:
      PRINT("%sConfigure " PIFMT " " PUFMT " (",
            prefix,
            event->configure.x,
            event->configure.y,
            event->configure.width,
            event->configure.height);
      for (PuglViewStyleFlags mask = 1U; mask <= PUGL_MAX_VIEW_STYLE_FLAG;
           mask <<= 1U) {
        if (event->configure.style & mask) {
          PRINT(" %s", viewStyleFlagString((PuglViewStyleFlag)mask));
        }
      }
      PRINT("%s\n", " )");
      return 0;
    case PUGL_EXPOSE:
      return PRINT("%sExpose    " PIFMT " " PUFMT "\n",
                   prefix,
                   event->expose.x,
                   event->expose.y,
                   event->expose.width,
                   event->expose.height);
    case PUGL_CLOSE:
      return PRINT("%sClose\n", prefix);
    case PUGL_MOTION:
      return PRINT("%sMouse motion at " PFFMT "\n",
                   prefix,
                   event->motion.x,
                   event->motion.y);
    case PUGL_TIMER:
      return PRINT("%sTimer %" PRIuPTR "\n", prefix, event->timer.id);
    default:
      return PRINT("%sUnknown event type %d\n", prefix, (int)event->type);
    }
  }

#undef PRINT
#undef PUFMT
#undef PIFMT
#undef PFFMT

  return 0;
}

static inline const char*
puglViewHintString(const PuglViewHint hint)
{
  switch (hint) {
  case PUGL_CONTEXT_API:
    return "Context API";
  case PUGL_CONTEXT_VERSION_MAJOR:
    return "Context major version";
  case PUGL_CONTEXT_VERSION_MINOR:
    return "Context minor version";
  case PUGL_CONTEXT_PROFILE:
    return "Context profile";
  case PUGL_CONTEXT_DEBUG:
    return "Context debug";
  case PUGL_RED_BITS:
    return "Red bits";
  case PUGL_GREEN_BITS:
    return "Green bits";
  case PUGL_BLUE_BITS:
    return "Blue bits";
  case PUGL_ALPHA_BITS:
    return "Alpha bits";
  case PUGL_DEPTH_BITS:
    return "Depth bits";
  case PUGL_STENCIL_BITS:
    return "Stencil bits";
  case PUGL_SAMPLE_BUFFERS:
    return "Sample buffers";
  case PUGL_SAMPLES:
    return "Samples";
  case PUGL_DOUBLE_BUFFER:
    return "Double buffer";
  case PUGL_SWAP_INTERVAL:
    return "Swap interval";
  case PUGL_RESIZABLE:
    return "Resizable";
  case PUGL_IGNORE_KEY_REPEAT:
    return "Ignore key repeat";
  case PUGL_REFRESH_RATE:
    return "Refresh rate";
  case PUGL_VIEW_TYPE:
    return "View type";
  case PUGL_DARK_FRAME:
    return "Dark frame";
  }

  return "Unknown";
}

static inline void
printViewHints(const PuglView* view)
{
  for (unsigned i = 0; i < PUGL_NUM_VIEW_HINTS; ++i) {
    const PuglViewHint hint = (PuglViewHint)i;
    fprintf(stderr,
            "%s: %d\n",
            puglViewHintString(hint),
            puglGetViewHint(view, hint));
  }
}

static inline void
puglPrintTestUsage(const char* prog, const char* posHelp)
{
  printf("Usage: %s [OPTION]... %s\n\n"
         "  -E  Use OpenGL ES\n"
         "  -G  OpenGL context version\n"
         "  -a  Enable anti-aliasing\n"
         "  -b  Block and only update on user input\n"
         "  -d  Directly draw to window (no double-buffering)\n"
         "  -e  Enable platform error-checking\n"
         "  -f  Fast drawing, explicitly disable vertical sync\n"
         "  -h  Display this help\n"
         "  -i  Ignore key repeat\n"
         "  -v  Print verbose output\n"
         "  -r  Resizable window\n"
         "  -s  Explicitly enable vertical sync\n",
         prog,
         posHelp);
}

static inline PuglTestOptions
puglParseTestOptions(int* pargc, char*** pargv)
{
  PuglTestOptions opts = {
    0,
    PUGL_TRUE,
    PUGL_DONT_CARE,
    PUGL_OPENGL_API,
    3,
    3,
    true,
    false,
    false,
    false,
    false,
    false,
  };

  char** const argv = *pargv;
  int          i    = 1;
  for (; i < *pargc; ++i) {
    if (!strcmp(argv[i], "-E")) {
      opts.glApi = PUGL_OPENGL_ES_API;
    } else if (!strcmp(argv[i], "-G")) {
      if (++i == *pargc) {
        fprintf(stderr, "error: Missing OpenGL version argument\n");
        return opts;
      }

      const int matches =
        sscanf(argv[i], "%d.%d", &opts.glMajorVersion, &opts.glMinorVersion);
      if (matches != 2) {
        fprintf(stderr, "error: Invalid OpenGL version argument\n");
        return opts;
      }
    } else if (!strcmp(argv[i], "-a")) {
      opts.samples = 4;
    } else if (!strcmp(argv[i], "-b")) {
      opts.continuous = false;
    } else if (!strcmp(argv[i], "-d")) {
      opts.doubleBuffer = PUGL_FALSE;
    } else if (!strcmp(argv[i], "-e")) {
      opts.errorChecking = PUGL_TRUE;
    } else if (!strcmp(argv[i], "-f")) {
      opts.sync = PUGL_FALSE;
    } else if (!strcmp(argv[i], "-h")) {
      opts.help = true;
      return opts;
    } else if (!strcmp(argv[i], "-i")) {
      opts.ignoreKeyRepeat = true;
    } else if (!strcmp(argv[i], "-r")) {
      opts.resizable = true;
    } else if (!strcmp(argv[i], "-s")) {
      opts.sync = PUGL_TRUE;
    } else if (!strcmp(argv[i], "-v")) {
      opts.verbose = true;
    } else if (argv[i][0] != '-') {
      break;
    } else {
      opts.help = true;
      logError("Unknown option: %s\n", argv[i]);
    }
  }

  *pargc -= i;
  *pargv += i;

  return opts;
}

#endif // TEST_TEST_UTILS_H
