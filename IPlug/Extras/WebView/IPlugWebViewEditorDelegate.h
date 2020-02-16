 /*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

#include "IPlugEditorDelegate.h"
#include <functional>


BEGIN_IPLUG_NAMESPACE

/** This EditorDelegate allows using WKWebKitView for an iPlug user interface on macOS/iOS... */
class WebViewEditorDelegate : public IEditorDelegate
{
public:
  WebViewEditorDelegate(int nParams);
  virtual ~WebViewEditorDelegate();
  
  //IEditorDelegate
  void* OpenWindow(void* pParent) override;
  void CloseWindow() override;
  
  void SendControlValueFromDelegate(int ctrlTag, double normalizedValue) override;
  void SendControlMsgFromDelegate(int ctrlTag, int msgTag, int dataSize, const void* pData) override;
  void SendParameterValueFromDelegate(int paramIdx, double value, bool normalized) override;
  void SendArbitraryMsgFromDelegate(int msgTag, int dataSize, const void* pData) override;
  void Resize(int width, int height);

  //WebViewEditorDelegate
  void LoadHTML(const WDL_String& html);
  void LoadURL(const char* url);
  void LoadFileFromBundle(const char* fileName);
  void EvaluateJavaScript(const char* scriptStr);
  void EnableScroll(bool enable);
  virtual void OnWebContentLoaded() { OnUIOpen(); };
protected:
  std::function<void()> mEditorInitFunc = nullptr;

private:
  void* mWKWebView = nullptr;
  void* mWebConfig = nullptr;
  void* mScriptHandler = nullptr;
};

END_IPLUG_NAMESPACE
