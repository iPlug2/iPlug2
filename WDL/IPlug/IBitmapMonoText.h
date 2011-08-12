#ifndef _IBITMAPMONOTEXT_
#define _IBITMAPMONOTEXT_
#include "IGraphics.h"

void DrawBitmapedText(IGraphics* pGraphics, 
                             IBitmap* pTextBitmap, 
                             IRECT* controlRect, 
                             IText* pItext, 
                             IChannelBlend* pBlend, 
                             const char* str, 
                             bool vCenter = true,
                             bool multiline = false,
                             int charWidth = 6, 
                             int charHeight = 12);


#endif //_IBITMAPMONOTEXT_