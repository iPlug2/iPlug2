#pragma once

#include "IGraphics.h"
#include "IControl.h"

void DrawBitmapedText(IGraphics& graphics,
                      IBitmap& textBitmap,
                      IRECT& controlRect,
                      IText& text,
                      IChannelBlend* pBlend,
                      const char* pStr,
                      bool vCenter = true,
                      bool multiline = false,
                      int charWidth = 6,
                      int charHeight = 12,
                      int charOffset = 0);

//TODO: fix Centre/Right aligned behaviour when string exceeds bounds or should wrap onto new line

class IBitmapTextControl : public IControl
{
public:
  IBitmapTextControl(IPlugBase* pPlug,
                     IRECT rect,
                     IBitmap& bitmap,
                     const char* str = "",
                     IText* pText = 0,
                     int charWidth = 6,
                     int charHeight = 12,
                     int charOffset = 0,
                     bool multiLine = false,
                     bool vCenter = true)
  : IControl(pPlug, rect)
  , mTextBitmap(bitmap)
  , mCharWidth(charWidth)
  , mCharHeight(charHeight)
  , mCharOffset(charOffset)
  , mMultiLine(multiLine)
  , mVCentre(vCenter)
  {
    mStr.Set(str);
  }
  
  ~IBitmapTextControl() {}

  void SetTextFromPlug(char* pStr);
  
  void ClearTextFromPlug()
  {
    SetTextFromPlug( (char *) "");
  }

  bool Draw(IGraphics& graphics);

protected:
  WDL_String mStr;
  int mCharWidth, mCharHeight, mCharOffset;
  IBitmap mTextBitmap;
  bool mMultiLine;
  bool mVCentre;
};
