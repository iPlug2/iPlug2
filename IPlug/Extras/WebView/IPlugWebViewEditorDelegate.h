/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

#include "IPlugEditorDelegate.h"
#include "IPlugWebView.h"
#include "wdl_base64.h"
#include "json.hpp"
#include <functional>

BEGIN_IPLUG_NAMESPACE

/** This Editor Delegate allows using a platform native Web View as the UI for an iPlug plugin */
class WebViewEditorDelegate : public IEditorDelegate
                            , public IWebView
{
public:
  WebViewEditorDelegate(int nParams);
  virtual ~WebViewEditorDelegate();
  
  //IEditorDelegate
  void* OpenWindow(void* pParent) override;
  
  void CloseWindow() override
  {
    CloseWebView();
  }

  void SendControlValueFromDelegate(int ctrlTag, double normalizedValue) override
  {
    WDL_String str;
    str.SetFormatted(50, "SCVFD(%i, %f)", ctrlTag, normalizedValue);
    EvaluateJavaScript(str.Get());
  }

  void SendControlMsgFromDelegate(int ctrlTag, int msgTag, int dataSize, const void* pData) override
  {
    WDL_String str;
    WDL_TypedBuf<char> base64;
    int sizeOfBase64 = static_cast<int>(4. * std::ceil((static_cast<double>(dataSize)/3.)));
    base64.Resize(sizeOfBase64);
    wdl_base64encode(reinterpret_cast<const unsigned char*>(pData), base64.GetFast(), dataSize);
    str.SetFormatted(50, "SCMFD(%i, %i, %i, %s)", ctrlTag, msgTag, dataSize, base64.GetFast());
    EvaluateJavaScript(str.Get());
  }

  void SendParameterValueFromDelegate(int paramIdx, double value, bool normalized) override
  {
    WDL_String str;
    str.SetFormatted(50, "SPVFD(%i, %f)", paramIdx, value);
    EvaluateJavaScript(str.Get());
  }

  void SendArbitraryMsgFromDelegate(int msgTag, int dataSize, const void* pData) override
  {
    WDL_String str;
    WDL_TypedBuf<char> base64;
    int sizeOfBase64 = static_cast<int>(4. * std::ceil((static_cast<double>(dataSize)/3.)));
    base64.Resize(sizeOfBase64);
    wdl_base64encode(reinterpret_cast<const unsigned char*>(pData), base64.GetFast(), dataSize);
    str.SetFormatted(50, "SAMFD(%i, %i, %s)", msgTag, dataSize, base64.GetFast());
    EvaluateJavaScript(str.Get());
  }

  void OnMessageFromWebView(const char* jsonStr) override
  {
    auto json = nlohmann::json::parse(jsonStr, nullptr, false);
    
    if(json["msg"] == "SPVFUI")
    {
      SendParameterValueFromUI(json["paramIdx"], json["value"]);
    }
    else if (json["msg"] == "BPCFUI")
    {
      BeginInformHostOfParamChangeFromUI(json["paramIdx"]);
    }
    else if (json["msg"] == "EPCFUI")
    {
      EndInformHostOfParamChangeFromUI(json["paramIdx"]);
    }
    else if (json["msg"] == "SAMFUI")
    {
      SendArbitraryMsgFromUI(json["msgTag"], json["ctrlTag"], json["dataSize"], /*dataSize > 0 ? json["data"] :*/ nullptr);
    }
  }

  void Resize(int width, int height)
  {
    SetWebViewBounds(0, 0, static_cast<float>(width), static_cast<float>(height));
    EditorResizeFromUI(width, height, true);
  }

  void OnWebViewReady() override
  {
    if (mEditorInitFunc)
      mEditorInitFunc();
  }
  
  void OnWebContentLoaded() override
  {
    OnUIOpen();
  }
  
protected:
  std::function<void()> mEditorInitFunc = nullptr;
};

END_IPLUG_NAMESPACE
