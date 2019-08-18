#if defined IGRAPHICS_NANOVG
  #if defined IGRAPHICS_METAL
    #define MNVGtexture CONCAT(MNVGtexture_)
    #define MNVGbuffers CONCAT(MNVGbuffers_)
    #define MNVGcontext CONCAT(MNVGcontext_)
    #include "nanovg_mtl.m"
  #endif
  #if defined IGRAPHICS_FREETYPE
    #define FONS_USE_FREETYPE
  #endif
  #include "nanovg.c"
#endif
