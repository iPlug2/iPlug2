/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#import "IPlugDialogHandlerViewMac.h"

@implementation IPLUG_MENU_RCVR

- (NSMenuItem*) selectedItem
{
  return selectedItem;
}

- (void) onMenuSelection:(id) sender
{
  selectedItem = sender;
}

@end

@implementation IPLUG_MENU

- (id) initWithIPopupMenuAndReceiver: (IPopupMenu*) pMenu : (NSView*) pView
{
  [self initWithTitle: @""];

  NSMenuItem* nsMenuItem = nil;
  NSMutableString* nsMenuItemTitle = nil;

  [self setAutoenablesItems:NO];

  int numItems = pMenu->NItems();

  for (int i = 0; i < numItems; ++i)
  {
    IPopupMenu::Item* pMenuItem = pMenu->GetItem(i);

    nsMenuItemTitle = [[[NSMutableString alloc] initWithCString:pMenuItem->GetText() encoding:NSUTF8StringEncoding] autorelease];

    if (pMenu->GetPrefix())
    {
      NSString* prefixString = 0;

      switch (pMenu->GetPrefix())
      {
        case 0: prefixString = [NSString stringWithUTF8String:""]; break;
        case 1: prefixString = [NSString stringWithFormat:@"%1d: ", i+1]; break;
        case 2: prefixString = [NSString stringWithFormat:@"%02d: ", i+1]; break;
        case 3: prefixString = [NSString stringWithFormat:@"%03d: ", i+1]; break;
      }

      [nsMenuItemTitle insertString:prefixString atIndex:0];
    }

    if (pMenuItem->GetIsSeparator())
    {
      [self addItem:[NSMenuItem separatorItem]];
    }
    else if (pMenuItem->GetSubmenu())
    {
      nsMenuItem = [self addItemWithTitle:nsMenuItemTitle action:@selector(onMenuSelection:) keyEquivalent:@""];
      IPLUG_MENU* subMenu = [[IPLUG_MENU alloc] initWithIPopupMenuAndReceiver:pMenuItem->GetSubmenu() : pView];
      [self setSubmenu: subMenu forItem:nsMenuItem];
      [nsMenuItem setTarget:pView];
      [subMenu release];
    }
    else
    {
      nsMenuItem = [self addItemWithTitle:nsMenuItemTitle action:@selector(onMenuSelection:) keyEquivalent:@""];
      [nsMenuItem setTarget:pView];
    }
    
    if (nsMenuItem && !pMenuItem->GetIsSeparator())
    {
      [nsMenuItem setIndentationLevel:pMenuItem->GetIsTitle() ? 1 : 0 ];
      [nsMenuItem setEnabled:pMenuItem->GetEnabled() ? YES : NO];
      [nsMenuItem setState:pMenuItem->GetChecked() ? NSOnState : NSOffState];
    }
  }

  mIPopupMenu = pMenu;

  return self;
}
  
  
- (void)menuItemSelected:(NSMenuItem *)sender
{
  NSLog(@"%@ selected", sender.title);
  
  NSMenu* pChosenMenu = [sender menu];
  IPopupMenu* pIPopupMenu = [(IPLUG_MENU*) pChosenMenu iPopupMenu];

  auto chosenItemIdx = [pChosenMenu indexOfItem: sender];

  if (chosenItemIdx > -1 && pIPopupMenu)
  {
    pIPopupMenu->SetChosenItemIdx((int) chosenItemIdx);
  }
}
  
- (IPopupMenu*) iPopupMenu
{
  return mIPopupMenu;
}

@end

@implementation IPLUG_DIALOG_HANDLER_VIEW
{
}

- (void) showPopupMenuAtLocation:(IPopupMenu&) menu : (NSPoint)location : (IPopupMenuCompletionHandlerFunc)completionHandler
{
//  dispatch_async(dispatch_get_main_queue(), ^{

    IPLUG_MENU_RCVR* pReceiverView = [[[IPLUG_MENU_RCVR alloc] init] autorelease];
    IPLUG_MENU* nsMenu = [[[IPLUG_MENU alloc] initWithIPopupMenuAndReceiver:&menu: pReceiverView] autorelease];
    NSPoint flippedPoint = {location.x, self.frame.size.height - location.y};
    [nsMenu popUpMenuPositioningItem:nil atLocation:flippedPoint inView:self];
    
    NSMenuItem* pChosenItem = [pReceiverView selectedItem];
    NSMenu* pChosenMenu = [pChosenItem menu];
    IPopupMenu* pReturnMenu = [(IPLUG_MENU*) pChosenMenu iPopupMenu];

    long chosenItemIdx = [pChosenMenu indexOfItem: pChosenItem];

    if (chosenItemIdx > -1 && pReturnMenu)
    {
      pReturnMenu->SetChosenItemIdx((int) chosenItemIdx);
    }
    else
    {
      pReturnMenu = nullptr;
    }
    
    completionHandler(pReturnMenu);
//  });
}

@end

