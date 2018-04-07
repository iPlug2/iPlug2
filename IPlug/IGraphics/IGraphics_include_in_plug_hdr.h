#pragma once

/** \file IGraphics_include_in_plug_hdr.h
 \brief IGraphics header include

 Include this file in the main header for your plugin/app using IGraphics
 */

#include "IPlugPlatform.h"
#include "config.h"

#ifndef NO_IGRAPHICS

#ifdef OS_WIN
  #include "IGraphicsWin.h"
#elif defined OS_MAC
  #include "IGraphicsMac.h"
#elif defined OS_LINUX
  #include "IGraphicsLinux.h"
#elif defined OS_WEB
  #include "IGraphicsWeb.h"
#endif

#endif // NO_IGRAPHICS
