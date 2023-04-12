/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#import "IPlugDialogHandlerViewIOS.h"

@implementation IPLUG_DIALOG_HANDLER_VIEW
{
}

- (void) promptForFile: (NSString*) fileName : (NSString*) path : (EFileAction) action : (NSArray*) contentTypes : (IFileDialogCompletionHandlerFunc) completionHandler
{
//  [self endUserInput];

  mFileDialogFunc = completionHandler;

  UIDocumentPickerViewController* vc = NULL;
  
  if (action == EFileAction::Open)
  {
    vc = [[UIDocumentPickerViewController alloc] initForOpeningContentTypes:contentTypes asCopy:YES];
  }
  else
  {
    NSURL* url = [[NSURL alloc] initFileURLWithPath:path];
    
    vc = [[UIDocumentPickerViewController alloc] initForExportingURLs:@[url]];
  }
  
  [vc setDelegate:self];
  
  [self.window.rootViewController presentViewController:vc animated:YES completion:nil];
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

@end

