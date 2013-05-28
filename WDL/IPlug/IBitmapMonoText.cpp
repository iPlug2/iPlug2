#include "IBitmapMonoText.h"

void DrawBitmapedText(IGraphics* pGraphics,
                      IBitmap* pTextBitmap,
                      IRECT* controlRect,
                      IText* pItext,
                      IChannelBlend* pBlend,
                      const char* str,
                      bool vCenter,
                      bool multiline,
                      int charWidth,
                      int charHeight,
                      int charOffset)
{
  if (CSTR_NOT_EMPTY(str))
  {
    int len = strlen(str);

    int basicYOffset, basicXOffset;

    if (vCenter)
      basicYOffset = controlRect->T + ((controlRect->H() - charHeight) / 2);
    else
      basicYOffset = controlRect->T;
    
    if (pItext->mAlign == IText::kAlignCenter)
      basicXOffset = controlRect->L + ((controlRect->W() - (len * charWidth)) / 2);
    else
      basicXOffset = controlRect->L + charWidth;

    int widthAsOneLine = charWidth * len;
    int lineWidth = controlRect->W() - (charWidth * 2);

    assert(lineWidth > 0);

    int nLines;
    int stridx = 0;

    int lineCount;

    if(multiline)
    {
      if (widthAsOneLine > lineWidth)
      {
        lineCount = lineWidth / charWidth;
        nLines = widthAsOneLine / lineWidth;
      }
      else// line is shorter than width of rect
      {
        lineCount = len;
        nLines = 1;
      }
    }
    else
    {
      nLines = 1;
      lineCount = lineWidth / charWidth;
    }
    //int newlines = 0;

    for(int line=0; line<=nLines; line++)
    {
      int yOffset = basicYOffset + line * charHeight;

      for(int linepos=0; linepos<lineCount; linepos++)
      {
        if (str[stridx] == '\0') return;
//        else if(str[stridx] == '\n')
//        {
//          yOffset = basicYOffset + line * charHeight;
//        }

        int frameOffset = (int) str[stridx++] - 31; // calculate which frame to look up

        int xOffset = (linepos * (charWidth + charOffset)) + basicXOffset;    // calculate xOffset for character we're drawing
        IRECT charRect = IRECT(xOffset, yOffset, xOffset + charWidth, yOffset + charHeight);
        pGraphics->DrawBitmap(pTextBitmap, &charRect, frameOffset, pBlend);
      }
    }
  }
}

void IBitmapTextControl::SetTextFromPlug(char* str)
{
  if (strcmp(mStr.Get(), str))
  {
    SetDirty(false);
    mStr.Set(str);
  }
}

bool IBitmapTextControl::Draw(IGraphics* pGraphics)
{
  char* cStr = mStr.Get();
  if (CSTR_NOT_EMPTY(cStr))
  {
    DrawBitmapedText(pGraphics, &mTextBitmap, &mRECT, &mText, &mBlend, cStr, true, false, mCharWidth, mCharHeight, mCharOffset);
  }
  return true;
}