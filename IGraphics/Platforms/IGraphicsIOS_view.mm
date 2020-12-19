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
#ifdef IGRAPHICS_IMGUI
#include "imgui.h"
#import "imgui_impl_metal.h"
#endif

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
  
  if(pItem->GetChecked())
    cell.accessoryType = UITableViewCellAccessoryCheckmark;
  else
    cell.accessoryType = pItem->GetSubmenu() ? UITableViewCellAccessoryDisclosureIndicator : UITableViewCellAccessoryNone;

  if(!pItem->GetEnabled())
  {
    cell.userInteractionEnabled = NO;
    cell.textLabel.enabled = NO;
  }
  
  return cell;
}

- (CGFloat) tableView:(UITableView*) tableView heightForRowAtIndexPath:(NSIndexPath*) indexPath
{
  int cellIndex = static_cast<int>(indexPath.row);

  IPopupMenu::Item* pItem = mMenu->GetItem(cellIndex);

  if(pItem->GetIsSeparator())
    return 0.5f;
  else
    return self.tableView.rowHeight;
}

- (void) tableView:(UITableView*) tableView didSelectRowAtIndexPath:(NSIndexPath*) indexPath
{
  int cellIndex = static_cast<int>(indexPath.row);

  IPopupMenu::Item* pItem = mMenu->GetItem(cellIndex);
  IPopupMenu* pSubMenu = pItem->GetSubmenu();
  
  if(pSubMenu)
  {
    IGRAPHICS_UITABLEVC* newViewController = [[IGRAPHICS_UITABLEVC alloc] initWithIPopupMenuAndIGraphics: pSubMenu : mGraphics];
    [newViewController setTitle:[NSString stringWithUTF8String:CStringHasContents(pSubMenu->GetRootTitle()) ? pSubMenu->GetRootTitle() : pItem->GetText()]];
    [self.navigationController pushViewController:newViewController animated:YES];
    
    return;
  }

  if(pItem->GetIsChoosable())
  {
    mMenu->SetChosenItemIdx(cellIndex);
    
    if(mMenu->GetFunction())
      mMenu->ExecFunction();
    
    mGraphics->SetControlValueAfterPopupMenu(mMenu);
    
    [self dismissViewControllerAnimated:YES completion:nil];
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

- (void)setPreferredContentSize:(CGSize)preferredContentSize
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
  
  //scrollview
  [self setContentSize:r.size];
  self.delegate = self;
  self.scrollEnabled = NO;
  
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
  
  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardWillShow:) name:UIKeyboardWillShowNotification object:nil];
  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(keyboardWillBeHidden:) name:UIKeyboardWillHideNotification object:nil];
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
  CGSize drawableSize = self.bounds.size;
  
  // Since drawable size is in pixels, we need to multiply by the scale to move from points to pixels
  drawableSize.width *= scale;
  drawableSize.height *= scale;
    
  mMTLLayer.drawableSize = drawableSize;
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

- (void) textFieldDidEndEditing:(UITextField*) textField reason:(UITextFieldDidEndEditingReason) reason
{
  if(textField == mTextField)
  {
    mGraphics->SetControlValueAfterTextEdit([[mTextField text] UTF8String]);
    mGraphics->SetAllControlsDirty();
    
    [self endUserInput];
  }
}

- (BOOL) textFieldShouldReturn:(UITextField*) textField
{
  if(textField == mTextField)
  {
    mGraphics->SetControlValueAfterTextEdit([[mTextField text] UTF8String]);
    mGraphics->SetAllControlsDirty();
    
    [self endUserInput];
  }
  return YES;
}

- (void) textFieldDidEndEditing:(UITextField*) textField
{
  [self endUserInput];
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

  mTextField = [[UITextField alloc] initWithFrame:areaRect];
  mTextFieldLength = length;
  
  CoreTextFontDescriptor* CTFontDescriptor = CoreTextHelpers::GetCTFontDescriptor(text, sFontDescriptorCache);
  UIFontDescriptor* fontDescriptor = (__bridge UIFontDescriptor*) CTFontDescriptor->GetDescriptor();
  UIFont* font = [UIFont fontWithDescriptor: fontDescriptor size: text.mSize * 0.75];
  [mTextField setFont: font];
  
  [mTextField setText:[NSString stringWithUTF8String:str]];
  [mTextField setTextColor:ToUIColor(text.mTextEntryFGColor)];
  [mTextField setBackgroundColor:ToUIColor(text.mTextEntryBGColor)];
  [mTextField setAutocorrectionType:UITextAutocorrectionTypeNo];
  [mTextField setDelegate:self];
  
  switch (text.mVAlign)
  {
    case EVAlign::Top:
      [mTextField setContentVerticalAlignment:UIControlContentVerticalAlignmentTop];
      break;
    case EVAlign::Middle:
      [mTextField setContentVerticalAlignment:UIControlContentVerticalAlignmentCenter];
      break;
    case EVAlign::Bottom:
      [mTextField setContentVerticalAlignment:UIControlContentVerticalAlignmentBottom];
      break;
    default:
      break;
  }
  
  switch (text.mAlign)
  {
    case EAlign::Near:
      [mTextField setTextAlignment: NSTextAlignmentLeft];
      break;
    case EAlign::Center:
      [mTextField setTextAlignment: NSTextAlignmentCenter];
      break;
    case EAlign::Far:
      [mTextField setTextAlignment: NSTextAlignmentRight];
      break;
    default:
      break;
  }
  
  [self addSubview: mTextField];
  [mTextField becomeFirstResponder];
}

- (void) endUserInput
{
  [self becomeFirstResponder];
  [mTextField setDelegate: nil];
  [mTextField removeFromSuperview];
  mTextField = nullptr;
}

- (void) showMessageBox: (const char*) str : (const char*) caption : (EMsgBoxType) type : (IMsgBoxCompletionHanderFunc) completionHandler
{
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
  
  [self.window.rootViewController presentViewController:alertController animated:YES completion:nil];
}

- (BOOL) promptForColor: (IColor&) color : (const char*) str : (IColorPickerHandlerFunc) func
{
#ifdef __IPHONE_14_0
  UIColorPickerViewController* colorSelectionController = [[UIColorPickerViewController alloc] init];
  
  UIUserInterfaceIdiom idiom = [[UIDevice currentDevice] userInterfaceIdiom];
  
  if(idiom == UIUserInterfaceIdiomPad)
    colorSelectionController.modalPresentationStyle = UIModalPresentationPopover;
  else
    colorSelectionController.modalPresentationStyle = UIModalPresentationPageSheet;
  
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
#endif

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
  
  auto ds = mGraphics->GetDrawScale();

  if(mGraphics->RespondsToGesture(pos.x / ds, pos.y / ds))
    return TRUE;
  else
    return FALSE;
}

- (void) keyboardWillShow:(NSNotification*) notification
{
  NSDictionary* info = [notification userInfo];
  CGSize kbSize = [[info objectForKey:UIKeyboardFrameEndUserInfoKey] CGRectValue].size;
  
  UIEdgeInsets contentInsets = UIEdgeInsetsMake(0.0, 0.0, kbSize.height, 0.0);
  self.contentInset = contentInsets;
  self.scrollIndicatorInsets = contentInsets;
  
  CGRect r = self.frame;
  r.size.height -= kbSize.height;
  
  if (!CGRectContainsPoint(r, CGPointMake(mTextField.frame.origin.x + mTextField.frame.size.width, mTextField.frame.origin.y + mTextField.frame.size.height)) ) {
    [self scrollRectToVisible:mTextField.frame animated:YES];
  }
}

- (void) keyboardWillBeHidden:(NSNotification*) notification
{
  UIEdgeInsets contentInsets = UIEdgeInsetsZero;
  self.contentInset = contentInsets;
  self.scrollIndicatorInsets = contentInsets;
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

- (void) scrollViewDidScroll:(UIScrollView*) scrollView
{
  mGraphics->SetTranslation(0, -self.contentOffset.y);
  mGraphics->SetAllControlsDirty();
}

- (void) presentationControllerDidDismiss: (UIPresentationController*) presentationController
{
  mGraphics->SetControlValueAfterPopupMenu(nullptr);
}

#ifdef __IPHONE_14_0
- (void) colorPickerViewControllerDidSelectColor:(UIColorPickerViewController*) viewController;
{
  if(mColorPickerHandlerFunc)
  {
    IColor c = FromUIColor([viewController selectedColor]);
    mColorPickerHandlerFunc(c);
  }
}

- (void) colorPickerViewControllerDidFinish:(UIColorPickerViewController*) viewController;
{
  mColorPickerHandlerFunc = nullptr;
}
#endif

- (void) getLastTouchLocation: (float&) x : (float&) y
{
  const float scale = mGraphics->GetDrawScale();
  x = mPrevX * scale;
  y = mPrevY * scale;
}

@end

#ifdef IGRAPHICS_IMGUI

@implementation IGRAPHICS_IMGUIVIEW
{
}

- (id) initWithIGraphicsView: (IGraphicsIOS_View*) pView;
{
  mView = pView;
  self = [super initWithFrame:[pView frame] device: MTLCreateSystemDefaultDevice()];
  
  if(self)
  {
    _commandQueue = [self.device newCommandQueue];
    self.layer.opaque = NO;
  }
  
  return self;
}

- (void) drawRect:(CGRect)rect
{
  id<MTLCommandBuffer> commandBuffer = [self.commandQueue commandBuffer];
  
  MTLRenderPassDescriptor *renderPassDescriptor = self.currentRenderPassDescriptor;
  if (renderPassDescriptor != nil)
  {
    renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0,0,0,0);
    
    id <MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
    [renderEncoder pushDebugGroup:@"ImGui IGraphics"];
    
    ImGui_ImplMetal_NewFrame(renderPassDescriptor);
    
    mView->mGraphics->mImGuiRenderer->DoFrame();
    
    ImDrawData *drawData = ImGui::GetDrawData();
    ImGui_ImplMetal_RenderDrawData(drawData, commandBuffer, renderEncoder);
    
    [renderEncoder popDebugGroup];
    [renderEncoder endEncoding];
    
    [commandBuffer presentDrawable:self.currentDrawable];
  }
  [commandBuffer commit];
}

@end

#endif

