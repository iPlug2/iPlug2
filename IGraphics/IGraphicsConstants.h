/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#include "IPlugPlatform.h"

static const int DEFAULT_FPS = 25; // TODO: default 60 FPS?

// If not dirty for this many timer ticks, we call OnGUIIDle.
// Only looked at if USE_IDLE_CALLS is defined.
static const int IDLE_TICKS = 20;

#define DEFAULT_ANIMATION_DURATION 100

#ifndef CONTROL_BOUNDS_COLOR
#define CONTROL_BOUNDS_COLOR COLOR_GREEN
#endif

#define PARAM_EDIT_W 40
#define PARAM_EDIT_H 16

#define MAX_URL_LEN 256
#define MAX_NET_ERR_MSG_LEN 1024

#define MAX_IMG_SCALE 3

static const int DEFAULT_TEXT_ENTRY_LEN = 7;
static const double DEFAULT_GEARING = 4.0;

//what is this stuff
#define MAX_INET_ERR_CODE 32
#define TOOLWIN_BORDER_W 6
#define TOOLWIN_BORDER_H 23
#define MAX_CLASSNAME_LEN 128
//

static const float GRAYED_ALPHA = 0.25f;

#ifndef DEFAULT_PATH
static const char* DEFAULT_PATH = "~/Desktop";
#endif

#ifdef IGRAPHICS_NANOVG
const char* const DEFAULT_FONT = "Roboto-Regular";
const int DEFAULT_TEXT_SIZE = 14;
#else
  #if defined OS_WIN
    const char* const DEFAULT_FONT = "Verdana";
    const int DEFAULT_TEXT_SIZE = 12;
  #elif defined OS_MAC
    const char* const DEFAULT_FONT = "Verdana";
    const int DEFAULT_TEXT_SIZE = 10;
  #elif defined OS_LINUX
    #error NOT IMPLEMENTED
  #elif defined OS_WEB
    const char* const DEFAULT_FONT = "Verdana";
    const int DEFAULT_TEXT_SIZE = 10;
  #endif
#endif

const int FONT_LEN = 32;

/** @enum EType Blend type
 * \todo This could use some documentation
 */
enum EBlendType
{
  kBlendNone,     // Copy over whatever is already there, but look at src alpha.
  kBlendClobber,  // Copy completely over whatever is already there.
  kBlendAdd,
  kBlendColorDodge,
  kBlendUnder,
  kBlendSourceIn,
  // etc
};

enum EFileAction
{
  kFileOpen,
  kFileSave  
};

enum EDirection
{
  kVertical = 0,
  kHorizontal = 1
};

enum EResourceLocation
{
  kNotFound = 0,
  kAbsolutePath,
  kWinBinary
};

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
  kX2,        // extra2
  kX3,        // extra3
  kNumDefaultVColors
};

enum EFillRule
{
  kFillWinding,
  kFillEvenOdd
};

enum ELineCap
{
  kCapButt,
  kCapRound,
  kCapSquare
};

enum ELineJoin
{
  kJoinMiter,
  kJoinRound,
  kJoinBevel
};

enum EPatternType
{
  kSolidPattern,
  kLinearPattern,
  kRadialPattern
};

enum EPatternExtend
{
  kExtendNone,
  kExtendPad,
  kExtendReflect,
  kExtendRepeat
};

enum EUIResizerMode
{
  kUIResizerScale,
  kUIResizerSize
};

enum ECursor
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
enum EMessageBoxType
{
  kMB_OK = 0,
  kMB_OKCANCEL = 1,
  kMB_YESNOCANCEL = 3,
  kMB_YESNO = 4,
  kMB_RETRYCANCEL = 5
};

// This enumeration must match win32 message box results
enum EMessageBoxResult
{
  kOK = 1,
  kCANCEL = 2,
  kABORT = 3,
  kRETRY = 4,
  kIGNORE = 5,
  kYES = 6,
  kNO = 7
};

static const char* kMessageResultStrs[8] = {"", "OK", "CANCEL", "ABORT", "RETRY", "IGNORE", "YES", "NO"};


