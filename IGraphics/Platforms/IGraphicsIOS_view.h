/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#import <UIKit/UIKit.h>
#include "IGraphicsIOS.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

inline CGRect ToCGRect(IGraphics* pGraphics, const IRECT& bounds)
{
  float scale = pGraphics->GetDrawScale();
  float x = floor(bounds.L * scale);
  float y = floor(bounds.T * scale);
  float x2 = ceil(bounds.R * scale);
  float y2 = ceil(bounds.B * scale);
  
  return CGRectMake(x, y, x2 - x, y2 - y);
}

inline UIColor* ToUIColor(const IColor& c)
{
  return [UIColor colorWithRed:(double) c.R / 255.0 green:(double) c.G / 255.0 blue:(double) c.B / 255.0 alpha:(double) c.A / 255.0];
}

inline IColor FromUIColor(const UIColor* c)
{
  CGFloat r,g,b,a;
  [c getRed:&r green:&g blue:&b alpha:&a];
  return IColor(a * 255., r * 255., g * 255., b * 255.);
}

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE

using namespace iplug;
using namespace igraphics;

@interface IGRAPHICS_UITABLEVC : UIViewController<UITableViewDataSource, UITableViewDelegate> // UITableViewController
{
  IPopupMenu* mMenu;
  IGraphicsIOS* mGraphics;
}
@property (strong, nonatomic) UITableView* tableView;
@property (strong, nonatomic) NSMutableArray* items;
- (id) initWithIPopupMenuAndIGraphics: (IPopupMenu*) pMenu : (IGraphicsIOS*) pGraphics;

@end

@interface IGRAPHICS_VIEW : UIView
<
UITextFieldDelegate,
UIPopoverPresentationControllerDelegate,
UIGestureRecognizerDelegate,
UITraitEnvironment,
UIDocumentPickerDelegate,
UIColorPickerViewControllerDelegate
>
{
@public
  IGraphicsIOS* mGraphics;
  IGRAPHICS_UITABLEVC* mMenuTableController;
  UINavigationController* mMenuNavigationController;
  UITextField* mTextField;
  UIAlertController* mAlertController;
  CAMetalLayer* mMTLLayer;
  int mTextFieldLength;
  IColorPickerHandlerFunc mColorPickerHandlerFunc;
  IFileDialogCompletionHandlerFunc mFileDialogFunc;
  float mPrevX, mPrevY;
}
- (id) initWithIGraphics: (IGraphicsIOS*) pGraphics;
- (BOOL) isOpaque;
- (BOOL) acceptsFirstResponder;
- (BOOL) delaysContentTouches;
- (void) removeFromSuperview;
- (IPopupMenu*) createPopupMenu: (IPopupMenu&) menu : (CGRect) bounds;
- (void) createTextEntry: (int) paramIdx : (const IText&) text : (const char*) str : (int) length : (CGRect) areaRect;
- (void) endUserInput;
- (void) showMessageBox: (const char*) str : (const char*) title : (EMsgBoxType) type : (IMsgBoxCompletionHandlerFunc) completionHandler;
- (void) promptForFile: (NSString*) fileName : (NSString*) path : (EFileAction) action : (NSArray*) contentTypes : (IFileDialogCompletionHandlerFunc) completionHandler;
- (void) promptForDirectory: (NSString*) path : (IFileDialogCompletionHandlerFunc) completionHandler;
- (BOOL) promptForColor: (IColor&) color : (const char*) str : (IColorPickerHandlerFunc) func;
- (void) presentationControllerDidDismiss: (UIPresentationController*) presentationController;

//UIDocumentPickerDelegate,
- (void) documentPicker:(UIDocumentPickerViewController*) controller didPickDocumentsAtURLs:(NSArray <NSURL *>*)urls;
- (void) documentPickerWasCancelled:(UIDocumentPickerViewController*) controller;

//UIColorPickerViewControllerDelegate
- (void) colorPickerViewControllerDidSelectColor:(UIColorPickerViewController*) viewController;
- (void) colorPickerViewControllerDidFinish:(UIColorPickerViewController*) viewController;

//gestures
- (void) attachGestureRecognizer: (EGestureType) type;
- (BOOL) gestureRecognizer:(UIGestureRecognizer*) gestureRecognizer shouldReceiveTouch:(UITouch*)touch;
- (void) onTapGesture: (UITapGestureRecognizer*) recognizer;
- (void) onLongPressGesture: (UILongPressGestureRecognizer*) recognizer;
- (void) onSwipeGesture: (UISwipeGestureRecognizer*) recognizer;
- (void) onPinchGesture: (UIPinchGestureRecognizer*) recognizer;
- (void) onRotateGesture: (UIRotationGestureRecognizer*) recognizer;

- (void) getLastTouchLocation: (float&) x : (float&) y;

- (void) traitCollectionDidChange: (UITraitCollection*) previousTraitCollection;

@property (readonly) CAMetalLayer* metalLayer;
@property (nonatomic, strong) CADisplayLink *displayLink;

@end
