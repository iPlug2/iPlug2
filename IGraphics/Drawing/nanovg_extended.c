
/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
 */

#include <nanovg.c>

#ifdef FONS_USE_FREETYPE
int fons_loadFontIdx(FONScontext *context, FONSttFontImpl *font, unsigned char *data, int dataSize, int faceIdx)
{
  FONS_NOTUSED(context);
  
  font->font.userdata = stash;
  FT_Error ftError = FT_New_Memory_Face(ftLibrary, (const FT_Byte*)data, dataSize, faceIdx, &font->font);
  return ftError == 0;
}
#else
int fons_loadFontIdx(FONScontext *context, FONSttFontImpl *font, unsigned char *data, int dataSize, int faceIdx)
{
  FONS_NOTUSED(dataSize);

  int offset = stbtt_GetFontOffsetForIndex(data, faceIdx);

  font->font.userdata = context;
  if (offset >= 0)
    return stbtt_InitFont(&font->font, data, offset);
  
  return 0;
}
#endif

int nvgCreateFontMemIdx(NVGcontext* ctx, const char* name, unsigned char* data, int dataSize, int faceIdx)
{
  int i, ascent, descent, fh, lineGap;
  FONScontext* stash = ctx->fs;
  
  int idx = fons__allocFont(stash);
  if (idx == FONS_INVALID)
    return FONS_INVALID;
  
  struct FONSfont*font = stash->fonts[idx];
  
  strncpy(font->name, name, sizeof(font->name));
  font->name[sizeof(font->name)-1] = '\0';
  
  // Init hash lookup.
  for (i = 0; i < FONS_HASH_LUT_SIZE; ++i)
    font->lut[i] = -1;
  
  // Read in the font data.
  font->dataSize = dataSize;
  font->data = data;
  font->freeData = 0;
  
  // Init font
  stash->nscratch = 0;
  if (!fons_loadFontIdx(stash, &font->font, data, dataSize, faceIdx)) goto error;
  
  // Store normalized line height. The real line height is got
  // by multiplying the lineh by font size.
  fons__tt_getFontVMetrics( &font->font, &ascent, &descent, &lineGap);
  fh = ascent - descent;
  font->ascender = (float)ascent / (float)fh;
  font->descender = (float)descent / (float)fh;
  font->lineh = (float)(fh + lineGap) / (float)fh;
  
  return idx;
  
error:
  fons__freeFont(font);
  stash->nfonts--;
  return FONS_INVALID;
}
