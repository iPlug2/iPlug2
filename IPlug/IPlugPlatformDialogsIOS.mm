 /*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#if __has_feature(objc_arc)
#error This file must be compiled without Arc. Don't use -fobjc-arc flag!
#endif

#include "IPlugPlatformDialogs.h"
#include "IPlugUtilities.h"
#include "IPlugPaths.h"

#import <UIKit/UIKit.h>
#import <UniformTypeIdentifiers/UniformTypeIdentifiers.h>
#import "IPlugDialogHandlerViewIOS.h"

using namespace iplug;

void IPlatformDialogs::StoreCursorPosition()
{
}

void IPlatformDialogs::GetMouseLocation(float& x, float&y) const
{
}

void IPlatformDialogs::HideMouseCursor(bool hide, bool lock)
{
}

void IPlatformDialogs::MoveMouseCursor(float x, float y)
{
}

EMsgBoxResult IPlatformDialogs::ShowMessageBox(const char* str, const char* caption, EMsgBoxType type, IMsgBoxCompletionHandlerFunc completionHandler)
{
  NSString* titleNString = [NSString stringWithUTF8String:str];
  NSString* captionNString = [NSString stringWithUTF8String:caption];
  
  UIAlertController* alertController = [UIAlertController alertControllerWithTitle:titleNString message:captionNString preferredStyle:UIAlertControllerStyleAlert];
  
  void (^handlerBlock)(UIAlertAction*) =
  ^(UIAlertAction* action) {
    
    if (completionHandler != nullptr)
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
  
  if (type == kMB_OK || type == kMB_OKCANCEL)
  {
    UIAlertAction* okAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault handler:handlerBlock];
    [alertController addAction:okAction];
  }
  
  if (type == kMB_YESNO || type == kMB_YESNOCANCEL)
  {
    UIAlertAction* yesAction = [UIAlertAction actionWithTitle:@"Yes" style:UIAlertActionStyleDefault handler:handlerBlock];
    [alertController addAction:yesAction];
    
    UIAlertAction* noAction = [UIAlertAction actionWithTitle:@"No" style:UIAlertActionStyleDefault handler:handlerBlock];
    [alertController addAction:noAction];
  }
  
  if (type == kMB_RETRYCANCEL)
  {
    UIAlertAction* retryAction = [UIAlertAction actionWithTitle:@"Retry" style:UIAlertActionStyleDefault handler:handlerBlock];
    [alertController addAction:retryAction];
  }
  
  if (type == kMB_OKCANCEL || type == kMB_YESNOCANCEL || type == kMB_RETRYCANCEL)
  {
    UIAlertAction* cancelAction = [UIAlertAction actionWithTitle:@"Cancel" style:UIAlertActionStyleCancel handler:handlerBlock];
    [alertController addAction:cancelAction];
  }
  
  UIView* view = (UIView*) mOwningView;
  
  [view.window.rootViewController presentViewController:alertController animated:YES completion:nil];
  
  return EMsgBoxResult::kNoResult;
}

void IPlatformDialogs::PromptForFile(WDL_String& fileName, WDL_String& path, EFileAction action, const char* ext, IFileDialogCompletionHandlerFunc completionHandler)
{
  assert(completionHandler != nullptr && "You must provide a completion handler on iOS");
  
  NSString* pDefaultFileName = nil;
  NSString* pDefaultPath = nil;
  NSMutableArray* pFileTypes = nil;

  if (fileName.GetLength())
    pDefaultFileName = [NSString stringWithCString:fileName.Get() encoding:NSUTF8StringEncoding];
  else
    pDefaultFileName = @"";
  
  if (path.GetLength())
    pDefaultPath = [NSString stringWithCString:path.Get() encoding:NSUTF8StringEncoding];
  else
    pDefaultPath = @"";

  fileName.Set(""); // reset it

  if (CStringHasContents(ext))
  {
    pFileTypes = [[NSMutableArray alloc] init];

    NSArray* pFileExtensions = [[NSString stringWithUTF8String:ext] componentsSeparatedByString: @" "];
    
    for (NSString* pFileExtension in pFileExtensions)
    {
      UTType* pUTType = [UTType typeWithFilenameExtension:pFileExtension];
      [pFileTypes addObject:pUTType];
    }
  }
  
  [(IPLUG_DIALOG_HANDLER_VIEW*) mOwningView promptForFile: pDefaultFileName : pDefaultPath : action : pFileTypes : completionHandler];
}

void IPlatformDialogs::PromptForDirectory(WDL_String& dir, IFileDialogCompletionHandlerFunc completionHandler)
{
}

bool IPlatformDialogs::RevealPathInExplorerOrFinder(WDL_String& path, bool select)
{
  return false;
}

bool IPlatformDialogs::OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure)
{
  NSURL* pNSURL = nullptr;
  
  if (strstr(url, "http"))
    pNSURL = [NSURL URLWithString:[NSString stringWithCString:url encoding:NSUTF8StringEncoding]];
  else
    pNSURL = [NSURL fileURLWithPath:[NSString stringWithCString:url encoding:NSUTF8StringEncoding]];

  if (pNSURL)
  {
    UIResponder* pResponder = (UIResponder*) mOwningView;
    while(pResponder) {
      if ([pResponder respondsToSelector: @selector(openURL:)])
        [pResponder performSelector: @selector(openURL:) withObject: pNSURL];

      pResponder = [pResponder nextResponder];
    }
    return true;
  }
  return false;
}

void IPlatformDialogs::CreatePopupMenu(IPopupMenu& menu, float x, float y, IPopupMenuCompletionHandlerFunc completionHandler)
{
  
}
