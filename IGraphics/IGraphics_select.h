#if !defined DOXYGEN_SHOULD_SKIP_THIS
  #if defined IGRAPHICS_LICE
    #include "IGraphicsLice.h"
    typedef IGraphicsLice IGRAPHICS_DRAW_CLASS;
  #elif defined IGRAPHICS_AGG
    #include "IGraphicsAGG.h"
    typedef IGraphicsAGG IGRAPHICS_DRAW_CLASS;
  #elif defined IGRAPHICS_CAIRO
    #include "IGraphicsCairo.h"
    typedef IGraphicsCairo IGRAPHICS_DRAW_CLASS;
  #elif defined IGRAPHICS_NANOVG
    #include "IGraphicsNanoVG.h"
    typedef IGraphicsNanoVG IGRAPHICS_DRAW_CLASS;
  #elif defined IGRAPHICS_SKIA
    #include "IGraphicsSkia.h"
    typedef IGraphicsSkia IGRAPHICS_DRAW_CLASS;
  #elif defined IGRAPHICS_CANVAS
     #include "IGraphicsCanvas.h"
     typedef IGraphicsCanvas IGRAPHICS_DRAW_CLASS;
  #else
    #error NO IGRAPHICS_MODE defined
  #endif
#endif
