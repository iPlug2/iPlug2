#pragma once

#include "IPlugEditorDelegate.h"
#include <functional>

/** This EditorDelegate allows using WKWebKitView for an iPlug user interface on macOS/iOS... */

BEGIN_IPLUG_NAMESPACE

class WebViewEditorDelegate : public IEditorDelegate
{
public:
  WebViewEditorDelegate(int nParams);
  virtual ~WebViewEditorDelegate();
  
  //IEditorDelegate
  void* OpenWindow(void* pParent) override;
  void CloseWindow() override;
  
  void SendControlValueFromDelegate(int controlTag, double normalizedValue) override;
  void SendControlMsgFromDelegate(int controlTag, int messageTag, int dataSize, const void* pData) override;
  void SendParameterValueFromDelegate(int paramIdx, double value, bool normalized) override;
  void SendArbitraryMsgFromDelegate(int messageTag, int dataSize, const void* pData) override;
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
