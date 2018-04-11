#include "IPlugPluginDelegate.h"

#ifdef NO_IGRAPHICS
  typedef IPluginDelegate PLUG_DELEGATE;
#else
  #include "IPlugGraphicsDelegate.h"
  typedef IPluginDelegate IGRAPHICS_DELEGATE;
  #ifdef SPLIT_UI // this means that the user interface is in a separated editor class
    #define EDITOR_DELEGATE IGraphicsDelegate
    #define PLUG_DELEGATE IPluginDelegate
  #else
    #define PLUG_DELEGATE IGraphicsDelegate
  #endif
#endif
