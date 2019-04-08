/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#ifndef __IGRAPHICS_HDR_INC__
#define __IGRAPHICS_HDR_INC__

/**
 * @file IGraphics_include_in_plug_hdr.h
 * @brief IGraphics header include
 * Include this file in the main header if using IGraphics outside a plugin context
 */

#include "IPlugPlatform.h"

#ifndef NO_IGRAPHICS

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

#ifdef OS_WIN
  #include "IGraphicsWin.h"
#elif defined OS_MAC
  #include "IGraphicsMac.h"
#elif defined OS_IOS
  #include "IGraphicsIOS.h"
#elif defined OS_LINUX
  #include "IGraphicsLinux.h"
#elif defined OS_WEB
  #include "IGraphicsWeb.h"
#endif

#endif // NO_IGRAPHICS

#endif //__IGRAPHICS_HDR_INC__
