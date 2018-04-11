#ifdef NO_IGRAPHICS
  #ifdef NO_PRESETS
    #include "IPlugDelegate.h"
    typedef IDelegate IPLUG_DELEGATE;
  #else
    #include "IPlugPresetDelegate.h"
    typedef IPresetDelegate IPLUG_DELEGATE;
  #endif
#else
  #ifdef NO_PRESETS
    #include "IPlugGraphicsDelegate.h"
    typedef IDelegate IGRAPHICS_DELEGATE;
    typedef IGraphicsDelegate IPLUG_DELEGATE;
  #else
    #include "IPlugPresetDelegate.h"
    typedef IPresetDelegate IGRAPHICS_DELEGATE;
    #include "IPlugGraphicsDelegate.h"
    typedef IGRAPHICS_DELEGATE IPLUG_DELEGATE;
  #endif
#endif
