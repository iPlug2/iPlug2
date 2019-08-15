#include "IPlugWebView.h"
#include "IPlug_include_in_plug_src.h"

IPlugWebView::IPlugWebView(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPrograms))
{
  GetParam(kGain)->InitDouble("Gain", 0., 0., 100.0, 0.01, "%");
  
  mEditorInitFunc = [&](){
    //LoadURL("http://localhost:3000/");
    //To load over http:// the host app needs to have NSAppTransportSecurity set to allow it, in its info.plist.
    //https://developer.apple.com/documentation/bundleresources/information_property_list/nsapptransportsecurity?language=objc
    //This is impractical for audio plugins, but perhaps viable for standalone apps
    //To server content from the app you can use IWebsocketServer which wraps Civetweb
    
    //Otherwise you can load web content into WKWebView via the filesystem, but beware, many modern toolkits like React require content to be served!
    LoadFileFromBundle("index.html");
  };
  
  MakePreset("One", 0.);
  MakePreset("Two", 10.);
  MakePreset("Three", 100.);
}

void IPlugWebView::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kGain)->Value() / 100.;
  
  sample maxVal = 0.;
  
  mOscillator.ProcessBlock(inputs[0], nFrames); // comment for audio in

  for (int s = 0; s < nFrames; s++)
  {
    sample smoothGain = mGainSmoother.Process(gain);
    outputs[0][s] = inputs[0][s] * smoothGain;
    outputs[1][s] = outputs[0][s];
    
    maxVal += std::fabs(outputs[0][s]);
  }
  
  mLastPeak = maxVal / (sample) nFrames;
}

bool IPlugWebView::OnMessage(int messageTag, int controlTag, int dataSize, const void* pData)
{
  if(messageTag == kMsgTagButton1) {
    Resize(512, 335);
  }
  else if(messageTag == kMsgTagButton2) {
    Resize(1024, 335);
  }
  else if(messageTag == kMsgTagButton3) {
    Resize(1024, 768);
  }
  
  return false;
}

void IPlugWebView::OnIdle()
{
  SendControlValueFromDelegate(kCtrlTagMeter, mLastPeak);
}
