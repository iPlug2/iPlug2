#include <gtk/gtk.h>
#include <chrono>
#include <cstdio>
#include <cstring>
#include "IGraphicsLinux.h"

using namespace iplug;
using namespace iplug::igraphics;
static unsigned sFailures = 0;

static void Check(bool result, const char* message)
{
  if (!result)
  {
    fprintf(stderr, "FAIL: %s\n", message);
    ++sFailures;
  }
}

class Delegate final : public IGEditorDelegate
{
public:
  Delegate() : IGEditorDelegate(0)
  {
    mMakeGraphicsFunc = [this]() {
      auto* graphics = new IGraphicsLinux(*this, 320, 240, 60, 1.f);
      graphics->SetHostDriven(true);
      return graphics;
    };
  }
  void BeginInformHostOfParamChangeFromUI(int) override {}
  void EndInformHostOfParamChangeFromUI(int) override {}
};

int main()
{
  if (!gtk_init_check(nullptr, nullptr))
    return 1;
  Delegate delegate;
  if (!delegate.OpenWindow(nullptr))
    return 1;
  auto* graphics = static_cast<IGraphicsLinux*>(delegate.GetUI());
  GtkClipboard* clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
  for (const char* text : {"host text", "\xe6\x97\xa5\xe6\x9c\xac\xe8\xaa\x9e", ""})
  {
    gtk_clipboard_set_text(clipboard, text, -1);
    while (gtk_events_pending())
      gtk_main_iteration_do(FALSE);
    WDL_String result("old contents");
    Check(graphics->GetTextFromClipboard(result), "paste from GTK owner on the editor's main thread");
    Check(strcmp(result.Get(), text) == 0, "GTK paste preserves UTF-8 and empty contents");
  }

  // A nonresponding owner forces the paste to enter the GTK loop. Teardown
  // requested there must remain deferred while GetTextFromClipboard is active.
  Display* host = XOpenDisplay(nullptr);
  if (!host)
    return 1;
  Window owner = XCreateSimpleWindow(host, DefaultRootWindow(host), 0, 0, 1, 1, 0, 0, 0);
  XSetSelectionOwner(host, XInternAtom(host, "CLIPBOARD", 0), owner, 0);
  XSync(host, 0);
  g_idle_add([](gpointer context) -> gboolean {
    auto& delegate = *static_cast<Delegate*>(context);
    auto* graphics = static_cast<IGraphicsLinux*>(delegate.GetUI());
    delegate.CloseWindow();
    Check(delegate.GetUI() == graphics && graphics->WindowCloseRequested(),
          "close during clipboard dispatch is deferred");
    graphics->OnDisplayTimer();
    graphics->ProcessX11Events();
    return G_SOURCE_REMOVE;
  }, &delegate);
  WDL_String result;
  const auto start = std::chrono::steady_clock::now();
  Check(!graphics->GetTextFromClipboard(result), "paste cancels when the editor closes during its wait");
  Check(std::chrono::steady_clock::now() - start < std::chrono::seconds(1), "close cancels paste promptly");
  Check(delegate.CloseWindowIfRequested() && !delegate.GetUI(), "owner completes deferred close after paste");
  XDestroyWindow(host, owner);
  XCloseDisplay(host);
  return sFailures ? 1 : 0;
}
