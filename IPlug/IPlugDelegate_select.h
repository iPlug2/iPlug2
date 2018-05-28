#if defined UIKIT_EDITOR_DELEGATE
#include "UIKitEditorDelegate.h"
typedef UIKitEditorDelegate EDITOR_DELEGATE_CLASS;
#elif defined NO_IGRAPHICS
  #include "IPlugEditorDelegate.h"
  typedef IEditorDelegate EDITOR_DELEGATE_CLASS;
#else
  #include "IGraphicsEditorDelegate.h"
  typedef IGraphicsEditorDelegate EDITOR_DELEGATE_CLASS;
#endif
