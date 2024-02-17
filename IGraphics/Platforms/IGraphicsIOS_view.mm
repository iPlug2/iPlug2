/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#if !__has_feature(objc_arc)
#error This file must be compiled with Arc. Use -fobjc-arc flag
#endif

#import <QuartzCore/QuartzCore.h>
#import <Metal/Metal.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>

#import "IGraphicsIOS_view.h"

#include "IGraphicsCoreText.h"
#include "IControl.h"
#include "IPlugParameter.h"

extern StaticStorage<CoreTextFontDescriptor> sFontDescriptorCache;

@implementation IGRAPHICS_UITABLEVC

- (void) viewDidLoad
{
  [super viewDidLoad];
  self.tableView = [[UITableView alloc] initWithFrame:self.view.frame];
  self.tableView.dataSource = self;
  self.tableView.delegate = self;
  self.tableView.scrollEnabled = YES;
  self.tableView.autoresizingMask = UIViewAutoresizingFlexibleHeight | UIViewAutoresizingFlexibleWidth;
  self.items = [[NSMutableArray alloc] init];
  
  int numItems = mMenu->NItems();

  NSMutableString* elementTitle;
  
  for (int i = 0; i < numItems; ++i)
  {
    IPopupMenu::Item* pMenuItem = mMenu->GetItem(i);

    elementTitle = [[NSMutableString alloc] initWithCString:pMenuItem->GetText() encoding:NSUTF8StringEncoding];

    if (mMenu->GetPrefix())
    {
      NSString* prefixString = nil;

      switch (mMenu->GetPrefix())
      {
        case 1: prefixString = [NSString stringWithFormat:@"%1d: ", i+1]; break;
        case 2: prefixString = [NSString stringWithFormat:@"%02d: ", i+1]; break;
        case 3: prefixString = [NSString stringWithFormat:@"%03d: ", i+1]; break;
        case 0:
        default:
          prefixString = [NSString stringWithUTF8String:""]; break;
      }

      [elementTitle insertString:prefixString atIndex:0];
    }

    [self.items addObject:elementTitle];
  }
  
  [self.view addSubview:self.tableView];
}

- (id) initWithIPopupMenuAndIGraphics:(IPopupMenu*) pMenu :(IGraphicsIOS*) pGraphics
{
  self = [super init];
  
  mGraphics = pGraphics;
  mMenu = pMenu;
  
  return self;
}

- (NSInteger) tableView:(UITableView*) tableView numberOfRowsInSection:(NSInteger) section
{
  return self.items.count;
}

- (NSInteger) numberOfSectionsInTableView:(UITableView*) tableView
{
  return 1;
}

- (UITableViewCell*) tableView:(UITableView*) tableView cellForRowAtIndexPath:(NSIndexPath*) indexPath
{
  static NSString *identifer = @"cell";
  UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:identifer];
  
  if (cell == nil)
  {
    cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:identifer];
  }
  
  int cellIndex = static_cast<int>(indexPath.row);
  
  cell.textLabel.text = [NSString stringWithFormat:@"%@", self.items[indexPath.row]];
  
  IPopupMenu::Item* pItem = mMenu->GetItem(cellIndex);
  
  if (pItem->GetChecked())
    cell.accessoryType = UITableViewCellAccessoryCheckmark;
  else
    cell.accessoryType = pItem->GetSubmenu() ? UITableViewCellAccessoryDisclosureIndicator : UITableViewCellAccessoryNone;

  cell.textLabel.enabled = cell.userInteractionEnabled = pItem->GetEnabled();
  
  return cell;
}

- (CGFloat) tableView:(UITableView*) tableView heightForRowAtIndexPath:(NSIndexPath*) indexPath
{
  int cellIndex = static_cast<int>(indexPath.row);

  IPopupMenu::Item* pItem = mMenu->GetItem(cellIndex);

  if (pItem->GetIsSeparator())
    return 0.5;
  else
    return self.tableView.rowHeight;
}

- (CGFloat)tableView:(UITableView *)tableView estimatedHeightForRowAtIndexPath:(NSIndexPath *)indexPath
{
  int cellIndex = static_cast<int>(indexPath.row);

  IPopupMenu::Item* pItem = mMenu->GetItem(cellIndex);

  if (pItem->GetIsSeparator())
    return 0.5;
  else
    return self.tableView.rowHeight;
}

- (void) tableView:(UITableView*) tableView didSelectRowAtIndexPath:(NSIndexPath*) indexPath
{
  int cellIndex = static_cast<int>(indexPath.row);

  IPopupMenu::Item* pItem = mMenu->GetItem(cellIndex);
  IPopupMenu* pSubMenu = pItem->GetSubmenu();
  
  if (pSubMenu)
  {
    IGRAPHICS_UITABLEVC* newViewController = [[IGRAPHICS_UITABLEVC alloc] initWithIPopupMenuAndIGraphics: pSubMenu : mGraphics];
    [newViewController setTitle:[NSString stringWithUTF8String:CStringHasContents(pSubMenu->GetRootTitle()) ? pSubMenu->GetRootTitle() : pItem->GetText()]];
    [self.navigationController pushViewController:newViewController animated:YES];
    
    return;
  }

  if (pItem->GetIsChoosable())
  {
    [self dismissViewControllerAnimated:YES completion:nil];

    mMenu->SetChosenItemIdx(cellIndex);
    
    if (mMenu->GetFunction())
      mMenu->ExecFunction();
    
    mGraphics->SetControlValueAfterPopupMenu(mMenu);
  }
}

- (CGSize) preferredContentSize
{
  if (self.presentingViewController && self.tableView != nil)
  {
    CGSize tempSize = self.presentingViewController.view.bounds.size;
    tempSize.width = 300;
    CGSize size = [self.tableView sizeThatFits:tempSize];
    return size;
  } else {
    return [super preferredContentSize];
  }
}

- (void) setPreferredContentSize:(CGSize)preferredContentSize
{
  super.preferredContentSize = preferredContentSize;
}

@end

@implementation IGRAPHICS_VIEW

- (id) initWithIGraphics: (IGraphicsIOS*) pGraphics
{
  TRACE

  mGraphics = pGraphics;
  CGRect r = CGRectMake(0.f, 0.f, (float) pGraphics->WindowWidth(), (float) pGraphics->WindowHeight());
  self = [super initWithFrame:r];
    
#ifdef IGRAPHICS_METAL
  mMTLLayer = [[CAMetalLayer alloc] init];
  mMTLLayer.device = MTLCreateSystemDefaultDevice();
  mMTLLayer.framebufferOnly = YES;
  mMTLLayer.frame = self.layer.frame;
  mMTLLayer.opaque = YES;
  mMTLLayer.contentsScale = [UIScreen mainScreen].scale;
  
  [self.layer addSublayer: mMTLLayer];
#endif

  self.multipleTouchEnabled = NO;
  
  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(applicationDidEnterBackgroundNotification:) name:UIApplicationDidEnterBackgroundNotification object:nil];
  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(applicationWillEnterForegroundNotification:) name:UIApplicationWillEnterForegroundNotification object:nil];
  mColorPickerHandlerFunc = nullptr;
  
  return self;
}

- (void) setFrame:(CGRect) frame
{
  [super setFrame:frame];
  
  // During the first layout pass, we will not be in a view hierarchy, so we guess our scale
  CGFloat scale = [UIScreen mainScreen].scale;
  
  // If we've moved to a window by the time our frame is being set, we can take its scale as our own
  if (self.window)
    scale = self.window.screen.scale;
  
  #ifdef IGRAPHICS_METAL
  [CATransaction begin];
  [CATransaction setValue:(id)kCFBooleanTrue forKey:kCATransactionDisableActions];
  CGSize drawableSize = self.bounds.size;
  [self.layer setFrame:frame];
  mMTLLayer.frame = self.layer.frame;

  drawableSize.width *= scale;
  drawableSize.height *= scale;

  mMTLLayer.drawableSize = drawableSize;
  
  [CATransaction commit];
  #endif
}

- (void) onTouchEvent:(ETouchEvent) eventType withTouches:(NSSet*) touches withEvent:(UIEvent*) event
{
  if(mGraphics == nullptr) //TODO: why?
    return;
  
  NSEnumerator* pEnumerator = [[event allTouches] objectEnumerator];
  UITouch* pTouch;
  
  std::vector<IMouseInfo> points;

  while ((pTouch = [pEnumerator nextObject]))
  {
    CGPoint pos = [pTouch locationInView:pTouch.view];
    
    IMouseInfo point;
    
    auto ds = mGraphics->GetDrawScale();
    
    point.ms.L = true;
    point.ms.touchID = reinterpret_cast<ITouchID>(pTouch);
    point.ms.touchRadius = [pTouch majorRadius];
  
    point.x = pos.x / ds;
    point.y = pos.y / ds;
    CGPoint posPrev = [pTouch previousLocationInView: self];
    point.dX = (pos.x - posPrev.x) / ds;
    point.dY = (pos.y - posPrev.y) / ds;
    
    if([touches containsObject:pTouch])
    {
      mPrevX = point.x;
      mPrevY = point.y;
      points.push_back(point);
    }
  }

//  DBGMSG("%lu\n", points[0].ms.idx);
  
  if(eventType == ETouchEvent::Began)
    mGraphics->OnMouseDown(points);
  
  if(eventType == ETouchEvent::Moved)
    mGraphics->OnMouseDrag(points);
  
  if(eventType == ETouchEvent::Ended)
    mGraphics->OnMouseUp(points);
  
  if(eventType == ETouchEvent::Cancelled)
    mGraphics->OnTouchCancelled(points);
}

- (void) touchesBegan:(NSSet*) touches withEvent:(UIEvent*) event
{
  [self onTouchEvent:ETouchEvent::Began withTouches:touches withEvent:event];
}

- (void) touchesMoved:(NSSet*) touches withEvent:(UIEvent*) event
{
  [self onTouchEvent:ETouchEvent::Moved withTouches:touches withEvent:event];
}

- (void) touchesEnded:(NSSet*) touches withEvent:(UIEvent*) event
{
  [self onTouchEvent:ETouchEvent::Ended withTouches:touches withEvent:event];
}

- (void) touchesCancelled:(NSSet*) touches withEvent:(UIEvent*) event
{
  [self onTouchEvent:ETouchEvent::Cancelled withTouches:touches withEvent:event];
}

- (CAMetalLayer*) metalLayer
{
  return mMTLLayer;
}

- (void) didMoveToSuperview
{
  [super didMoveToSuperview];
  if (self.superview)
  {
    self.displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(redraw:)];
    [self.displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
    self.displayLink.preferredFramesPerSecond = mGraphics->FPS();
  }
  else
  {
    [self.displayLink invalidate];
    self.displayLink = nil;
  }
}

- (void) drawRect:(CGRect)rect
{
  IRECTList rects;
  
  if(mGraphics)
  {
    mGraphics->SetPlatformContext(UIGraphicsGetCurrentContext());
    
    if (mGraphics->IsDirty(rects))
    {
      mGraphics->SetAllControlsClean();
      mGraphics->Draw(rects);
    }
  }
}

- (void) redraw:(CADisplayLink*) displayLink
{
#ifdef IGRAPHICS_CPU
  [self setNeedsDisplay];
#else
  [self drawRect:CGRect()];
#endif
}

- (BOOL) isOpaque
{
  return YES;
}

- (BOOL) acceptsFirstResponder
{
  return YES;
}

- (BOOL) canBecomeFirstResponder
{
  return YES;
}

- (void) removeFromSuperview
{
  [self.displayLink invalidate];
  self.displayLink = nil;
  mTextField = nil;
  mGraphics = nil;
  mMenuTableController = nil;
  mMenuNavigationController = nil;
  [mMTLLayer removeFromSuperlayer];
  mMTLLayer = nil;
}

- (BOOL) textFieldShouldReturn:(UITextField*) textField
{
  if (textField == mTextField)
  {
    mGraphics->SetControlValueAfterTextEdit([[mTextField text] UTF8String]);
    mGraphics->SetAllControlsDirty();
    
    [self endUserInput];
  }
  return YES;
}

- (BOOL) textField:(UITextField*) textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString*) string
{
  if (!string.length)
    return YES;
  
  // verify max length has not been exceeded
  NSString* proposedText = [mTextField.text stringByReplacingCharactersInRange:range withString:string];
  
  if (proposedText.length > mTextFieldLength)
    return NO;
  
  IControl* pInTextEntry = mGraphics->GetControlInTextEntry();
  
  if(pInTextEntry)
  {
    const IParam* pParam = pInTextEntry->GetParam();
    
    if (pParam)
    {
      NSMutableCharacterSet *characterSet = [[NSMutableCharacterSet alloc] init];
      
      switch ( pParam->Type() )
      {
        case IParam::kTypeEnum:
        case IParam::kTypeInt:
        case IParam::kTypeBool:
          [characterSet addCharactersInString:@"0123456789-+"];
          break;
        case IParam::kTypeDouble:
          [characterSet addCharactersInString:@"0123456789.-+"];
          break;
        default:
          break;
      }
      
      if ([string rangeOfCharacterFromSet:characterSet.invertedSet].location != NSNotFound)
        return NO;
    }
  }
  
  return YES;
}

- (UIModalPresentationStyle) adaptivePresentationStyleForPresentationController:(UIPresentationController*) controller
{
  return UIModalPresentationNone;
}

- (BOOL) presentationControllerShouldDismiss:(UIPopoverPresentationController*) popoverPresentationController
{
  return YES;
}

- (IPopupMenu*) createPopupMenu: (IPopupMenu&) menu : (CGRect) bounds;
{
  mMenuTableController = [[IGRAPHICS_UITABLEVC alloc] initWithIPopupMenuAndIGraphics:&menu : mGraphics];
  [mMenuTableController setTitle: [NSString stringWithUTF8String:menu.GetRootTitle()]];

  mMenuNavigationController = [[UINavigationController alloc] initWithRootViewController:mMenuTableController];

  mMenuNavigationController.modalPresentationStyle = UIModalPresentationPopover;
  mMenuNavigationController.popoverPresentationController.sourceView = self;
  mMenuNavigationController.popoverPresentationController.sourceRect = bounds;
//  mMenuNavigationController.popoverPresentationController.permittedArrowDirections = UIPopoverArrowDirectionUp;
  mMenuNavigationController.popoverPresentationController.delegate = self;

  [self.window.rootViewController presentViewController:mMenuNavigationController animated:YES completion:nil];
  
  return nullptr;
}

- (void) createTextEntry: (int) paramIdx : (const IText&) text : (const char*) str : (int) length : (CGRect) areaRect
{
  if (mTextField)
    return;
  
  mAlertController = [UIAlertController alertControllerWithTitle:@"Input a value:" message:@"" preferredStyle:UIAlertControllerStyleAlert];

  __weak IGRAPHICS_VIEW* weakSelf = self;

  void (^cancelHandler)(UIAlertAction*) = ^(UIAlertAction *action)
  {
    __strong IGRAPHICS_VIEW* strongSelf = weakSelf;
    strongSelf->mGraphics->SetAllControlsDirty();
    [strongSelf endUserInput];
  };
    
  UIAlertAction* cancelAction = [UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleDefault handler:cancelHandler];
  [mAlertController addAction:cancelAction];

  void (^okHandler)(UIAlertAction*) = ^(UIAlertAction *action)
  {
    __strong IGRAPHICS_VIEW* strongSelf = weakSelf;
    strongSelf->mGraphics->SetControlValueAfterTextEdit([[strongSelf->mTextField text] UTF8String]);
    strongSelf->mGraphics->SetAllControlsDirty();
    [strongSelf endUserInput];
  };
    
  UIAlertAction* okAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:okHandler];
  [mAlertController addAction:okAction];
  [mAlertController setPreferredAction:okAction];
    
  [mAlertController addTextFieldWithConfigurationHandler:^(UITextField* aTextField) {
    __strong IGRAPHICS_VIEW* strongSelf = weakSelf;
    strongSelf->mTextField = aTextField;
    strongSelf->mTextFieldLength = length;
    aTextField.delegate = strongSelf;
    [aTextField setText:[NSString stringWithUTF8String:str]];
  }];
  [self.window.rootViewController presentViewController:mAlertController animated:YES completion:nil];
}

- (void) endUserInput
{
  [self becomeFirstResponder];
  [self.window.rootViewController dismissViewControllerAnimated:NO completion:nil];
  [mTextField setDelegate: nil];
  mAlertController = nullptr;
  mTextField = nullptr;
  mGraphics->ClearInTextEntryControl();
}

- (void) showMessageBox: (const char*) str : (const char*) caption : (EMsgBoxType) type : (IMsgBoxCompletionHandlerFunc) completionHandler
{
  [self endUserInput];

  NSString* titleNString = [NSString stringWithUTF8String:str];
  NSString* captionNString = [NSString stringWithUTF8String:caption];
  
  UIAlertController* alertController = [UIAlertController alertControllerWithTitle:titleNString message:captionNString preferredStyle:UIAlertControllerStyleAlert];
  
  void (^handlerBlock)(UIAlertAction*) =
  ^(UIAlertAction* action) {
    
    if(completionHandler != nullptr)
    {
      EMsgBoxResult result = EMsgBoxResult::kCANCEL;
      
      if([action.title isEqualToString:@"OK"])
        result = EMsgBoxResult::kOK;
      if([action.title isEqualToString:@"Cancel"])
        result = EMsgBoxResult::kCANCEL;
      if([action.title isEqualToString:@"Yes"])
        result = EMsgBoxResult::kYES;
      if([action.title isEqualToString:@"No"])
        result = EMsgBoxResult::kNO;
      if([action.title isEqualToString:@"Retry"])
        result = EMsgBoxResult::kRETRY;
      
      completionHandler(result);
    }
    
  };
  
  if(type == kMB_OK || type == kMB_OKCANCEL)
  {
    UIAlertAction* okAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:handlerBlock];
    [alertController addAction:okAction];
  }
  
  if(type == kMB_YESNO || type == kMB_YESNOCANCEL)
  {
    UIAlertAction* yesAction = [UIAlertAction actionWithTitle:@"Yes" style:UIAlertActionStyleDefault handler:handlerBlock];
    [alertController addAction:yesAction];
    
    UIAlertAction* noAction = [UIAlertAction actionWithTitle:@"No" style:UIAlertActionStyleDefault handler:handlerBlock];
    [alertController addAction:noAction];
  }
  
  if(type == kMB_RETRYCANCEL)
  {
    UIAlertAction* retryAction = [UIAlertAction actionWithTitle:@"Retry" style:UIAlertActionStyleDefault handler:handlerBlock];
    [alertController addAction:retryAction];
  }
  
  if(type == kMB_OKCANCEL || type == kMB_YESNOCANCEL || type == kMB_RETRYCANCEL)
  {
    UIAlertAction* cancelAction = [UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:handlerBlock];
    [alertController addAction:cancelAction];
  }
  
  [[NSOperationQueue mainQueue] addOperationWithBlock:^{
    [self.window.rootViewController presentViewController:alertController animated:YES completion:nil];
  }];
}

- (void) promptForFile: (NSString*) fileName : (NSString*) path : (EFileAction) action : (NSArray*) contentTypes : (IFileDialogCompletionHandlerFunc) completionHandler
{
  [self endUserInput];

  mFileDialogFunc = completionHandler;

  UIDocumentPickerViewController* vc = NULL;
  NSURL* url = [[NSURL alloc] initFileURLWithPath:path];

  if (action == EFileAction::Open)
  {
    vc = [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:contentTypes asCopy:YES];
    [vc setDirectoryURL:url];
  }
  else
  {
    vc = [[UIDocumentPickerViewController alloc] initForExportingURLs:@[url]];
  }
  
  [vc setDelegate:self];
  
  [self.window.rootViewController presentViewController:vc animated:YES completion:nil];
}

- (void) promptForDirectory: (NSString*) path : (IFileDialogCompletionHandlerFunc) completionHandler
{
  [self endUserInput];

  mFileDialogFunc = completionHandler;

  UIDocumentPickerViewController* vc = NULL;
  NSURL* url = [[NSURL alloc] initFileURLWithPath:path];

  NSMutableArray* pFileTypes = [[NSMutableArray alloc] init];
  UTType* directoryType = [UTType typeWithIdentifier:@"public.folder"];
  [pFileTypes addObject:directoryType];
  
  vc = [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:pFileTypes];
  [vc setDirectoryURL:url];

  [vc setDelegate:self];
  
  [self.window.rootViewController presentViewController:vc animated:YES completion:nil];
}

- (BOOL) promptForColor: (IColor&) color : (const char*) str : (IColorPickerHandlerFunc) func
{
  [self endUserInput];

  UIColorPickerViewController* colorSelectionController = [[UIColorPickerViewController alloc] init];
  
  colorSelectionController.modalPresentationStyle = UIModalPresentationPopover;
  colorSelectionController.popoverPresentationController.delegate = self;
  colorSelectionController.popoverPresentationController.sourceView = self;
  
  float x, y;
  mGraphics->GetMouseLocation(x, y);
  colorSelectionController.popoverPresentationController.sourceRect = CGRectMake(x, y, 1, 1);
  
  colorSelectionController.delegate = self;
  colorSelectionController.selectedColor = ToUIColor(color);
  colorSelectionController.supportsAlpha = YES;
  
  mColorPickerHandlerFunc = func;
  
  [self.window.rootViewController presentViewController:colorSelectionController animated:YES completion:nil];

  return false;
}

- (void) attachGestureRecognizer: (EGestureType) type
{
  UIGestureRecognizer* gestureRecognizer;
  
  switch (type)
  {
    case EGestureType::DoubleTap:
    case EGestureType::TripleTap:
    {
      gestureRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(onTapGesture:)];
      [(UITapGestureRecognizer*) gestureRecognizer setNumberOfTapsRequired: type == EGestureType::DoubleTap ? 2 : 3];
      [(UITapGestureRecognizer*) gestureRecognizer setNumberOfTouchesRequired:1];
      break;
    }
    case EGestureType::LongPress1:
    case EGestureType::LongPress2:
    {
      gestureRecognizer = [[UILongPressGestureRecognizer alloc] initWithTarget:self action:@selector(onLongPressGesture:)];
      [(UILongPressGestureRecognizer*) gestureRecognizer setNumberOfTouchesRequired: type == EGestureType::LongPress1 ? 1 : 2];
      break;
    }
    case EGestureType::SwipeLeft:
    {
      gestureRecognizer = [[UISwipeGestureRecognizer alloc] initWithTarget:self action:@selector(onSwipeGesture:)];
      [(UISwipeGestureRecognizer*) gestureRecognizer setDirection:UISwipeGestureRecognizerDirectionLeft];
      break;
    }
    case EGestureType::SwipeRight:
    {
      gestureRecognizer = [[UISwipeGestureRecognizer alloc] initWithTarget:self action:@selector(onSwipeGesture:)];
      [(UISwipeGestureRecognizer*) gestureRecognizer setDirection:UISwipeGestureRecognizerDirectionRight];
      break;
    }
    case EGestureType::SwipeUp:
    {
      gestureRecognizer = [[UISwipeGestureRecognizer alloc] initWithTarget:self action:@selector(onSwipeGesture:)];
      [(UISwipeGestureRecognizer*) gestureRecognizer setDirection:UISwipeGestureRecognizerDirectionUp];
      break;
    }
    case EGestureType::SwipeDown:
    {
      gestureRecognizer = [[UISwipeGestureRecognizer alloc] initWithTarget:self action:@selector(onSwipeGesture:)];
      [(UISwipeGestureRecognizer*) gestureRecognizer setDirection:UISwipeGestureRecognizerDirectionDown];
      break;
    }
    case EGestureType::Pinch:
    {
      gestureRecognizer = [[UIPinchGestureRecognizer alloc] initWithTarget:self action:@selector(onPinchGesture:)];
      break;
    }
    case EGestureType::Rotate:
    {
      gestureRecognizer = [[UIRotationGestureRecognizer alloc] initWithTarget:self action:@selector(onRotateGesture:)];
      break;
    }
    default:
      return;
  }
    
  gestureRecognizer.delegate = self;
  gestureRecognizer.cancelsTouchesInView = YES;
  gestureRecognizer.delaysTouchesBegan = YES;
  [self addGestureRecognizer:gestureRecognizer];
}

- (void) onTapGesture: (UITapGestureRecognizer*) recognizer
{
  CGPoint p = [recognizer locationInView:self];
  auto ds = mGraphics->GetDrawScale();
  IGestureInfo info;
  info.x = p.x / ds;
  info.y = p.y / ds;
  info.type = recognizer.numberOfTapsRequired == 2 ? EGestureType::DoubleTap : EGestureType::TripleTap;
  
  mGraphics->OnGestureRecognized(info);
}

- (void) onLongPressGesture: (UILongPressGestureRecognizer*) recognizer
{
  CGPoint p = [recognizer locationInView:self];
  auto ds = mGraphics->GetDrawScale();
  IGestureInfo info;
  info.x = p.x / ds;
  info.y = p.y / ds;
  if(recognizer.state == UIGestureRecognizerStateBegan)
    info.state = EGestureState::Began;
  else if(recognizer.state == UIGestureRecognizerStateChanged)
    info.state = EGestureState::InProcess;
  else if(recognizer.state == UIGestureRecognizerStateEnded)
    info.state = EGestureState::Ended;
  
  info.type = recognizer.numberOfTouchesRequired == 1 ? EGestureType::LongPress1 : EGestureType::LongPress2;
  
  mGraphics->OnGestureRecognized(info);
}

- (void) onSwipeGesture: (UISwipeGestureRecognizer*) recognizer
{
  CGPoint p = [recognizer locationInView:self];
  auto ds = mGraphics->GetDrawScale();
  IGestureInfo info;
  info.x = p.x / ds;
  info.y = p.y / ds;

  switch (recognizer.direction) {
    case UISwipeGestureRecognizerDirectionLeft: info.type = EGestureType::SwipeLeft; break;
    case UISwipeGestureRecognizerDirectionRight: info.type = EGestureType::SwipeRight; break;
    case UISwipeGestureRecognizerDirectionUp: info.type = EGestureType::SwipeUp; break;
    case UISwipeGestureRecognizerDirectionDown: info.type = EGestureType::SwipeDown; break;
    default:
      break;
  }
  
  mGraphics->OnGestureRecognized(info);
}

- (void) onPinchGesture: (UIPinchGestureRecognizer*) recognizer
{
  CGPoint p = [recognizer locationInView:self];
  auto ds = mGraphics->GetDrawScale();
  IGestureInfo info;
  info.x = p.x / ds;
  info.y = p.y / ds;
  info.velocity = recognizer.velocity;
  info.scale = recognizer.scale;
  
  if(recognizer.state == UIGestureRecognizerStateBegan)
    info.state = EGestureState::Began;
  else if(recognizer.state == UIGestureRecognizerStateChanged)
    info.state = EGestureState::InProcess;
  else if(recognizer.state == UIGestureRecognizerStateEnded)
    info.state = EGestureState::Ended;
  
  info.type = EGestureType::Pinch;
  
  mGraphics->OnGestureRecognized(info);
}

- (void) onRotateGesture: (UIRotationGestureRecognizer*) recognizer
{
  CGPoint p = [recognizer locationInView:self];
  auto ds = mGraphics->GetDrawScale();
  IGestureInfo info;
  info.x = p.x / ds;
  info.y = p.y / ds;
  info.velocity = recognizer.velocity;
  info.angle = RadToDeg(recognizer.rotation);
  
  if(recognizer.state == UIGestureRecognizerStateBegan)
    info.state = EGestureState::Began;
  else if(recognizer.state == UIGestureRecognizerStateChanged)
    info.state = EGestureState::InProcess;
  else if(recognizer.state == UIGestureRecognizerStateEnded)
    info.state = EGestureState::Ended;
  
  info.type = EGestureType::Rotate;

  mGraphics->OnGestureRecognized(info);
}

-(BOOL) gestureRecognizer:(UIGestureRecognizer*) gestureRecognizer shouldReceiveTouch:(UITouch*) touch
{
  CGPoint pos = [touch locationInView:touch.view];
  
  if (mGraphics)
  {
    auto ds = mGraphics->GetDrawScale();
    
    if (mGraphics->RespondsToGesture(pos.x / ds, pos.y / ds))
    {
      return TRUE;
    }
  }
  
  return FALSE;
}

- (void) applicationDidEnterBackgroundNotification:(NSNotification*) notification
{
  [self.displayLink setPaused:YES];
}

- (void) applicationWillEnterForegroundNotification:(NSNotification*) notification
{
  [self.displayLink setPaused:NO];
}

- (BOOL) delaysContentTouches
{
  return NO;
}

- (void) presentationControllerDidDismiss: (UIPresentationController*) presentationController
{
  mGraphics->SetControlValueAfterPopupMenu(nullptr);
}

- (void) documentPicker:(UIDocumentPickerViewController*) controller didPickDocumentsAtURLs:(NSArray <NSURL*>*) urls
{
  WDL_String fileName, path;
  
  if (urls.count == 1)
  {
    NSURL* pSource = urls[0];
    NSString* pFullPath = [pSource path];
    fileName.Set([pFullPath UTF8String]);
    
    NSString* pTruncatedPath = [pFullPath stringByDeletingLastPathComponent];

    if (pTruncatedPath)
    {
      path.Set([pTruncatedPath UTF8String]);
      path.Append("/");
    }

    if (mFileDialogFunc)
      mFileDialogFunc(fileName, path);
  }
  else
  {
    // call with empty values
    if (mFileDialogFunc)
      mFileDialogFunc(fileName, path);
  }
}

- (void) documentPickerWasCancelled:(UIDocumentPickerViewController*) controller
{
  WDL_String fileName, path;
  
  if (mFileDialogFunc)
    mFileDialogFunc(fileName, path);
}

- (void) colorPickerViewControllerDidSelectColor:(UIColorPickerViewController*) viewController;
{
  if (mColorPickerHandlerFunc)
  {
    IColor c = FromUIColor([viewController selectedColor]);
    mColorPickerHandlerFunc(c);
  }
}

- (void) colorPickerViewControllerDidFinish:(UIColorPickerViewController*) viewController;
{
  mColorPickerHandlerFunc = nullptr;
}

- (void) traitCollectionDidChange: (UITraitCollection*) previousTraitCollection
{
  [super traitCollectionDidChange: previousTraitCollection];

  if(mGraphics)
  {
    mGraphics->OnAppearanceChanged([self.traitCollection userInterfaceStyle] == UIUserInterfaceStyleDark ? EUIAppearance::Dark
                                                                                                         : EUIAppearance::Light);
  }
}

- (void) getLastTouchLocation: (float&) x : (float&) y
{
  const float scale = mGraphics->GetDrawScale();
  x = mPrevX * scale;
  y = mPrevY * scale;
}

@end

