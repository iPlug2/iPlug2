#ifndef __IGRAPHICS_HDR_INC__
#define __IGRAPHICS_HDR_INC__
/** \file IGraphics_include_in_plug_hdr.h
 \brief IGraphics header include

 Include this file in the main header for your plugin/app using IGraphics
 */

#include "IPlugPlatform.h"

#ifndef NO_IGRAPHICS

#ifdef IGRAPHICS_FREETYPE
  #define FONS_USE_FREETYPE
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
