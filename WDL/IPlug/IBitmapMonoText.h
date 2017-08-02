#ifndef _IBITMAPMONOTEXT_
#define _IBITMAPMONOTEXT_
#include "IGraphics.h"
#include "IControl.h"

void DrawBitmapedText(IGraphics* pGraphics,
                      IBitmap* pTextBitmap,
                      IRECT* controlRect,
                      IText* pItext,
                      IChannelBlend* pBlend,
                      const char* str,
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
                     IRECT pR,
                     IBitmap* pBitmap,
                     const char* str = "",
                     IText* pText = 0,
                     int charWidth = 6,
                     int charHeight = 12,
                     int charOffset = 0,
                     bool multiLine = false,
                     bool vCenter = true)
  : IControl(pPlug, pR)
  , mTextBitmap(*pBitmap)
  , mCharWidth(charWidth)
  , mCharHeight(charHeight)
  , mCharOffset(charOffset)
  , mMultiLine(multiLine)
  , mVCentre(vCenter)
  {
    if (pText)
    {
      mText = *pText;
    }

    mStr.Set(str);
  }
  
  ~IBitmapTextControl() {}

  void SetTextFromPlug(char* str);
  
  void ClearTextFromPlug()
  {
    SetTextFromPlug( (char *) "");
  }

  bool Draw(IGraphics* pGraphics);

protected:
  WDL_String mStr;
  int mCharWidth, mCharHeight, mCharOffset;
  IBitmap mTextBitmap;
  bool mMultiLine;
  bool mVCentre;
};

#endif //_IBITMAPMONOTEXT_