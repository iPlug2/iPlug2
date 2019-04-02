
/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
 */

#pragma once

#include <nanovg.h>

#ifdef __cplusplus
extern "C" {
#endif
int nvgCreateFontMemIdx(NVGcontext* ctx, const char* name, unsigned char* data, int dataSize, int faceIdx);

#ifdef __cplusplus
}
#endif
