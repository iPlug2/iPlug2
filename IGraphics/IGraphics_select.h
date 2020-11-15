/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

/**
 * @file
 * @brief Used for choosing a drawing backend
 */

#if !defined DOXYGEN_SHOULD_SKIP_THIS

  #if defined IGRAPHICS_GLES2
    #define IGRAPHICS_GL
    #if defined OS_IOS
      #include <OpenGLES/ES2/gl.h>
    #elif defined OS_WEB
      #include <GLES2/gl2.h>
    #endif
  #elif defined IGRAPHICS_GLES3
    #define IGRAPHICS_GL
    #if defined OS_IOS
      #include <OpenGLES/ES3/gl.h>
    #elif defined OS_WEB
      #include <GLES3/gl3.h>
    #endif
  #elif defined IGRAPHICS_GL2 || defined IGRAPHICS_GL3
    #define IGRAPHICS_GL
    #if defined OS_WIN
      #include <glad/glad.h>
    #elif defined OS_MAC
      #if defined IGRAPHICS_GL2
        #include <OpenGL/gl.h>
      #elif defined IGRAPHICS_GL3
        #include <OpenGL/gl3.h>
      #endif
    #else
      #include <OpenGL/gl.h>
    #endif
  #endif

  #if defined IGRAPHICS_NANOVG
    #include "IGraphicsNanoVG.h"
    #define IGRAPHICS_DRAW_CLASS_TYPE IGraphicsNanoVG
  #elif defined IGRAPHICS_SKIA
    #include "IGraphicsSkia.h"
    #define IGRAPHICS_DRAW_CLASS_TYPE IGraphicsSkia
  #elif defined IGRAPHICS_CANVAS
     #include "IGraphicsCanvas.h"
     #define IGRAPHICS_DRAW_CLASS_TYPE IGraphicsCanvas
    #if defined IGRAPHICS_GL || defined IGRAPHICS_METAL
      #error When using IGRAPHICS_CANVAS, don't define IGRAPHICS_METAL or IGRAPHICS_GL*
    #endif
  #else
    #error NO IGRAPHICS_MODE defined
  #endif
#endif

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE
using IGRAPHICS_DRAW_CLASS = IGRAPHICS_DRAW_CLASS_TYPE;
END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE

#if defined IGRAPHICS_IMGUI
  #include "imgui.h"
#endif
