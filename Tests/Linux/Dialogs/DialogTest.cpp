#include <gtk/gtk.h>
#include <dlfcn.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "IGraphicsLinux.h"

using namespace iplug;
using namespace iplug::igraphics;

namespace {

bool sHideGTK = false;
int sFailures = 0;

void Check(bool condition, const char* message)
{
  if (!condition)
  {
    fprintf(stderr, "FAIL: %s\n", message);
    ++sFailures;
  }
}

class DialogDelegate final : public IGEditorDelegate
{
public:
  DialogDelegate() : IGEditorDelegate(0) {}
  void BeginInformHostOfParamChangeFromUI(int) override
  {
    Check(false, "unexpected parameter gesture");
  }
  void EndInformHostOfParamChangeFromUI(int) override
  {
    Check(false, "unexpected parameter gesture");
  }
};

struct DialogDriver
{
  int response;
  std::vector<int> buttons;
  std::string selection;
  std::string initialFolder;
  std::string initialName;
  bool checkFilters = false;
  bool configured = false;
  bool done = false;
  unsigned ticks = 0;
  guint source;

  explicit DialogDriver(int response, std::vector<int> buttons = {})
  : response(response), buttons(std::move(buttons))
  {
    source = g_timeout_add(30, Tick, this);
  }

  ~DialogDriver()
  {
    if (!done)
      g_source_remove(source);
    Check(done, "dialog automation completed");
  }

  static gboolean Tick(gpointer data)
  {
    auto& driver = *static_cast<DialogDriver*>(data);
    ++driver.ticks;
    GList* windows = gtk_window_list_toplevels();
    GtkWidget* dialog = nullptr;
    for (GList* entry = windows; entry; entry = entry->next)
    {
      if (GTK_IS_DIALOG(entry->data) && gtk_widget_get_visible(GTK_WIDGET(entry->data)))
      {
        dialog = GTK_WIDGET(entry->data);
        break;
      }
    }
    g_list_free(windows);
    if (!dialog)
      return G_SOURCE_CONTINUE;

    if (driver.ticks > 100)
    {
      Check(false, "dialog selection became ready before timeout");
      gtk_dialog_response(GTK_DIALOG(dialog), GTK_RESPONSE_CANCEL);
      driver.done = true;
      return G_SOURCE_REMOVE;
    }

    if (!driver.configured)
    {
      if (GTK_IS_FILE_CHOOSER(dialog) && !driver.initialFolder.empty())
      {
        char* folder = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
        if (!folder)
          return G_SOURCE_CONTINUE;
        Check(driver.initialFolder == folder, "initial chooser directory preserved");
        g_free(folder);
      }
      if (!driver.initialName.empty())
      {
        char* name = gtk_file_chooser_get_current_name(GTK_FILE_CHOOSER(dialog));
        Check(name && driver.initialName == name, "initial save filename preserved");
        g_free(name);
      }
      if (!driver.buttons.empty())
      {
        GList* children = gtk_container_get_children(
          GTK_CONTAINER(gtk_dialog_get_action_area(GTK_DIALOG(dialog))));
        Check(g_list_length(children) == driver.buttons.size(), "message button count");
        g_list_free(children);
        for (int response : driver.buttons)
          Check(gtk_dialog_get_widget_for_response(GTK_DIALOG(dialog), response) != nullptr,
                "requested message button is present");
      }
      if (driver.checkFilters)
      {
        GSList* filters = gtk_file_chooser_list_filters(GTK_FILE_CHOOSER(dialog));
        Check(g_slist_length(filters) == 2, "extension and all-files filters are present");
        if (filters && filters->next)
        {
          const char* names[] = {"audio.wav", "audio.aif", "audio.flac", "audio.txt"};
          for (unsigned i = 0; i < 4; ++i)
          {
            GtkFileFilterInfo info {};
            info.contains = static_cast<GtkFileFilterFlags>(
              GTK_FILE_FILTER_FILENAME | GTK_FILE_FILTER_DISPLAY_NAME);
            info.filename = names[i];
            info.display_name = names[i];
            Check(bool(gtk_file_filter_filter(GTK_FILE_FILTER(filters->data), &info)) == (i < 3),
                  "extension filter matches each requested extension only");
            Check(gtk_file_filter_filter(GTK_FILE_FILTER(filters->next->data), &info),
                  "all-files filter accepts other extensions");
          }
        }
        g_slist_free(filters);
      }
      if (!driver.selection.empty())
      {
        auto* chooser = GTK_FILE_CHOOSER(dialog);
        if (gtk_file_chooser_get_action(chooser) == GTK_FILE_CHOOSER_ACTION_SAVE)
        {
          const auto slash = driver.selection.find_last_of('/');
          gtk_file_chooser_set_current_name(chooser, driver.selection.c_str() + slash + 1);
        }
        else
          Check(gtk_file_chooser_set_filename(chooser, driver.selection.c_str()),
                "requested chooser selection accepted");
      }
      driver.configured = true;
    }
    if (!driver.selection.empty())
    {
      char* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
      const bool ready = filename && driver.selection == filename;
      g_free(filename);
      if (!ready)
        return G_SOURCE_CONTINUE;
    }

    gtk_dialog_response(GTK_DIALOG(dialog), driver.response);
    driver.done = true;
    return G_SOURCE_REMOVE;
  }
};

struct MessageCase
{
  EMsgBoxType type;
  std::vector<int> buttons;
  std::vector<EMsgBoxResult> results;
};

void TestMessages(IGraphicsLinux& graphics)
{
  const MessageCase cases[] = {
    {kMB_OK, {GTK_RESPONSE_OK}, {kOK}},
    {kMB_OKCANCEL, {GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL}, {kOK, kCANCEL}},
    {kMB_YESNOCANCEL, {GTK_RESPONSE_YES, GTK_RESPONSE_NO, GTK_RESPONSE_CANCEL}, {kYES, kNO, kCANCEL}},
    {kMB_YESNO, {GTK_RESPONSE_YES, GTK_RESPONSE_NO}, {kYES, kNO}},
    {kMB_RETRYCANCEL, {1, GTK_RESPONSE_CANCEL}, {kRETRY, kCANCEL}}
  };
  for (const auto& test : cases)
  {
    for (size_t i = 0; i <= test.buttons.size(); ++i)
    {
      const bool dismiss = i == test.buttons.size();
      const auto expected = sHideGTK || dismiss ? kCANCEL : test.results[i];
      unsigned callbacks = 0;
      auto callback = [&](EMsgBoxResult result) {
        ++callbacks;
        Check(result == expected, "message callback result");
      };
      if (sHideGTK)
        Check(graphics.ShowMessageBox("Test", "Linux regression", test.type, callback) == expected,
              "unavailable GTK never approves a message");
      else
      {
        DialogDriver driver(dismiss ? GTK_RESPONSE_DELETE_EVENT : test.buttons[i], test.buttons);
        Check(graphics.ShowMessageBox("Test", "Linux regression", test.type, callback) == expected,
              "message synchronous result");
      }
      Check(callbacks == 1, "message callback called exactly once");
    }
  }
  // 2 is within the enum's representable range, but is not a supported button
  // set. Casting 99 would make the test itself invoke undefined behavior.
  Check(graphics.ShowMessageBox("Test", "Unsupported type", static_cast<EMsgBoxType>(2), nullptr) == kCANCEL,
        "unsupported message type safely cancels without a callback");
}

void TestFiles(IGraphicsLinux& graphics, const std::string& folder)
{
  for (auto action : {EFileAction::Open, EFileAction::Save})
  {
    for (int response : {GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, GTK_RESPONSE_DELETE_EVENT})
    {
      WDL_String file("suggested.wav"), path(folder.c_str());
      const bool accepted = !sHideGTK && response == GTK_RESPONSE_OK;
      const std::string selected = folder + (action == EFileAction::Open ? "/input.wav" : "/output.wav");
      const std::string expectedFile = accepted ? selected : "";
      const std::string expectedPath = accepted ? folder + "/" : "";
      unsigned callbacks = 0;
      auto callback = [&](const WDL_String& resultFile, const WDL_String& resultPath) {
        ++callbacks;
        Check(expectedFile == resultFile.Get(), "file callback contains full filename");
        Check(expectedPath == resultPath.Get(), "file callback contains directory with separator");
        Check(expectedFile == file.Get() && expectedPath == path.Get(),
              "file reference outputs updated before callback");
      };
      const char* extensions = "  wav\t.aif  *.flac \n";
      if (sHideGTK)
        graphics.PromptForFile(file, path, action, extensions, callback);
      else
      {
        DialogDriver driver(response);
        driver.initialFolder = folder;
        if (action == EFileAction::Save)
          driver.initialName = "suggested.wav";
        driver.checkFilters = true;
        if (accepted)
          driver.selection = selected;
        graphics.PromptForFile(file, path, action, extensions, callback);
      }
      Check(callbacks == 1, "file callback called exactly once");
      Check(expectedFile == file.Get() && expectedPath == path.Get(), "file synchronous outputs");
    }
  }
  WDL_String file("old"), path(folder.c_str());
  if (sHideGTK)
    graphics.PromptForFile(file, path, EFileAction::Open, "", nullptr);
  else
  {
    DialogDriver driver(GTK_RESPONSE_OK);
    driver.selection = folder + "/input.wav";
    graphics.PromptForFile(file, path, EFileAction::Open, "", nullptr);
  }
  Check((sHideGTK ? "" : folder + "/input.wav") == file.Get(),
        "file output works without a completion handler or extension filter");
}

void TestDirectories(IGraphicsLinux& graphics, const std::string& folder)
{
  for (int response : {GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, GTK_RESPONSE_DELETE_EVENT})
  {
    WDL_String dir(folder.c_str());
    const bool accepted = !sHideGTK && response == GTK_RESPONSE_OK;
    const std::string expected = accepted ? folder + "/" : "";
    unsigned callbacks = 0;
    auto callback = [&](const WDL_String& file, const WDL_String& path) {
      ++callbacks;
      Check(file.GetLength() == 0, "directory callback filename is empty");
      Check(expected == path.Get(), "directory callback uses path argument with separator");
      Check(expected == dir.Get(), "directory reference updated before callback");
    };
    if (sHideGTK)
      graphics.PromptForDirectory(dir, callback);
    else
    {
      DialogDriver driver(response);
      driver.initialFolder = folder;
      if (accepted)
        driver.selection = folder;
      graphics.PromptForDirectory(dir, callback);
    }
    Check(callbacks == 1, "directory callback called exactly once");
    Check(expected == dir.Get(), "directory synchronous output");
  }
  WDL_String dir(folder.c_str());
  if (sHideGTK)
    graphics.PromptForDirectory(dir, nullptr);
  else
  {
    DialogDriver driver(GTK_RESPONSE_CANCEL);
    graphics.PromptForDirectory(dir, nullptr);
  }
  Check(dir.GetLength() == 0, "directory cancellation works without a completion handler");
}

} // namespace

// Intercept only this executable's GTK load to exercise the optional-runtime failure path.
extern "C" void* dlopen(const char* filename, int flags)
{
  if (sHideGTK && filename && strcmp(filename, "libgtk-3.so.0") == 0)
    return nullptr;
  static auto realDlopen = reinterpret_cast<void* (*)(const char*, int)>(dlsym(RTLD_NEXT, "dlopen"));
  if (!realDlopen)
  {
    fprintf(stderr, "Could not resolve the system dlopen\n");
    abort();
  }
  return realDlopen(filename, flags);
}

int main(int argc, char** argv)
{
  sHideGTK = argc == 2 && strcmp(argv[1], "no-gtk") == 0;
  if (!sHideGTK && !gtk_init_check(nullptr, nullptr))
  {
    fprintf(stderr, "GTK initialization failed\n");
    return 1;
  }
  char temp[] = "/tmp/iplug-dialog-XXXXXX";
  char* directory = mkdtemp(temp);
  if (!directory)
  {
    perror("mkdtemp");
    return 1;
  }
  const std::string folder(directory);
  const std::string input = folder + "/input.wav";
  FILE* fixture = fopen(input.c_str(), "wb");
  if (!fixture)
  {
    perror("creating dialog fixture");
    rmdir(directory);
    return 1;
  }
  fclose(fixture);

  {
    DialogDelegate delegate;
    IGraphicsLinux graphics(delegate, 320, 240, 30, 1.f);
    TestMessages(graphics);
    TestFiles(graphics, folder);
    TestDirectories(graphics, folder);
  }

  Check(unlink(input.c_str()) == 0, "remove file fixture");
  Check(rmdir(directory) == 0, "remove temporary directory");
  printf("Linux dialog checks: %s\n", sFailures ? "FAILED" : "passed");
  return sFailures ? 1 : 0;
}
