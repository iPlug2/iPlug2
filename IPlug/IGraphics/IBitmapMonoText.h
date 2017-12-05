#pragma once

#include "IGraphics.h"
#include "IControl.h"

static void DrawBitmapedText(IGraphics& graphics,
                      IBitmap& bitmap,
                      IRECT& rect,
                      IText& text,
                      IChannelBlend* pBlend,
                      const char* pStr,
                      bool vCenter = true,
                      bool multiline = false,
                      int charWidth = 6,
                      int charHeight = 12,
                      int charOffset = 0)
{
  if (CSTR_NOT_EMPTY(pStr))
  {
    int stringLength = (int) strlen(pStr);
    
    int basicYOffset, basicXOffset;
    
    if (vCenter)
      basicYOffset = rect.T + ((rect.H() - charHeight) / 2);
    else
      basicYOffset = rect.T;
    
    if (text.mAlign == IText::kAlignCenter)
      basicXOffset = rect.L + ((rect.W() - (stringLength * charWidth)) / 2);
    else if (text.mAlign == IText::kAlignNear)
      basicXOffset = rect.L;
    else if (text.mAlign == IText::kAlignFar)
      basicXOffset = rect.R - (stringLength * charWidth);
    
    int widthAsOneLine = charWidth * stringLength;
    
    int nLines;
    int stridx = 0;
    
    int nCharsThatFitIntoLine;
    
    if(multiline)
    {
      if (widthAsOneLine > rect.W())
      {
        nCharsThatFitIntoLine = rect.W() / charWidth;
        nLines = (widthAsOneLine / rect.W()) + 1;
      }
      else // line is shorter than width of rect
      {
        nCharsThatFitIntoLine = stringLength;
        nLines = 1;
      }
    }
    else
    {
      nCharsThatFitIntoLine = rect.W() / charWidth;
      nLines = 1;
    }
    
    for(int line=0; line<nLines; line++)
    {
      int yOffset = basicYOffset + line * charHeight;
      
      for(int linepos=0; linepos<nCharsThatFitIntoLine; linepos++)
      {
        if (pStr[stridx] == '\0') return;
        
        int frameOffset = (int) pStr[stridx++] - 31; // calculate which frame to look up
        
        int xOffset = (linepos * (charWidth + charOffset)) + basicXOffset;    // calculate xOffset for character we're drawing
        IRECT charRect = IRECT(xOffset, yOffset, xOffset + charWidth, yOffset + charHeight);
        graphics.DrawBitmap(bitmap, charRect, frameOffset, pBlend);
      }
    }
  }
}

//TODO: fix Centre/Right aligned behaviour when string exceeds bounds or should wrap onto new line

class IBitmapTextControl : public IControl
{
public:
  IBitmapTextControl(IPlugBaseGraphics& plug,
                     IRECT rect,
                     IBitmap& bitmap,
                     const char* str = "",
                     IText* text = 0,
                     int charWidth = 6,
                     int charHeight = 12,
                     int charOffset = 0,
                     bool multiLine = false,
                     bool vCenter = true)
  : IControl(plug, rect)
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

  void SetTextFromPlug(const char* str)
  {
    if (strcmp(mStr.Get(), str))
    {
      SetDirty(false);
      mStr.Set(str);
    }
  }
  
  void ClearTextFromPlug()
  {
    SetTextFromPlug("");
  }

  void Draw(IGraphics& graphics)
  {
    if (CSTR_NOT_EMPTY(mStr.Get()))
    {
      DrawBitmapedText(graphics, mTextBitmap, mRECT, mText, &mBlend, mStr.Get(), mVCentre, mMultiLine, mCharWidth, mCharHeight, mCharOffset);
    }
  }

protected:
  WDL_String mStr;
  int mCharWidth, mCharHeight, mCharOffset;
  IBitmap mTextBitmap;
  bool mMultiLine;
  bool mVCentre;
};
