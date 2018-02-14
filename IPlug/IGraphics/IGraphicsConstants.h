#pragma once

#include "IPlugPlatform.h"

static const int DEFAULT_FPS = 25;

// If not dirty for this many timer ticks, we call OnGUIIDle.
// Only looked at if USE_IDLE_CALLS is defined.
static const int IDLE_TICKS = 20;

#ifndef CONTROL_BOUNDS_COLOR
#define CONTROL_BOUNDS_COLOR COLOR_GREEN
#endif

#define PARAM_EDIT_W 40
#define PARAM_EDIT_H 16

#define MAX_URL_LEN 256
#define MAX_NET_ERR_MSG_LEN 1024

static const int DEFAULT_TEXT_ENTRY_LEN = 7;
static const double DEFAULT_GEARING = 4.0;

//what is this stuff
#define MAX_INET_ERR_CODE 32
#define TOOLWIN_BORDER_W 6
#define TOOLWIN_BORDER_H 23
#define MAX_CLASSNAME_LEN 128
//

static const float GRAYED_ALPHA = 0.25f;

#ifdef OS_WIN
const char* const DEFAULT_FONT = "Verdana";
const int DEFAULT_TEXT_SIZE = 12;
#elif defined OS_MAC
const char* const DEFAULT_FONT = "Monaco";
const int DEFAULT_TEXT_SIZE = 10;
#elif defined OS_LINUX
const char* const DEFAULT_FONT = "DejaVu Sans";
const int DEFAULT_TEXT_SIZE = 10;
#endif

const int FONT_LEN = 32;

/** @enum EType Blend type
 * @todo This could use some documentation
 */
enum EBlendType
{
  kBlendNone,     // Copy over whatever is already there, but look at src alpha.
  kBlendClobber,  // Copy completely over whatever is already there.
  kBlendAdd,
  kBlendColorDodge,
  // etc
};

enum EFileAction
{
  kFileOpen,
  kFileSave  
};

enum EDirection
{
  kVertical,
  kHorizontal
};

enum EVColor
{
  kBG = 0, // background
  kFG,     // foreground
  kFR,     // frame
  kHL,     // highlight
  kX1,     // extra1
  kX2,     // extra2
  kX3,     // extra3
  kNumDefaultVColors
};
