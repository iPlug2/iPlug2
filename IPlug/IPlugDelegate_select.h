#ifdef NO_IGRAPHICS
  #include "IPlugEditorDelegate.h"
  typedef IEditorDelegate EDITOR_DELEGATE_CLASS;
#else
  #include "IGraphicsEditorDelegate.h"
  typedef IGraphicsEditorDelegate EDITOR_DELEGATE_CLASS;
#endif
