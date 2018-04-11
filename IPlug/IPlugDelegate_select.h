#ifdef NO_IGRAPHICS
  #include "IPlugDelegate.h"
  typedef IDelegate IPLUGIN_SUPER_CLASS;
  #include "IPlugPluginDelegate.h"
  #define PLUGBASE_SUPER_CLASS IPluginDelegate
#else
  #include "IPlugGraphicsDelegate.h"
  typedef IGraphicsDelegate IPLUGIN_SUPER_CLASS;
  #include "IPlugPluginDelegate.h"
  #ifdef SPLIT_UI // this means that the user interface is in a separated editor class
    #define EDITOR_SUPER_CLASS IPluginDelegate
    #define PLUGBASE_SUPER_CLASS IPluginDelegate
  #else
    #define PLUGBASE_SUPER_CLASS IPluginDelegate
  #endif
#endif
