/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

/**
 * @file
 * @brief Used for choosing an editor delegate @see IEditorDelegate
 * @defgroup EditorDelegates IPlug::EditorDelegates
 * EditorDelegates are base classes that handle communication between the plug-in and the user interface
 */

#if defined COCOA_EDITOR_DELEGATE
  #include "IPlugCocoaEditorDelegate.h"
  using EDITOR_DELEGATE_CLASS = iplug::CocoaEditorDelegate;
#elif defined WEBVIEW_EDITOR_DELEGATE
  #include "IPlugWebViewEditorDelegate.h"
  using EDITOR_DELEGATE_CLASS = iplug::WebViewEditorDelegate;
#elif defined NO_IGRAPHICS
  #include "IPlugEditorDelegate.h"
  using EDITOR_DELEGATE_CLASS = iplug::IEditorDelegate;
#else
  #include "IGraphicsEditorDelegate.h"
  using EDITOR_DELEGATE_CLASS = iplug::igraphics::IGEditorDelegate;
#endif
