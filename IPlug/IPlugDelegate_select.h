#ifdef NO_IGRAPHICS
  #include "IPlugDelegate.h"
  typedef IDelegate IPLUGIN_SUPER_CLASS;
#else
  #include "IPlugGraphicsDelegate.h"
  typedef IGraphicsDelegate IPLUGIN_SUPER_CLASS;
#endif
