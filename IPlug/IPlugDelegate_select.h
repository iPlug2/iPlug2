#ifdef NO_IGRAPHICS
  #include "IPlugEditorDelegate.h"
  typedef IEditorDelegate IPLUGIN_SUPER_CLASS;
#else
  #include "IPlugGraphicsDelegate.h"
  typedef IGraphicsEditorDelegate IPLUGIN_SUPER_CLASS;
#endif
