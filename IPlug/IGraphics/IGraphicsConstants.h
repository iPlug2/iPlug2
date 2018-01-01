#pragma once

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

