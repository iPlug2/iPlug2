
#include "IGraphicsStructs.h"

#include <cmath>

#include <windows.h>
#include <windowsx.h>
#include <winuser.h>

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

namespace IGraphicsWinUtilities
{
  using FontType = CoreTextFontDescriptor;
  
  // GUI Load
  
  void GUILoad() {}
  
  // Cursor Utilities
  
  void HideCursor(bool hide)
  {
    ShowCursor(!hide);
  }

  void GetCursorPosition(float& x, float& y)
  {
    POINT p;
    
    GetCursorPos(&p);
    
    x = p.x;
    y = p.y;
  }
  
  bool RepositionCursor(float x, float y)
  {
    return SetCursorPos(static_cast<int>(std::round(x)), static_cast<int>(std::round(y)));
  }

  // Mouse Modifiers
  
  IMouseMod GetMouseModifiers(bool l, bool r)
  {
    int mods = (int) [NSEvent modifierFlags];

    return IMouseMod(l, r, (mods & NSShiftKeyMask), (mods & NSControlKeyMask), (mods & NSAlternateKeyMask));
  }
  
  // Reveal
  
  bool RevealPathInExplorerOrFinder(WDL_String& path, bool select)
  {
    bool success = false;
    
    if (path.GetLength())
    {
      WCHAR winDir[IPLUG_WIN_MAX_WIDE_PATH];
      UINT len = GetSystemDirectoryW(winDir, IPLUG_WIN_MAX_WIDE_PATH);

      if (len && !(len > MAX_PATH - 2))
      {
        winDir[len]   = L'\\';
        winDir[++len] = L'\0';
        
        WDL_String explorerParams;
        
        if (select)
          explorerParams.Append("/select,");
        
        explorerParams.Append("\"");
        explorerParams.Append(path.Get());
        explorerParams.Append("\\\"");
        
        HINSTANCE result;
        
        if ((result=::ShellExecuteW(NULL, L"open", L"explorer.exe", UTF8AsUTF16(explorerParams).Get(), winDir, SW_SHOWNORMAL)) <= (HINSTANCE) 32)
          success = true;
      }
    }
    
    return success;
  }

  bool OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure)
  {
    if (confirmMsg && MessageBoxW(mPlugWnd, UTF8AsUTF16(confirmMsg).Get(), UTF8AsUTF16(msgWindowTitle).Get(), MB_YESNO) != IDYES)
    {
      return false;
    }
    DWORD inetStatus = 0;
    if (InternetGetConnectedState(&inetStatus, 0))
    {
      if (ShellExecuteW(mPlugWnd, L"open", UTF8AsUTF16(url).Get(), 0, 0, SW_SHOWNORMAL) > HINSTANCE(32))
      {
        return true;
      }
    }
    if (errMsgOnFailure)
    {
      MessageBoxW(mPlugWnd, UTF8AsUTF16(errMsgOnFailure).Get(), UTF8AsUTF16(msgWindowTitle).Get(), MB_OK);
    }
    return false;
  }
  
  // Clipboard
  
  bool GetTextFromClipboard(WDL_String& str)
  {
    bool result = false;

    if (IsClipboardFormatAvailable(CF_UNICODETEXT))
    {
      if (OpenClipboard(0))
      {
        HGLOBAL hglb = GetClipboardData(CF_UNICODETEXT);
        
        if (hglb)
        {
          WCHAR *origStr = (WCHAR*) GlobalLock(hglb);

          if (origStr)
          {
            UTF16ToUTF8(str, origStr);
            GlobalUnlock(hglb);
            result = true;
          }
        }
      }
      
      CloseClipboard();
    }
    
    if (!result)
      str.Set("");
    
    return result;
  }

  bool SetTextInClipboard(const char* str)
  {
    if (!OpenClipboard(mMainWnd))
      return false;

    EmptyClipboard();

    bool result = true;

    if (strlen(str))
    {
      // figure out how many characters we need for the wide version of this string
      const int lenWide = UTF8ToUTF16Len(str);

      // allocate global memory object for the text
      HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, lenWide*sizeof(WCHAR));
      if (!hglbCopy)
      {
        CloseClipboard();
        return false;
      }

      // lock the handle
      LPWSTR lpstrCopy = (LPWSTR) GlobalLock(hglbCopy);

      if (lpstrCopy)
      {
        // copy the string into the buffer
        UTF8ToUTF16(lpstrCopy, str, lenWide);
        GlobalUnlock(hglbCopy);

        // place the handle on the clipboard
        result = SetClipboardData(CF_UNICODETEXT, hglbCopy);

        // free the handle if unsuccessful
        if (!result)
          GlobalFree(hglbCopy);
      }
    }

    CloseClipboard();

    return result;
  }

  bool SetFilePathInClipboard(const char* path)
  {
    if (!OpenClipboard(mMainWnd))
      return false;

    EmptyClipboard();

    UTF8AsUTF16 pathWide(path);

    // N.B. GHND ensures that the memory is zeroed

    HGLOBAL hGlobal = GlobalAlloc(GHND, sizeof(DROPFILES) + (sizeof(wchar_t) * (pathWide.GetLength() + 1)));

    if (!hGlobal)
      return false;

    DROPFILES* pDropFiles = (DROPFILES*) GlobalLock(hGlobal);
    bool result = false;

    if (pDropFiles)
    {
      // Populate the dropfile structure and copy the file path

      pDropFiles->pFiles = sizeof(DROPFILES);
      pDropFiles->pt = { 0, 0 };
      pDropFiles->fNC = true;
      pDropFiles->fWide = true;

      std::copy_n(pathWide.Get(), pathWide.GetLength(), reinterpret_cast<wchar_t*>(&pDropFiles[1]));

      GlobalUnlock(hGlobal);

      result = SetClipboardData(CF_HDROP, hGlobal);
    }

    // free the handle if unsuccessful
    if (!result)
      GlobalFree(hGlobal);

    CloseClipboard();
    return result;
  }
  
  // Font Loading
  
  PlatformFontPtr LoadPlatformFont(const char* fontID, const char* fileNameOrResID, const char* bundleID, const char* sharedResourcesSubPath)
  {
    return CoreTextHelpers::LoadPlatformFont(fontID, fileNameOrResID, bundleID, sharedResourcesSubPath);
  }

  PlatformFontPtr LoadPlatformFont(const char* fontID, const char* fontName, ETextStyle style)
  {
    return CoreTextHelpers::LoadPlatformFont(fontID, fontName, style);
  }

  PlatformFontPtr LoadPlatformFont(const char* fontID, void* pData, int dataSize)
  {
    return CoreTextHelpers::LoadPlatformFont(fontID, pData, dataSize);
  }

  void CachePlatformFont(const char* fontID, const PlatformFontPtr& font, StaticStorage<CoreTextFontDescriptor>& cache)
  {
    CoreTextHelpers::CachePlatformFont(fontID, font, cache);
  }
}

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
