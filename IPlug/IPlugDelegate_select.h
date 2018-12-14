/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

//#if defined PUGL_EDITOR_DELEGATE
//  #include "PUGLEditorDelegate.h"
//  typedef PUGLEditorDelegate EDITOR_DELEGATE_CLASS;
//#elif defined UIKIT_EDITOR_DELEGATE
//  #include "UIKitEditorDelegate.h"
//  typedef UIKitEditorDelegate EDITOR_DELEGATE_CLASS;
#if defined NO_IGRAPHICS
  #include "IPlugEditorDelegate.h"
  typedef IEditorDelegate EDITOR_DELEGATE_CLASS;
#else
  #if defined WEBSOCKET_SERVER
    #include "IWebsocketEditorDelegate.h"
    typedef IWebsocketEditorDelegate EDITOR_DELEGATE_CLASS;
  #else
    #include "IGraphicsEditorDelegate.h"
    typedef IGEditorDelegate EDITOR_DELEGATE_CLASS;
  #endif
#endif
