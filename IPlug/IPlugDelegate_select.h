#ifdef NO_IGRAPHICS
  #ifdef NO_PRESETS
    #include "IPlugPluginDelegate.h"
    typedef IPluginDelegate PLUG_DELEGATE;
  #else
    #include "IPlugPresetsDelegate.h"
    typedef IPresetsDelegate PLUG_DELEGATE;
  #endif
#else
  #ifdef NO_PRESETS
//    #include "IPlugGraphicsDelegate.h"
//    typedef IDelegate IGRAPHICS_DELEGATE;
//    typedef IGraphicsDelegate PLUG_DELEGATE;
  #else
    #include "IPlugPresetsDelegate.h"
    typedef IPresetsDelegate IGRAPHICS_DELEGATE;
    #include "IPlugGraphicsDelegate.h"
    #ifdef SPLIT_UI // this means that the user interface is in a separated editor class
      #define EDITOR_DELEGATE IGraphicsDelegate
      #define PLUG_DELEGATE IPresetsDelegate
    #else
      #define PLUG_DELEGATE IGraphicsDelegate
    #endif
  #endif
#endif
