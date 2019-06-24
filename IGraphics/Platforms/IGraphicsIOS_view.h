/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#import <UIKit/UIKit.h>
#include "IGraphicsIOS.h"

inline CGRect ToCGRect(IGraphics* pGraphics, const IRECT& bounds)
{
  float B = (pGraphics->Height() - bounds.B);
  return CGRectMake(bounds.L, B, bounds.W(), bounds.H());
}

@interface IGRAPHICS_POPOVER_VIEW_CONTROLLER : UIViewController<UITableViewDataSource, UITableViewDelegate>//UITableViewController
{
  IPopupMenu* mMenu;
}
@property (strong, nonatomic) UITableView* tableView;
@property (strong, nonatomic) NSMutableArray* items;
- (id) initWithIPopupMenu: (IPopupMenu&) menu;

@end

@interface IGRAPHICS_VIEW : UIView<UIPopoverPresentationControllerDelegate>
{  
@public
  IGraphicsIOS* mGraphics;
  IGRAPHICS_POPOVER_VIEW_CONTROLLER* mPopoverViewController;
}
- (id) initWithIGraphics: (IGraphicsIOS*) pGraphics;
- (BOOL) isOpaque;
- (BOOL) acceptsFirstResponder;
- (void) removeFromSuperview;
- (void) controlTextDidEndEditing: (NSNotification*) aNotification;
- (IPopupMenu*) createPopupMenu: (IPopupMenu&) menu : (CGRect) bounds;
- (void) createTextEntry: (int) paramIdx : (const IText&) text : (const char*) str : (int) length : (CGRect) areaRect;
- (void) endUserInput;
- (void) showMessageBox: (const char*) str : (const char*) caption : (EMsgBoxType) type : (IMsgBoxCompletionHanderFunc) completionHandler;
- (void) getTouchXY: (CGPoint) pt x: (float*) pX y: (float*) pY;
@property (readonly) CAMetalLayer* metalLayer;
@property (nonatomic, strong) CADisplayLink *displayLink;

@end

#ifdef IGRAPHICS_IMGUI
#import <MetalKit/MetalKit.h>

@interface IGRAPHICS_IMGUIVIEW : MTKView
{
  IGraphicsIOS_View* mView;
}
@property (nonatomic, strong) id <MTLCommandQueue> commandQueue;
- (id) initWithIGraphicsView: (IGraphicsIOS_View*) pView;
@end
#endif
