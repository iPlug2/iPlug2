#include "IPlugReaperExtensionWebUI.h"
#include "ReaperExt_include_in_plug_src.h"

#include "wavwrite.h"

#include <algorithm>
#include <vector>
#include <string>
#include <cstdio>

IPlugReaperExtensionWebUI::IPlugReaperExtensionWebUI(reaper_plugin_info_t* pRec)
: ReaperExtBase(pRec)
{
  //Use IMPAPI to register any Reaper APIs that you need to use
  IMPAPI(GetNumTracks);
  IMPAPI(CountTracks);
  IMPAPI(InsertTrackAtIndex);
  IMPAPI(ShowConsoleMsg);
  IMPAPI(UpdateArrange);

  // APIs for offline processing of a selected media item
  IMPAPI(CountSelectedMediaItems);
  IMPAPI(GetSelectedMediaItem);
  IMPAPI(GetActiveTake);
  IMPAPI(GetMediaItemTake_Source);
  IMPAPI(GetMediaSourceNumChannels);
  IMPAPI(GetMediaSourceSampleRate);
  IMPAPI(GetMediaSourceFileName);
  IMPAPI(GetMediaSourceParent);
  IMPAPI(CreateTakeAudioAccessor);
  IMPAPI(GetAudioAccessorStartTime);
  IMPAPI(GetAudioAccessorEndTime);
  IMPAPI(GetAudioAccessorSamples);
  IMPAPI(DestroyAudioAccessor);
  IMPAPI(InsertMedia);
  IMPAPI(GetMediaItemInfo_Value);
  IMPAPI(GetCursorPosition);
  IMPAPI(SetEditCurPos);

  // APIs for choosing where to write the processed file
  IMPAPI(GetProjectPathEx);
  IMPAPI(RecursiveCreateDirectory);
  IMPAPI(file_exists);

#ifdef _DEBUG
  SetEnableDevTools(true);
#endif

  // The WebView loads its UI from resources/web/index.html.
  mEditorInitFunc = [&]() {
#ifdef _DEBUG
    // Debug: load straight from the source tree so edits to the HTML/JS hot-reload on reopen.
    LoadIndexHtml(__FILE__, "");
#else
    // Release: a REAPER extension has no bundle, so LoadIndexHtml's bundle branch doesn't apply.
    // The post-build step deploys resources/web into REAPER's UserPlugins under
    // SHARED_RESOURCES_SUBPATH; load index.html from there via the REAPER resource path.
    WDL_String indexPath;
    indexPath.SetFormatted(2048, "%s/UserPlugins/%s/index.html", GetResourcePath(), SHARED_RESOURCES_SUBPATH);
    LoadFile(indexPath.Get(), nullptr);
#endif
    EnableScroll(false);
  };

  //Define some lambdas that can be called from either GUI widgets or in response to commands
  auto action1 = [](){
    MessageBox(gParent, "Action 1!", "Reaper extension test", MB_OK); //gParent
  };

  auto action2 = [](){
    InsertTrackAtIndex(GetNumTracks(), false);
  };

  //Register an action. args: name, lambda, add to Extensions submenu, toggle ptr, context menu id, menu label
  RegisterAction("IPlugReaperExtensionWebUI: Action 1 - MsgBox", action1, true, nullptr, nullptr, "Action 1 - MsgBox");
  RegisterAction("IPlugReaperExtensionWebUI: Action 2 - AddTrack", action2);
  RegisterAction("IPlugReaperExtensionWebUI: Action 3 - Show/Hide UI", [&]() { ShowHideMainWindow(); }, true, GetWindowTogglePtr(), nullptr, "Show/Hide UI");
  RegisterAction("IPlugReaperExtensionWebUI: Toggle dock UI", [&]() { ToggleDocking(); }, true, GetDockTogglePtr(), nullptr, "Toggle dock UI");

  // Also expose the offline process on the media item right-click menu. It uses the
  // last gain set from the UI (defaults to 100%). Unlike our Extensions submenu, that menu is
  // shared with every other extension, so the label names us explicitly.
  RegisterAction("IPlugReaperExtensionWebUI: Process selected item", [&]() { ProcessSelectedItem(mGain); }, false, nullptr, "Media item context",
                 "Process selected item with " IPLUG_STRINGIFY(PLUG_CLASS_NAME));
}

void IPlugReaperExtensionWebUI::SendArbitraryMsgFromUI(int msgTag, int ctrlTag, int dataSize, const void* pData)
{
  switch (msgTag)
  {
    case kMsgTagAddTrack:
      InsertTrackAtIndex(GetNumTracks(), false);
      break;
    case kMsgTagSetGain:
      // ctrlTag carries the gain as a 0..100 percentage. Keep mGain current as the slider
      // moves so the media-item context-menu action (which reads mGain) uses the latest value,
      // and so it persists with the project even if the Process button is never clicked.
      mGain = static_cast<double>(ctrlTag) / 100.0;
      break;
    case kMsgTagProcessItem:
      // ctrlTag carries the gain as a 0..100 percentage
      mGain = static_cast<double>(ctrlTag) / 100.0;
      ProcessSelectedItem(mGain);
      break;
    case kMsgTagToggleDock:
      ToggleDocking(); // recreates the window; OnUIOpen() re-pushes state afterwards
      break;
    default:
      break;
  }
}

namespace
{

#ifdef OS_WIN
constexpr char kPathSep = '\\';
#else
constexpr char kPathSep = '/';
#endif

/** Splits a full path into its directory (no trailing separator) and the file's stem (the
 * filename with any extension removed). Both separators are accepted when splitting, because
 * REAPER hands back backslashes on Windows. The extension is stripped from the filename only,
 * so a dot in a directory name doesn't truncate the path. */
void SplitPath(const std::string& path, std::string& dirOut, std::string& stemOut)
{
  const auto sepPos = path.find_last_of("/\\");
  const std::string fileName = (sepPos == std::string::npos) ? path : path.substr(sepPos + 1);
  dirOut = (sepPos == std::string::npos) ? std::string() : path.substr(0, sepPos);

  const auto dotPos = fileName.find_last_of('.');
  stemOut = (dotPos == std::string::npos) ? fileName : fileName.substr(0, dotPos);
}

/** Finds the media file behind a take's source. Section sources - reversed, looped or trimmed
 * takes - report an empty filename but wrap a real source, reachable via GetMediaSourceParent().
 * Returns "" for sources with no file at all, e.g. in-project MIDI.
 * NB. the name comes from the parent file, while the audio accessor renders the section as the
 * take actually sounds (reversed, trimmed). That is what we want. */
std::string ResolveSourceFileName(PCM_source* pSrc)
{
  char buf[2048];

  while (pSrc)
  {
    buf[0] = '\0';
    GetMediaSourceFileName(pSrc, buf, sizeof(buf));

    if (buf[0] != '\0')
      return std::string(buf);

    pSrc = GetMediaSourceParent(pSrc);
  }

  return std::string();
}

/** The project's media/recording directory, which is where REAPER's own glue and apply-FX
 * actions write. Comes back empty for an unsaved project, in which case we fall back to sitting
 * next to the source file. */
std::string ChooseOutputDir(const std::string& srcDir)
{
  char projPath[2048] = {};
  GetProjectPathEx(nullptr, projPath, sizeof(projPath));

  return (projPath[0] != '\0') ? std::string(projPath) : srcDir;
}

/** Builds a path that doesn't already exist: <dir>/<stem>-<extName>.wav, then -001, -002...
 * Returns "" if every candidate is taken, so we never overwrite an existing file - it is
 * usually already open in the project. */
std::string MakeUniqueOutputPath(const std::string& dir, const std::string& stem, const std::string& extName)
{
  const std::string base = dir + kPathSep + stem + "-" + extName;

  std::string candidate = base + ".wav";

  if (!file_exists(candidate.c_str()))
    return candidate;

  for (int i = 1; i <= 999; i++)
  {
    char suffix[16];
    snprintf(suffix, sizeof(suffix), "-%03d", i);
    candidate = base + suffix + ".wav";

    if (!file_exists(candidate.c_str()))
      return candidate;
  }

  return std::string();
}

} // namespace

// NB. this reads/writes the whole item synchronously on the calling (UI) thread, so a very
// large media item will briefly block REAPER's UI. Fine for an example; a real extension would
// stream in the background or show progress.
void IPlugReaperExtensionWebUI::ProcessSelectedItem(double gain)
{
  if (CountSelectedMediaItems(nullptr) < 1)
  {
    ShowConsoleMsg("IPlugReaperExtensionWebUI: no media item selected\n");
    return;
  }

  MediaItem* item = GetSelectedMediaItem(nullptr, 0);
  MediaItem_Take* take = item ? GetActiveTake(item) : nullptr;
  PCM_source* src = take ? GetMediaItemTake_Source(take) : nullptr;

  if (!src)
  {
    ShowConsoleMsg("IPlugReaperExtensionWebUI: selected item has no audio take\n");
    return;
  }

  const int nch = GetMediaSourceNumChannels(src);
  const int srate = GetMediaSourceSampleRate(src);

  if (nch < 1 || srate < 1)
  {
    ShowConsoleMsg("IPlugReaperExtensionWebUI: unsupported source (no PCM audio?)\n");
    return;
  }

  // WaveWriter only writes mono or stereo, so clamp the whole pipeline rather than let the
  // header disagree with the interleaved data we write
  const int outChannels = std::min(nch, 2);

  if (nch > outChannels)
  {
    WDL_String msg;
    msg.SetFormatted(256, "IPlugReaperExtensionWebUI: source has %i channels, writing the first %i\n", nch, outChannels);
    ShowConsoleMsg(msg.Get());
  }

  const std::string srcFileName = ResolveSourceFileName(src);

  if (srcFileName.empty())
  {
    ShowConsoleMsg("IPlugReaperExtensionWebUI: selected take has no media file (in-project MIDI?)\n");
    return;
  }

  std::string srcDir, srcStem;
  SplitPath(srcFileName, srcDir, srcStem);

  const std::string outDir = ChooseOutputDir(srcDir);

  if (outDir.empty())
  {
    ShowConsoleMsg("IPlugReaperExtensionWebUI: could not determine an output directory (save the project first)\n");
    return;
  }

  // The project's recording path may name a subfolder that doesn't exist yet
  RecursiveCreateDirectory(outDir.c_str(), 0);

  const std::string outPath = MakeUniqueOutputPath(outDir, srcStem, IPLUG_STRINGIFY(PLUG_CLASS_NAME));

  if (outPath.empty())
  {
    ShowConsoleMsg("IPlugReaperExtensionWebUI: could not find a free output filename\n");
    return;
  }

  AudioAccessor* acc = CreateTakeAudioAccessor(take);
  if (!acc)
  {
    ShowConsoleMsg("IPlugReaperExtensionWebUI: failed to create audio accessor\n");
    return;
  }

  const double startTime = GetAudioAccessorStartTime(acc);
  const double endTime = GetAudioAccessorEndTime(acc);
  const int totalFrames = static_cast<int>((endTime - startTime) * srate + 0.5);

  bool ok = false;

  {
    // WaveWriter finalises the file header in its destructor, so scope it so the file
    // is complete before we InsertMedia() it below.
    WaveWriter writer(outPath.c_str(), 24 /* 24-bit PCM */, outChannels, srate, 0 /* no append */);

    if (writer.Status())
    {
      const int kBlockFrames = 4096;
      std::vector<double> buf(static_cast<size_t>(kBlockFrames) * outChannels);

      int framesDone = 0;
      while (framesDone < totalFrames)
      {
        const int n = std::min(kBlockFrames, totalFrames - framesDone);
        const double t = startTime + static_cast<double>(framesDone) / srate;

        // Zero first: the accessor leaves gaps (silence) untouched
        std::fill(buf.begin(), buf.begin() + static_cast<size_t>(n) * outChannels, 0.0);
        GetAudioAccessorSamples(acc, srate, outChannels, t, n, buf.data());

        for (int i = 0; i < n * outChannels; i++)
          buf[i] *= gain;

        writer.WriteDoubles(buf.data(), n * outChannels); // interleaved
        framesDone += n;
      }
      ok = true;
    }
    else
    {
      ShowConsoleMsg("IPlugReaperExtensionWebUI: could not open output file for writing\n");
    }
  }

  DestroyAudioAccessor(acc);

  if (ok)
  {
    WDL_String msg;
    msg.SetFormatted(2100, "IPlugReaperExtensionWebUI: wrote %s (gain %.0f%%), inserting...\n", outPath.c_str(), gain * 100.0);
    ShowConsoleMsg(msg.Get());

    // InsertMedia() drops the file at the edit cursor, so move the cursor to the source item's
    // position first to line the two up in the arrangement, then put the cursor back where the
    // user had it (InsertMedia leaves it at the end of what it inserted).
    const double cursorPos = GetCursorPosition();
    SetEditCurPos(GetMediaItemInfo_Value(item, "D_POSITION"), false /* moveview */, false /* seekplay */);

    InsertMedia(outPath.c_str(), 1 /* add on a new track */);

    SetEditCurPos(cursorPos, false, false);
    UpdateArrange();
  }
}

void IPlugReaperExtensionWebUI::OnIdle()
{
  // Safety-net poll: catches track changes not triggered by an action (e.g. our own
  // "Add Track" button, which calls the API directly). Action-driven changes are
  // handled immediately in OnActionRun().
  PushTrackCount(false);
}

void IPlugReaperExtensionWebUI::OnActionRun(int commandId, int flag)
{
  // A main-section action ran (insert/remove track, undo, etc.) - refresh right away
  // instead of waiting for the next idle tick.
  PushTrackCount(false);
}

void IPlugReaperExtensionWebUI::OnUIOpen()
{
  ReaperExtBase::OnUIOpen(); // base sends current parameter values (none here)

  // Push current state to the freshly-loaded UI so the slider / count / dock button are in sync
  PushGain();
  PushTrackCount(true);
  PushDockState();
}

void IPlugReaperExtensionWebUI::PushTrackCount(bool force)
{
  int tracks = CountTracks(0);

  if (force || tracks != mPrevTrackCount) {
    mPrevTrackCount = tracks;
    WDL_String js;
    js.SetFormatted(64, "OnTrackCount(%i)", tracks);
    EvaluateJavaScript(js.Get());
  }
}

void IPlugReaperExtensionWebUI::PushGain()
{
  WDL_String js;
  js.SetFormatted(64, "OnSetGain(%i)", static_cast<int>(mGain * 100.0 + 0.5));
  EvaluateJavaScript(js.Get());
}

void IPlugReaperExtensionWebUI::PushDockState()
{
  WDL_String js;
  js.SetFormatted(64, "OnDockState(%i)", IsDocked() ? 1 : 0);
  EvaluateJavaScript(js.Get());
}

void IPlugReaperExtensionWebUI::SaveProjectState(ProjectStateContext* ctx)
{
  // Persist the gain into the .RPP so it survives save/load and undo
  ctx->AddLine("IPLUGREAPEREXTENSIONWEBUI_GAIN %f", mGain);
}

bool IPlugReaperExtensionWebUI::LoadProjectStateLine(const char* line)
{
  double g = 0.0;
  if (sscanf(line, " IPLUGREAPEREXTENSIONWEBUI_GAIN %lf", &g) == 1) {
    mGain = g;
    PushGain(); // reflect it in the UI if the window is open
    return true; // this line belonged to us
  }
  return false;
}

void IPlugReaperExtensionWebUI::OnBeginLoadProjectState(bool isUndo)
{
  // Reset to default; LoadProjectStateLine() restores the value if the project stored one
  mGain = 1.0;
}
