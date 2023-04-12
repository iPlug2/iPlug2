/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#import <UIKit/UIKit.h>

#include "IPlugPlatformDialogs.h"

using namespace iplug;

@interface IPLUG_DIALOG_HANDLER_VIEW : UIView<UIDocumentPickerDelegate>
{
  IFileDialogCompletionHandlerFunc mFileDialogFunc;
}
- (void) promptForFile: (NSString*) fileName : (NSString*) path : (EFileAction) action : (NSArray*) contentTypes : (IFileDialogCompletionHandlerFunc) completionHandler;

// UIDocumentPickerDelegate,
- (void) documentPicker:(UIDocumentPickerViewController*) controller didPickDocumentsAtURLs:(NSArray <NSURL *>*)urls;
- (void) documentPickerWasCancelled:(UIDocumentPickerViewController*) controller;
@end
