/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#include <windows.h>

#include "IGraphicsPrivate.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

// Fonts

class InstalledWinFont
{
public:
  InstalledWinFont(void* data, int resSize)
  : mFontHandle(nullptr)
  {
    if (data)
    {
      DWORD numFonts = 0;
      mFontHandle = AddFontMemResourceEx(data, resSize, NULL, &numFonts);
    }
  }
  
  ~InstalledWinFont()
  {
    if (IsValid())
      RemoveFontMemResourceEx(mFontHandle);
  }
  
  InstalledWinFont(const InstalledWinFont&) = delete;
  InstalledWinFont& operator=(const InstalledWinFont&) = delete;
    
  bool IsValid() const { return mFontHandle; }
  
private:
  HANDLE mFontHandle;
};

struct HFontHolder
{
  HFontHolder(HFONT hfont) : mHFont(nullptr)
  {
    LOGFONTW lFont = { 0 };
    GetObjectW(hfont, sizeof(LOGFONTW), &lFont);
    mHFont = CreateFontIndirectW(&lFont);
  }
  
  HFONT mHFont;
};

class WinFont : public PlatformFont
{
public:
  WinFont(HFONT font, const char* styleName, bool system)
  : PlatformFont(system), mFont(font), mStyleName(styleName) {}
  ~WinFont()
  {
    DeleteObject(mFont);
  }
  
  FontDescriptor GetDescriptor() override { return mFont; }
  
  IFontDataPtr GetFontData() override
  {
    HDC hdc = CreateCompatibleDC(NULL);
    IFontDataPtr fontData(new IFontData());
      
    if (hdc != NULL)
    {
      SelectObject(hdc, mFont);
      const size_t size = ::GetFontData(hdc, 0, 0, NULL, 0);

      if (size != GDI_ERROR)
      {
        fontData = std::make_unique<IFontData>(size);

        if (fontData->GetSize() == size)
        {
          size_t result = ::GetFontData(hdc, 0x66637474, 0, fontData->Get(), size);
          if (result == GDI_ERROR)
            result = ::GetFontData(hdc, 0, 0, fontData->Get(), size);
          if (result == size)
            fontData->SetFaceIdx(GetFaceIdx(fontData->Get(), fontData->GetSize(), mStyleName.Get()));
        }
      }
        
      DeleteDC(hdc);
    }

    return fontData;
  }
    
private:
  HFONT mFont;
  WDL_String mStyleName;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
