/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

#include "IPlugPlatform.h"
#include "IPlugConstants.h"
#include "IPlugPopupMenu.h"

#include <functional>

#include "wdlstring.h"

BEGIN_IPLUG_NAMESPACE

using IMsgBoxCompletionHandlerFunc = std::function<void(EMsgBoxResult result)>;
using IFileDialogCompletionHandlerFunc = std::function<void(const WDL_String& fileName, const WDL_String& path)>;
using IPopupMenuCompletionHandlerFunc = std::function<void(IPopupMenu* pMenu)>;

/** An Interface for cross platform dialogs that can be called from IGraphics, or other Editors */
class IPlatformDialogs
{
 public: 
  /** Call to hide/show the mouse cursor
   * @param hide Should the cursor be hidden or shown
   * @param lock Set \c true to hold the cursor in place while hidden */
  void HideMouseCursor(bool hide = true, bool lock = true);

  /** Force move the mouse cursor to a specific position
   * @param x New X position in pixels
   * @param y New Y position in pixels */
  void MoveMouseCursor(float x, float y);
  
  /** Pop up a modal platform message box dialog.
   * @param str The text message to display in the dialogue
   * @param caption The title of the message box window
   * @param type EMsgBoxType describing the button options available \see EMsgBoxType
   * @param completionHanlder an IMsgBoxCompletionHandlerFunc that will be called when a button is pressed
   * @return EMsgBoxResult signifying which button was pressed */
  virtual EMsgBoxResult ShowMessageBox(const char* str, const char* caption, EMsgBoxType type, IMsgBoxCompletionHandlerFunc completionHandler = nullptr);
  
  /** Create a platform file prompt dialog to choose a path for opening/saving a single file. NOTE: this method will block the main thread on macOS, unless you speficy the completionHander, which will be called asynchronously when the dialog button is pressed. On iOS, you must supply a completionHander.
   * @param fileName Non const WDL_String reference specifying the file name. Set this prior to calling the method for save dialogs, to provide a default file name. For file-open dialogs, on successful selection of a file this will get set to the file’s name.
   * @param path WDL_String reference where the path will be put on success or empty string on failure/user cancelled
   * @param action Determines whether this is an file-open dialog or a file-save dialog
   * @param ext A space separated CString list of file extensions to filter in the dialog (e.g. “.wav .aif”
   * @param completionHandler an IFileDialogCompletionHandlerFunc that will be called when a file is selected or the dialog is cancelled */
  virtual void PromptForFile(WDL_String& fileName, WDL_String& path, EFileAction action = EFileAction::Open, const char* ext = "", IFileDialogCompletionHandlerFunc completionHandler = nullptr);

  /** Create a platform file prompt dialog to choose a directory path for opening/saving a directory. NOTE: this method will block the main thread
   * @param dir Non const WDL_String reference specifying the directory path. Set this prior to calling the method for save dialogs, to provide a default path. For load dialogs, on successful selection of a directory this will get set to the full path.
   * @param completionHandler an IFileDialogCompletionHandlerFunc that will be called when a file is selected or the dialog is cancelled. Only the path argument will be populated. */
  virtual void PromptForDirectory(WDL_String& dir, IFileDialogCompletionHandlerFunc completionHandler = nullptr);
  
  /** @param path WDL_String reference where the path will be put on success or empty string on failure
   * @param select et \c true if you want to select the item in Explorer/Finder
   * @return \c true on success (if the path was valid) */
  virtual bool RevealPathInExplorerOrFinder(WDL_String& path, bool select = false);

  /** Open a URL in the platform’s default browser
   * @param url CString specifying the URL to open
   * @param msgWindowTitle \todo ?
   * @param confirmMsg \todo ?
   * @param errMsgOnFailure \todo ?
   * @return /c true on success */
  virtual bool OpenURL(const char* url, const char* msgWindowTitle = 0, const char* confirmMsg = 0, const char* errMsgOnFailure = 0);

  /** @return A CString representing the Platform API in use e.g. "macOS" */
  virtual const char* GetPlatformAPIStr() { return ""; }

  void CreatePopupMenu(IPopupMenu& menu, float x, float y, IPopupMenuCompletionHandlerFunc completionHandler = nullptr);
  
protected:
  void SetOwningView(void* pView) { mOwningView = pView; }
private:
  void GetMouseLocation(float& x, float&y) const;
  
  void StoreCursorPosition();
  /** Sets the mouse cursor to one of ECursor (implementations should return the result of the base implementation)
   * @param cursorType The cursor type
   * @return The previous cursor type so it can be restored later */
//  virtual ECursor SetMouseCursor(ECursor cursorType = ECursor::ARROW)
//  {
//    ECursor oldCursorType = mCursorType;
//    mCursorType = cursorType;
//    return oldCursorType;
//  }
private:
  void* mOwningView = nullptr;
  bool mCursorHidden = false;
  bool mCursorLock = false;
  bool mTabletInput = false;
  float mCursorX = -1.f;
  float mCursorY = -1.f;
  float mCursorLockX = -1.f;
  float mCursorLockY = -1.f;
};

END_IPLUG_NAMESPACE
