/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @ingroup Controls
 * @copydoc IRTTextControl
 */

#include "IControl.h"
#include "ISender.h"
#include "IPlugStructs.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** A control to display some text in the UI, driven by values in the RT audio thread */
template <int MAXNC = 1, typename T = double>
class IRTTextControl : public ITextControl
{
public:
  IRTTextControl(const IRECT& bounds, const char* fmtStr = "%f", const char* separatorStr = ", ", const char* initStr = "", const IText& text = DEFAULT_TEXT, const IColor& BGColor = DEFAULT_BGCOLOR)
  : ITextControl(bounds, initStr, text, BGColor)
  , mFMTStr(fmtStr)
  , mSeparatorStr(separatorStr)
  {
  }
  
  void OnMsgFromDelegate(int msgTag, int dataSize, const void* pData) override
  {
    if (!IsDisabled() && msgTag == ISender<>::kUpdateMessage)
    {
      IByteStream stream(pData, dataSize);

      int pos = 0;
      ISenderData<MAXNC, T> d;
      pos = stream.Get(&d, pos);

      WDL_String str;
      
      for(int i=0; i<d.nChans-1; i++)
      {
        str.AppendFormatted(256, mFMTStr.Get(), d.vals[i]);
        str.Append(mSeparatorStr.Get());
      }
      str.AppendFormatted(256, mFMTStr.Get(), d.vals[d.nChans-1]);
      
      SetStr(str.Get());
      SetDirty(false);
    }
  }
  
protected:
  WDL_String mFMTStr;
  WDL_String mSeparatorStr;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
