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
    int stringLength = strlen(str);

    int basicYOffset, basicXOffset;

    if (vCenter)
      basicYOffset = controlRect->T + ((controlRect->H() - charHeight) / 2);
    else
      basicYOffset = controlRect->T;
    
    if (pItext->mAlign == IText::kAlignCenter)
      basicXOffset = controlRect->L + ((controlRect->W() - (stringLength * charWidth)) / 2);
    else if (pItext->mAlign == IText::kAlignNear)
      basicXOffset = controlRect->L;
    else if (pItext->mAlign == IText::kAlignFar)
      basicXOffset = controlRect->R - (stringLength * charWidth);

    int widthAsOneLine = charWidth * stringLength;

    int nLines;
    int stridx = 0;

    int nCharsThatFitIntoLine;

    if(multiline)
    {
      if (widthAsOneLine > controlRect->W())
      {
        nCharsThatFitIntoLine = controlRect->W() / charWidth;
        nLines = (widthAsOneLine / controlRect->W()) + 1;
      }
      else // line is shorter than width of rect
      {
        nCharsThatFitIntoLine = stringLength;
        nLines = 1;
      }
    }
    else
    {
      nCharsThatFitIntoLine = controlRect->W() / charWidth;
      nLines = 1;
    }

    for(int line=0; line<nLines; line++)
    {
      int yOffset = basicYOffset + line * charHeight;

      for(int linepos=0; linepos<nCharsThatFitIntoLine; linepos++)
      {
        if (str[stridx] == '\0') return;

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
    DrawBitmapedText(pGraphics, &mTextBitmap, &mRECT, &mText, &mBlend, cStr, mVCentre, mMultiLine, mCharWidth, mCharHeight, mCharOffset);
  }
  return true;
}