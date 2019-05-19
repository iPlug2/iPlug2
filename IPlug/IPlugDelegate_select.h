/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

/**
 * @file
 * @brief Used for choosing an editor delegate @see IEditorDelegate
 */

//#if defined PUGL_EDITOR_DELEGATE
//  #include "PUGLEditorDelegate.h"
//  using EDITOR_DELEGATE_CLASS = PUGLEditorDelegate;
//#el
#if defined UIKIT_EDITOR_DELEGATE
  #include "IPlugUIKitEditorDelegate.h"
  using EDITOR_DELEGATE_CLASS = UIKitEditorDelegate;
#elif defined NO_IGRAPHICS
  #include "IPlugEditorDelegate.h"
  using EDITOR_DELEGATE_CLASS = IEditorDelegate;
#else
  #if defined WEBSOCKET_SERVER
    #include "IWebsocketEditorDelegate.h"
    using EDITOR_DELEGATE_CLASS = IWebsocketEditorDelegate;
  #else
    #include "IGraphicsEditorDelegate.h"
    using EDITOR_DELEGATE_CLASS = IGEditorDelegate;
  #endif
#endif
