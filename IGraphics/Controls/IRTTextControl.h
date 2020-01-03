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
#include "IPlugStructs.h"
#include "IPlugQueue.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** A control to display some text in the UI, driven by values in the RT audio thread */
template <typename T = double>
class IRTTextControl : public ITextControl
{
public:
  static constexpr int kUpdateMessage = 0;
  
  template <int QUEUE_SIZE = 64>
  class Sender
  {
  public:
    Sender(int ctrlTag)
    : mCtrlTag(ctrlTag)
    {
    }
    
    // this can be called on RT thread
    void SetValRT(T val)
    {
      mQueue.Push(val);
    }
    
    // this must be called on the main thread - typically in MyPlugin::OnIdle()
    void TransmitData(IEditorDelegate& dlg)
    {
      while(mQueue.ElementsAvailable())
      {
        T d;
        mQueue.Pop(d);
        dlg.SendControlMsgFromDelegate(mCtrlTag, kUpdateMessage, sizeof(T), (void*) &d);
      }
    }
    
  private:
    int mCtrlTag;
    IPlugQueue<T> mQueue {QUEUE_SIZE};
  };
  
  IRTTextControl(const IRECT& bounds, const char* fmtStr = "%f", const char* initStr = "", const IText& text = DEFAULT_TEXT, const IColor& BGColor = DEFAULT_BGCOLOR)
  : ITextControl(bounds, initStr, text, BGColor)
  , mFMTStr(fmtStr)
  {
  }
  
  void OnMsgFromDelegate(int msgTag, int dataSize, const void* pData) override
  {
    if(msgTag == kUpdateMessage && dataSize == sizeof(T))
    {
      WDL_String str;
      T* pTypedData = (T*) pData;
      str.SetFormatted(32, mFMTStr.Get(), *pTypedData);
      SetStr(str.Get());
      SetDirty(false);
    }
  }
  
protected:
  WDL_String mFMTStr;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
