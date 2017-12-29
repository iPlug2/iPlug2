#pragma once

#define DEFAULT_FPS 25

// If not dirty for this many timer ticks, we call OnGUIIDle.
// Only looked at if USE_IDLE_CALLS is defined.
#define IDLE_TICKS 20

#ifndef CONTROL_BOUNDS_COLOR
#define CONTROL_BOUNDS_COLOR COLOR_GREEN
#endif

#define PARAM_EDIT_W 40
#define PARAM_EDIT_H 16

#define MAX_URL_LEN 256
#define MAX_NET_ERR_MSG_LEN 1024

#define DEFAULT_TEXT_ENTRY_LEN 7

#define DEFAULT_GEARING 4.0

const float GRAYED_ALPHA = 0.25f;
