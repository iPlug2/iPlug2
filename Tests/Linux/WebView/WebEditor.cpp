#include "WebEditor.h"
#include "IPlug_include_in_plug_src.h"
#include <atomic>
#include <cstring>

static std::atomic<unsigned> sLoaded{0};
static std::atomic<unsigned> sMessages{0};
static std::atomic<unsigned> sEvaluated{0};

extern "C" unsigned LinuxWebEditorProbe(int which)
{
  return which == 0 ? sLoaded.load() : which == 1 ? sMessages.load() : sEvaluated.load();
}

LinuxWebEditor::LinuxWebEditor(const iplug::InstanceInfo& info)
: iplug::Plugin(info, iplug::MakeConfig(1, 1))
{
  GetParam(0)->InitDouble("Value", 0., 0., 1., 0.01);
  mEditorInitFunc = [this] {
    LoadFile("index.html", GetBundleID());
  };
}

void LinuxWebEditor::OnMessageFromWebView(const char* json)
{
  ++sMessages;
  iplug::WebViewEditorDelegate::OnMessageFromWebView(json);
}

void LinuxWebEditor::OnWebContentLoaded()
{
  ++sLoaded;
  iplug::WebViewEditorDelegate::OnWebContentLoaded();
  EvaluateJavaScript("40+2", [](const char* value) {
    if (value && strcmp(value, "42") == 0) ++sEvaluated;
  });
}