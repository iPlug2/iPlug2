/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#if !__has_feature(objc_arc)
#error This file must be compiled with Arc. Use -fobjc-arc flag
#endif

#if defined IGRAPHICS_NANOVG
  #if defined IGRAPHICS_METAL
    #include "nanovg_mtl.m"
  #endif
  #if defined IGRAPHICS_FREETYPE
    #define FONS_USE_FREETYPE
  #endif
  #include "nanovg.c"
#endif
