 /*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#include "IPlugPlatformDialogs.h"
#include "IPlugUtilities.h"
#include "IPlugPaths.h"

#include <commctrl.h>
#include <Shlobj.h>
#include <wininet.h>

using namespace iplug;

float GetTotalScale() { return 1.0f; }

void IPlatformDialogs::GetMouseLocation(float& x, float&y) const
{
  POINT p;
  GetCursorPos(&p);
  ScreenToClient((HWND)mOwningView, &p);

  const float scale = GetTotalScale();

  x = p.x / scale;
  y = p.y / scale;
}

void IPlatformDialogs::HideMouseCursor(bool hide, bool lock)
{
  if (mCursorHidden == hide)
  return;
  
  if (hide)
  {
    mCursorLockX = mCursorX;
    mCursorLockY = mCursorY;
      
    ShowCursor(false);
    mCursorHidden = true;
    mCursorLock = lock && !mTabletInput;
  }
  else
  {
    if (mCursorLock)
      MoveMouseCursor(mCursorLockX, mCursorLockY);

    ShowCursor(true);
    mCursorHidden = false;
    mCursorLock = false;
  }
}

void IPlatformDialogs::MoveMouseCursor(float x, float y)
{
  if (mTabletInput)
    return;
 
  const float scale = GetTotalScale();

  POINT p;
  p.x = LONG(std::round(x * scale));
  p.y = LONG(std::round(y * scale));
  
  ::ClientToScreen((HWND)mOwningView, &p);
  
  if (SetCursorPos(p.x, p.y))
  {
    GetCursorPos(&p);
    ScreenToClient((HWND)mOwningView, &p);
    
    mCursorLockX = mCursorX = p.x / scale;
    mCursorLockY = mCursorY = p.y / scale;
  }
}

EMsgBoxResult IPlatformDialogs::ShowMessageBox(const char* str, const char* caption, EMsgBoxType type, IMsgBoxCompletionHandlerFunc completionHandler)
{ 
  auto winResult = MessageBox((HWND) mOwningView, str, caption, static_cast<int>(type));
  EMsgBoxResult result = static_cast<EMsgBoxResult>(winResult);
  
  if (completionHandler)
    completionHandler(result);
  
  return result;
}

void IPlatformDialogs::PromptForFile(WDL_String& fileName, WDL_String& path, EFileAction action, const char* ext, IFileDialogCompletionHandlerFunc completionHandler)
{    
  wchar_t fnCStr[_MAX_PATH];
  wchar_t dirCStr[_MAX_PATH];
    
  if (fileName.GetLength())
    UTF8ToUTF16(fnCStr, fileName.Get(), _MAX_PATH);
  else
    fnCStr[0] = '\0';
    
  dirCStr[0] = '\0';
    
  if (!path.GetLength())
    DesktopPath(path);
    
  UTF8ToUTF16(dirCStr, path.Get(), _MAX_PATH);
    
  OPENFILENAMEW ofn;
  memset(&ofn, 0, sizeof(OPENFILENAMEW));
    
  ofn.lStructSize = sizeof(OPENFILENAMEW);
  ofn.hwndOwner = (HWND) mOwningView;
  ofn.lpstrFile = fnCStr;
  ofn.nMaxFile = _MAX_PATH - 1;
  ofn.lpstrInitialDir = dirCStr;
  ofn.Flags = OFN_PATHMUSTEXIST;
    
  if (CStringHasContents(ext))
  {
    wchar_t extStr[256];
    wchar_t defExtStr[16];
    int i, p, n = strlen(ext);
    bool seperator = true;
        
    for (i = 0, p = 0; i < n; ++i)
    {
      if (seperator)
      {
        if (p)
          extStr[p++] = ';';
                
        seperator = false;
        extStr[p++] = '*';
        extStr[p++] = '.';
      }

      if (ext[i] == ' ')
        seperator = true;
      else
        extStr[p++] = ext[i];
    }
    extStr[p++] = '\0';
        
    wcscpy(&extStr[p], extStr);
    extStr[p + p] = '\0';
    ofn.lpstrFilter = extStr;
        
    for (i = 0, p = 0; i < n && ext[i] != ' '; ++i)
      defExtStr[p++] = ext[i];
    
    defExtStr[p++] = '\0';
    ofn.lpstrDefExt = defExtStr;
  }
    
  bool rc = false;
    
  switch (action)
  {
    case EFileAction::Save:
      ofn.Flags |= OFN_OVERWRITEPROMPT;
      rc = GetSaveFileNameW(&ofn);
      break;
    case EFileAction::Open:
      default:
      ofn.Flags |= OFN_FILEMUSTEXIST;
      rc = GetOpenFileNameW(&ofn);
      break;
  }
    
  if (rc)
  {
    char drive[_MAX_DRIVE];
    char directoryOutCStr[_MAX_PATH];
    
    WDL_String tempUTF8;
    UTF16ToUTF8(tempUTF8, ofn.lpstrFile);
    
    if (_splitpath_s(tempUTF8.Get(), drive, sizeof(drive), directoryOutCStr, sizeof(directoryOutCStr), NULL, 0, NULL, 0) == 0)
    {
      path.Set(drive);
      path.Append(directoryOutCStr);
    }
      
    fileName.Set(tempUTF8.Get());
  }
  else
  {
    fileName.Set("");
  }

  // Async is not required on windows, but call the completion handler anyway
  if (completionHandler)
  {
    completionHandler(fileName, path);
  }
}

void IPlatformDialogs::PromptForDirectory(WDL_String& dir, IFileDialogCompletionHandlerFunc completionHandler)
{
  BROWSEINFO bi;
  memset(&bi, 0, sizeof(bi));
  
  bi.ulFlags   = BIF_USENEWUI;
  bi.hwndOwner = (HWND) mOwningView;
  bi.lpszTitle = "Choose a Directory";
  
  // must call this if using BIF_USENEWUI
  ::OleInitialize(NULL);
  LPITEMIDLIST pIDL = ::SHBrowseForFolder(&bi);
  
  if (pIDL != NULL)
  {
    char buffer[_MAX_PATH] = {'\0'};
    
    if (::SHGetPathFromIDList(pIDL, buffer) != 0)
    {
      dir.Set(buffer);
      dir.Append("\\");
    }
    
    // free the item id list
    CoTaskMemFree(pIDL);
  }
  else
  {
    dir.Set("");
  }
  
  if (completionHandler)
  {
    WDL_String fileName; // not used
    completionHandler(fileName, dir);
  }
  
  ::OleUninitialize();
}

bool IPlatformDialogs::RevealPathInExplorerOrFinder(WDL_String& path, bool select)
{
  bool success = false;
  
  if (path.GetLength())
  {
    WCHAR winDir[IPLUG_WIN_MAX_WIDE_PATH];
    WCHAR explorerWide[IPLUG_WIN_MAX_WIDE_PATH];
    UINT len = GetSystemDirectoryW(winDir, IPLUG_WIN_MAX_WIDE_PATH);
    
    if (len || !(len > MAX_PATH - 2))
    {
      winDir[len]   = L'\\';
      winDir[++len] = L'\0';
      
      WDL_String explorerParams;
      
      if(select)
        explorerParams.Append("/select,");
      
      explorerParams.Append("\"");
      explorerParams.Append(path.Get());
      explorerParams.Append("\\\"");
      
      UTF8ToUTF16(explorerWide, explorerParams.Get(), IPLUG_WIN_MAX_WIDE_PATH);
      HINSTANCE result;
      
      if ((result=::ShellExecuteW(NULL, L"open", L"explorer.exe", explorerWide, winDir, SW_SHOWNORMAL)) <= (HINSTANCE) 32)
        success = true;
    }
  }
  
  return success;
}

bool IPlatformDialogs::OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure)
{
  HWND hWnd = (HWND)mOwningView;

  if (confirmMsg && MessageBox(hWnd, confirmMsg, msgWindowTitle, MB_YESNO) != IDYES)
  {
    return false;
  }
  DWORD inetStatus = 0;
  if (InternetGetConnectedState(&inetStatus, 0))
  {
    WCHAR urlWide[IPLUG_WIN_MAX_WIDE_PATH];
    UTF8ToUTF16(urlWide, url, IPLUG_WIN_MAX_WIDE_PATH);
    if (ShellExecuteW(hWnd, L"open", urlWide, 0, 0, SW_SHOWNORMAL) > HINSTANCE(32))
    {
      return true;
    }
  }
  if (errMsgOnFailure)
  {
    MessageBox(hWnd, errMsgOnFailure, msgWindowTitle, MB_OK);
  }
  return false;
}

void IPlatformDialogs::CreatePopupMenu(IPopupMenu& menu, float x, float y, IPopupMenuCompletionHandlerFunc completionHandler)
{

}