/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#include "IPlugPlatform.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

static constexpr int DEFAULT_FPS = 25; // TODO: default 60 FPS?

// If not dirty for this many timer ticks, we call OnGUIIDle.
// Only looked at if USE_IDLE_CALLS is defined.
static constexpr int IDLE_TICKS = 20;

static constexpr int DEFAULT_ANIMATION_DURATION = 100;

#ifndef CONTROL_BOUNDS_COLOR
#define CONTROL_BOUNDS_COLOR COLOR_GREEN
#endif

static constexpr float PARAM_EDIT_W = 40.f; // TODO: remove?
static constexpr float PARAM_EDIT_H = 16.f; // TODO: remove?

#define MAX_URL_LEN 256
#define MAX_NET_ERR_MSG_LEN 1024

static constexpr int MAX_IMG_SCALE = 3;
static constexpr int DEFAULT_TEXT_ENTRY_LEN = 7;
static constexpr double DEFAULT_GEARING = 4.0;

//what is this stuff
#define MAX_INET_ERR_CODE 32
#define TOOLWIN_BORDER_W 6
#define TOOLWIN_BORDER_H 23
#define MAX_CLASSNAME_LEN 128
//

static constexpr float GRAYED_ALPHA = 0.25f;

#ifndef DEFAULT_PATH
static const char* DEFAULT_PATH = "~/Desktop";
#endif

const char* const DEFAULT_FONT = "Roboto-Regular";
static constexpr float DEFAULT_TEXT_SIZE = 14.f;
static constexpr int FONT_LEN = 64;

/** @enum EType Blend type
 * \todo This could use some documentation
 */
enum class EBlend
{
  Default,
  Clobber,
  SourceOver,
  SourceIn,
  SourceOut,
  SourceAtop,
  DestOver,
  DestIn,
  DestOut,
  DestAtop,
  Add,
  XOR,
  None = EBlend::Default
};

/** /todo */
enum class EFileAction { Open, Save };

/** /todo */
enum class EDirection { Vertical, Horizontal };

/** Used to specify text styles when loading fonts. */
enum class ETextStyle { Normal, Bold, Italic };

/** /todo */
enum class EAlign { Near, Center, Far };

/** /todo */
enum class EVAlign { Top, Middle, Bottom };

/** /todo */
enum EVColor
{
  kBG = 0,    // background color: All vector controls should fill their BG with this color, which is transparent by default
  kFG,        // foreground
  kOFF = kFG, // off states will use the same color as kFG to fill
  kPR,        // pressed
  kON = kPR,  // on states will use the same color as kPR to fill
  kFR,        // frame: the color of the stroke/borders
  kHL,        // highlight: mouse over or focus
  kSH,        // shadow
  kX1,        // extra1
  kGR = kX1,  // greyed
  kX2,        // extra2
  kX3,        // extra3
  kNumDefaultVColors
};

static const char* kVColorStrs[kNumDefaultVColors] =
{
  "background",
  "foreground/off states",
  "pressed/on states",
  "frame",
  "highlight",
  "shadow",
  "extra1/greyed",
  "extra2",
  "extra3"
};

/** /todo */
enum class EVShape { Rectangle, Ellipse, Triangle, EndsRounded, AllRounded };

/** /todo */
enum class EWinding { CW, CCW };

/** /todo */
enum class EFillRule { Winding, EvenOdd };

/** /todo */
enum class ELineCap { Butt, Round, Square };

/** /todo */
enum class ELineJoin { Miter, Round, Bevel };

/** /todo */
enum class EPatternType { Solid, Linear, Radial };

/** /todo */
enum class EPatternExtend { None, Pad, Reflect, Repeat };

/** /todo */
enum class EUIResizerMode { Scale, Size };

/** /todo */
enum class ECursor
{
  ARROW,
  IBEAM,
  WAIT,
  CROSS,
  UPARROW,
  SIZENWSE,
  SIZENESW,
  SIZEWE,
  SIZENS,
  SIZEALL,
  INO,
  HAND,
  APPSTARTING,
  HELP
};

// This enumeration must match win32 message box options
enum EMsgBoxType
{
  kMB_OK = 0,
  kMB_OKCANCEL = 1,
  kMB_YESNOCANCEL = 3,
  kMB_YESNO = 4,
  kMB_RETRYCANCEL = 5
};

// This enumeration must match win32 message box results
 //If IGraphics::ShowMessageBox can't return inline, it returns kNoResult (e.g. because it requires an asynchronous call)
enum EMsgBoxResult
{
  kNoResult,
  kOK = 1,
  kCANCEL = 2,
  kABORT = 3,
  kRETRY = 4,
  kIGNORE = 5,
  kYES = 6,
  kNO = 7
};

static const char* kMessageResultStrs[8] = {"", "OK", "CANCEL", "ABORT", "RETRY", "IGNORE", "YES", "NO"};

// This enumeration must match win32 Fkeys as specified in winuser.h
enum ESpecialKey
{
  kFVIRTKEY  = 0x01,
  kFSHIFT    = 0x04,
  kFCONTROL  = 0x08,
  kFALT      = 0x10,
  kFLWIN     = 0x20
};

// This enumeration must match win32 virtual keys as specified in winuser.h
enum EVirtualKey
{
  kVK_NONE =        0x00,
    
  kVK_LBUTTON =     0x01,
  kVK_RBUTTON =     0x02,
  kVK_MBUTTON =     0x04,

  kVK_BACK =        0x08,
  kVK_TAB =         0x09,

  kVK_CLEAR =       0x0C,
  kVK_RETURN =      0x0D,

  kVK_SHIFT =       0x10,
  kVK_CONTROL =     0x11,
  kVK_MENU =        0x12,
  kVK_PAUSE =       0x13,
  kVK_CAPITAL =     0x14,

  kVK_ESCAPE =      0x1B,

  kVK_SPACE =       0x20,
  kVK_PRIOR =       0x21,
  kVK_NEXT =        0x22,
  kVK_END =         0x23,
  kVK_HOME =        0x24,
  kVK_LEFT =        0x25,
  kVK_UP =          0x26,
  kVK_RIGHT =       0x27,
  kVK_DOWN =        0x28,
  kVK_SELECT =      0x29,
  kVK_PRINT =       0x2A,
  kVK_SNAPSHOT =    0x2C,
  kVK_INSERT =      0x2D,
  kVK_DELETE =      0x2E,
  kVK_HELP =        0x2F,

  kVK_0 =           0x30,
  kVK_1 =           0x31,
  kVK_2 =           0x32,
  kVK_3 =           0x33,
  kVK_4 =           0x34,
  kVK_5 =           0x35,
  kVK_6 =           0x36,
  kVK_7 =           0x37,
  kVK_8 =           0x38,
  kVK_9 =           0x39,
  kVK_A =           0x41,
  kVK_B =           0x42,
  kVK_C =           0x43,
  kVK_D =           0x44,
  kVK_E =           0x45,
  kVK_F =           0x46,
  kVK_G =           0x47,
  kVK_H =           0x48,
  kVK_I =           0x49,
  kVK_J =           0x4A,
  kVK_K =           0x4B,
  kVK_L =           0x4C,
  kVK_M =           0x4D,
  kVK_N =           0x4E,
  kVK_O =           0x4F,
  kVK_P =           0x50,
  kVK_Q =           0x51,
  kVK_R =           0x52,
  kVK_S =           0x53,
  kVK_T =           0x54,
  kVK_U =           0x55,
  kVK_V =           0x56,
  kVK_W =           0x57,
  kVK_X =           0x58,
  kVK_Y =           0x59,
  kVK_Z =           0x5A,
  
  kVK_LWIN =        0x5B,
  
  kVK_NUMPAD0 =     0x60,
  kVK_NUMPAD1 =     0x61,
  kVK_NUMPAD2 =     0x62,
  kVK_NUMPAD3 =     0x63,
  kVK_NUMPAD4 =     0x64,
  kVK_NUMPAD5 =     0x65,
  kVK_NUMPAD6 =     0x66,
  kVK_NUMPAD7 =     0x67,
  kVK_NUMPAD8 =     0x68,
  kVK_NUMPAD9 =     0x69,
  kVK_MULTIPLY =    0x6A,
  kVK_ADD =         0x6B,
  kVK_SEPARATOR =   0x6C,
  kVK_SUBTRACT =    0x6D,
  kVK_DECIMAL =     0x6E,
  kVK_DIVIDE =      0x6F,
  kVK_F1 =          0x70,
  kVK_F2 =          0x71,
  kVK_F3 =          0x72,
  kVK_F4 =          0x73,
  kVK_F5 =          0x74,
  kVK_F6 =          0x75,
  kVK_F7 =          0x76,
  kVK_F8 =          0x77,
  kVK_F9 =          0x78,
  kVK_F10 =         0x79,
  kVK_F11 =         0x7A,
  kVK_F12 =         0x7B,
  kVK_F13 =         0x7C,
  kVK_F14 =         0x7D,
  kVK_F15 =         0x7E,
  kVK_F16 =         0x7F,
  kVK_F17 =         0x80,
  kVK_F18 =         0x81,
  kVK_F19 =         0x82,
  kVK_F20 =         0x83,
  kVK_F21 =         0x84,
  kVK_F22 =         0x85,
  kVK_F23 =         0x86,
  kVK_F24 =         0x87,

  kVK_NUMLOCK =     0x90,
  kVK_SCROLL =      0x91
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
