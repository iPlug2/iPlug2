/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#import <Cocoa/Cocoa.h>

#include "IPlugPlatformDialogs.h"
#include "IPlugPopupMenu.h"

using namespace iplug;

@interface IPLUG_MENU : NSMenu
{
  IPopupMenu* mIPopupMenu;
}

- (id) initWithIPopupMenuAndReceiver: (IPopupMenu*) pMenu : (NSView*) pView;
- (IPopupMenu*) iPopupMenu;
@end

// Used to receive menu events inline
@interface IPLUG_MENU_RCVR : NSView
{
  NSMenuItem* selectedItem;
}
- (void) onMenuSelection:(id)sender;
- (NSMenuItem*) selectedItem;
@end

@interface IPLUG_DIALOG_HANDLER_VIEW : NSView
{
}

- (void)showPopupMenuAtLocation:(IPopupMenu&) menu : (NSPoint)location: (IPopupMenuCompletionHandlerFunc) completionHandler;
@end
