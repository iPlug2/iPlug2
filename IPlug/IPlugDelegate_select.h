#ifdef NO_IGRAPHICS
//  #ifdef NO_PRESETS
//    #include "IPlugDelegate.h"
//    typedef IDelegate PLUG_DELEGATE;
//  #else
//    #include "IPlugPresetsDelegate.h"
//    typedef IPresetDelegate PLUG_DELEGATE;
//  #endif
#else
  #ifdef NO_PRESETS
//    #include "IPlugGraphicsDelegate.h"
//    typedef IDelegate IGRAPHICS_DELEGATE;
//    typedef IGraphicsDelegate PLUG_DELEGATE;
  #else
    #include "IPlugPresetsDelegate.h"
    typedef IPresetDelegate IGRAPHICS_DELEGATE;
    #include "IPlugGraphicsDelegate.h"
    #ifdef SPLIT_UI // this means that the user interface is in a separated editor class
      #define EDITOR_DELEGATE IGraphicsDelegate
      #define PLUG_DELEGATE IPresetDelegate
    #else
      #define PLUG_DELEGATE IGraphicsDelegate
    #endif
  #endif
#endif
