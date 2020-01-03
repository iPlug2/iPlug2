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
enum class EFillRule { Winding, EvenOdd, Preserve };

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

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
