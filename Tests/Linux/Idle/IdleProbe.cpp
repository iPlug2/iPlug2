#ifdef APP_API
#include <gtk/gtk.h>
#include "IPlugSWELL.h"
extern HWND gHWND;
#endif
#include "IdleProbe.h"
#include "IPlug_include_in_plug_src.h"
#ifndef NO_IGRAPHICS
#include "IControls.h"
#endif
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#if defined APP_API && !defined NO_IGRAPHICS
class CallbackControl final : public iplug::igraphics::IPanelControl
{
public:
  CallbackControl(LinuxIdleProbe& probe, std::thread::id thread)
  : IPanelControl(iplug::igraphics::IRECT(200, 200, 400, 400), iplug::igraphics::COLOR_WHITE)
  , mProbe(probe), mThread(thread)
  {
    mIgnoreMouse = false;
  }

  void OnMouseDown(float, float, const iplug::igraphics::IMouseMod&) override
  {
    const char* mode = getenv("IPLUG_LINUX_TEST_MODE");
    if (mode && strcmp(mode, "ui-thread") == 0)
    {
      const guint timer = g_timeout_add(30, [](gpointer data) -> gboolean {
        auto* control = static_cast<CallbackControl*>(data);
        GList* windows = gtk_window_list_toplevels();
        GtkWidget* dialog = nullptr;
        for (GList* item = windows; item; item = item->next)
          if (GTK_IS_DIALOG(item->data) && gtk_widget_get_visible(GTK_WIDGET(item->data)))
          {
            dialog = GTK_WIDGET(item->data);
            break;
          }
        g_list_free(windows);
        if (!dialog)
          return G_SOURCE_CONTINUE;
        static_cast<iplug::igraphics::IGraphicsLinux*>(control->mProbe.GetUI())->OnDisplayTimer();
        SendMessage(gHWND, WM_CLOSE, 0, 0);
        control->mDialogOnMain = std::this_thread::get_id() == control->mThread;
        gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);
        return G_SOURCE_REMOVE;
      }, this);
      const auto result = mProbe.GetUI()->ShowMessageBox("Main-thread dialog", "Linux UI probe",
        iplug::igraphics::kMB_OK, nullptr);
      if (!mDialogOnMain)
        g_source_remove(timer);
      printf("Linux UI dialog: main=%d accepted=%d\n", mDialogOnMain, result == iplug::igraphics::kOK);
    }
    if (mode && strcmp(mode, "callback-close") == 0)
    {
      mProbe.GetUI()->CloseWindow();
      mProbe.CloseWindow();
      // Nested main-loop dispatch must not complete the deferred close.
      mProbe.CloseWindowIfRequested();
    }
    printf("Linux UI callback: main=%d alive=%d\n",
           std::this_thread::get_id() == mThread, mProbe.GetUI() != nullptr);
  }

  void Draw(iplug::igraphics::IGraphics& graphics) override
  {
    if (!mReportedDraw)
    {
      printf("Linux UI draw: main=%d\n", std::this_thread::get_id() == mThread);
      mReportedDraw = true;
    }
    IPanelControl::Draw(graphics);
  }

private:
  LinuxIdleProbe& mProbe;
  std::thread::id mThread;
  bool mDialogOnMain = false;
  bool mReportedDraw = false;
};
#endif

static std::atomic<unsigned> sIdleCalls {0}, sWrongThread {0}, sMessages {0};
static std::atomic<unsigned> sOpens {0}, sCloses {0}, sCloseErrors {0};
static LinuxIdleProbe* sProbe = nullptr;

static iplug::Config ProbeConfig()
{
  auto config = iplug::MakeConfig(1, 1);
#ifdef NO_IGRAPHICS
  config.plugHasUI = false;
#endif
  return config;
}

LinuxIdleProbe::LinuxIdleProbe(const iplug::InstanceInfo& info)
: iplug::Plugin(info, ProbeConfig())
{
  sProbe = this;
  GetParam(0)->InitDouble("Probe", 0., 0., 100., 0.01);
#ifndef NO_IGRAPHICS
  mMakeGraphicsFunc = [this] {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS);
  };
  mLayoutFunc = [this](iplug::igraphics::IGraphics* pGraphics) {
    pGraphics->AttachPanelBackground(iplug::igraphics::COLOR_GRAY);
#ifdef APP_API
    pGraphics->AttachControl(new CallbackControl(*this, mMainThread));
#else
    pGraphics->AttachControl(new iplug::igraphics::IPanelControl(
      iplug::igraphics::IRECT(200, 200, 400, 400), iplug::igraphics::COLOR_WHITE));
#endif
  };
#endif
  SendParameterValueFromAPI(0, 0.75, true);
}

LinuxIdleProbe::~LinuxIdleProbe()
{
  printf("Linux idle probe stopped: idle=%u wrong-thread=%u messages=%u\n",
         sIdleCalls.load(), sWrongThread.load(), sMessages.load());
  printf("Linux close probe: opens=%u closes=%u errors=%u\n",
         sOpens.load(), sCloses.load(), sCloseErrors.load());
  sProbe = nullptr;
}

void LinuxIdleProbe::OnIdle()
{
  if (std::this_thread::get_id() != mMainThread)
    ++sWrongThread;
  if (++sIdleCalls == 1)
    printf("Linux idle probe: main=%d\n", std::this_thread::get_id() == mMainThread);
}

void LinuxIdleProbe::OnUIOpen()
{
  ++sOpens;
  iplug::Plugin::OnUIOpen();
}

void LinuxIdleProbe::OnUIClose()
{
  ++sCloses;
  if (std::this_thread::get_id() != mMainThread)
    ++sCloseErrors;
#ifndef NO_IGRAPHICS
  if (!GetUI() || !GetUI()->GetBackgroundControl())
    ++sCloseErrors;
#endif
  if (sCloses > sOpens)
    ++sCloseErrors;
}

void LinuxIdleProbe::SendParameterValueFromDelegate(int paramIdx, double value, bool normalized)
{
  if (std::this_thread::get_id() != mMainThread)
    ++sWrongThread;
  if (++sMessages == 1)
    printf("Linux idle message: main=%d\n", std::this_thread::get_id() == mMainThread);
  iplug::Plugin::SendParameterValueFromDelegate(paramIdx, value, normalized);
}

extern "C" EXPORT unsigned LinuxIdleProbeCount(unsigned kind)
{
  switch (kind)
  {
    case 0: return sIdleCalls.load();
    case 1: return sWrongThread.load();
    case 2: return sMessages.load();
    case 3: return sOpens.load();
    case 4: return sCloses.load();
    case 5: return sCloseErrors.load();
    default:
      fprintf(stderr, "Unknown Linux probe counter: %u\n", kind);
      return 0;
  }
}

extern "C" EXPORT void LinuxIdleProbeQueue()
{
  if (sProbe)
    sProbe->SendParameterValueFromAPI(0, 0.5, true);
}
