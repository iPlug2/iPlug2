#if defined IGRAPHICS_NANOVG
  #if defined IGRAPHICS_METAL
    #include "nanovg_mtl.m"
  #endif
  #if defined IGRAPHICS_FREETYPE
    #define FONS_USE_FREETYPE
  #endif
  #include "nanovg.c"
#endif
