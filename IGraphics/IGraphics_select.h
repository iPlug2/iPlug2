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
BEGIN_INCLUDE_DEPENDENCIES
  #if defined IGRAPHICS_GLES2
    #define IGRAPHICS_GL
    #if PLATFORM_IOS
      #include <OpenGLES/ES2/gl.h>
    #elif PLATFORM_WEB
      #include <GLES2/gl2.h>
    #endif
  #elif defined IGRAPHICS_GLES3
    #define IGRAPHICS_GL
    #if PLATFORM_IOS
      #include <OpenGLES/ES3/gl.h>
    #elif PLATFORM_WEB
      #include <GLES3/gl3.h>
    #endif
  #elif defined IGRAPHICS_GL2 || defined IGRAPHICS_GL3
    #define IGRAPHICS_GL
    #if PLATFORM_WINDOWS
      #include <glad/glad.h>
    #elif PLATFORM_MAC
      #if defined IGRAPHICS_GL2
        #include <OpenGL/gl.h>
      #elif defined IGRAPHICS_GL3
        #include <OpenGL/gl3.h>
      #endif
    #else
      #include <OpenGL/gl.h>
    #endif
  #endif
END_INCLUDE_DEPENDENCIES

  #if defined IGRAPHICS_LICE
    #include "IGraphicsLice.h"
    #define IGRAPHICS_DRAW_CLASS_TYPE IGraphicsLice
    #if defined IGRAPHICS_GL || defined IGRAPHICS_METAL
      #error When using IGRAPHICS_LICE, don't define IGRAPHICS_METAL or IGRAPHICS_GL*
    #endif
  #elif defined IGRAPHICS_AGG
    #include "IGraphicsAGG.h"
    #define IGRAPHICS_DRAW_CLASS_TYPE IGraphicsAGG
    #if defined IGRAPHICS_GL || defined IGRAPHICS_METAL
      #error When using IGRAPHICS_AGG, don't define IGRAPHICS_METAL or IGRAPHICS_GL*
    #endif
  #elif defined IGRAPHICS_CAIRO
    #include "IGraphicsCairo.h"
    #define IGRAPHICS_DRAW_CLASS_TYPE IGraphicsCairo
    #if defined IGRAPHICS_GL || defined IGRAPHICS_METAL
      #error When using IGRAPHICS_CAIRO, don't define IGRAPHICS_METAL or IGRAPHICS_GL*
    #endif
  #elif defined IGRAPHICS_NANOVG
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
BEGIN_INCLUDE_DEPENDENCIES
  #include <imgui.h>
END_INCLUDE_DEPENDENCIES
#endif
