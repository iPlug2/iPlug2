#pragma once

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "IPlugARA.h"


/** Results of the (simulated) offline analysis of an audio source: an overall peak/RMS plus a list
 * of detected notes. The notes are what this plug-in exports back to the host as ARA content -
 * this is the mechanism a real ARA plug-in like Melodyne uses to let the host read its detection
 * results, e.g. to drag them out as MIDI. */
struct IPlugARAExampleAnalysisResult
{
  double peak = 0.;
  double rms = 0.;
  std::vector<ARA::ARAContentNote> notes;
};

/** An ARA audio source that caches all its samples in memory, so that the playback renderer
 * can access them from the realtime audio thread, and runs a background analysis over that cache
 * which produces the results persisted in the ARA document archive.
 * N.B. for the sake of simplicity this example does not synchronize sample cache updates
 * (main thread) with rendering (audio thread) - a real plug-in must add appropriate
 * synchronization. The analysis results are properly guarded, since they are produced on a
 * background thread. */
class IPlugARAExampleAudioSource : public ARA::PlugIn::AudioSource
{
public:
  using ARA::PlugIn::AudioSource::AudioSource;

  ~IPlugARAExampleAudioSource() noexcept { AbortAnalysis(); }

  void UpdateSampleCache()
  {
    const auto nChans = getChannelCount();
    const auto nSamples = getSampleCount();

    mSampleCache.resize(static_cast<size_t>(nChans) * static_cast<size_t>(nSamples));

    std::vector<void*> chanPtrs(static_cast<size_t>(nChans));
    for (auto c = 0; c < nChans; c++)
      chanPtrs[c] = mSampleCache.data() + (static_cast<size_t>(c) * static_cast<size_t>(nSamples));

    ARA::PlugIn::HostAudioReader reader(this);
    if (!reader.readAudioSamples(0, nSamples, chanPtrs.data()))
      std::fill(mSampleCache.begin(), mSampleCache.end(), 0.f);
  }

  void ClearSampleCache() { mSampleCache.clear(); }

  bool HasSampleCache() const { return !mSampleCache.empty(); }

  const float* GetChannelData(int chan) const { return mSampleCache.data() + (static_cast<size_t>(chan) * static_cast<size_t>(getSampleCount())); }

  // -- Analysis --------------------------------------------------------------------------------

  /** Kick off the background analysis, unless results are already available or it is running.
   * Analysing on a worker thread is what lets the plug-in report analysis progress to the host,
   * which a real plug-in doing expensive detection must do. */
  void StartAnalysis()
  {
    if (HasAnalysis() || IsAnalysing() || !HasSampleCache())
      return;

    AbortAnalysis();

    mAnalysisAbort.store(false, std::memory_order_relaxed);
    mAnalysing.store(true, std::memory_order_release);

    getDocumentController()->notifyAudioSourceAnalysisProgressStarted(this);

    mAnalysisThread = std::thread([this]() { RunAnalysis(); });
  }

  void AbortAnalysis()
  {
    mAnalysisAbort.store(true, std::memory_order_relaxed);

    if (mAnalysisThread.joinable())
      mAnalysisThread.join();
  }

  bool IsAnalysing() const { return mAnalysing.load(std::memory_order_acquire); }
  float GetAnalysisProgress() const { return mAnalysisProgress.load(std::memory_order_relaxed); }

  bool HasAnalysis() const { return mHasAnalysis.load(std::memory_order_acquire); }

  /** @return a copy of the analysis results - safe to call from the main thread while the
   * background analysis is running */
  IPlugARAExampleAnalysisResult GetAnalysis() const
  {
    std::lock_guard<std::mutex> lock(mAnalysisMutex);
    return mAnalysis;
  }

  /** Called when restoring from an archive - the archived results stand in for a re-analysis,
   * which is the whole point of persisting them */
  void SetAnalysis(const IPlugARAExampleAnalysisResult& result)
  {
    {
      std::lock_guard<std::mutex> lock(mAnalysisMutex);
      mAnalysis = result;
    }

    mHasAnalysis.store(true, std::memory_order_release);
  }

  /** Consume the "analysis just finished" flag - the document controller polls this on the main
   * thread so that it can call notifyAudioSourceContentChanged() there */
  bool TakeAnalysisCompletedFlag()
  {
    bool expected = true;
    return mAnalysisCompleted.compare_exchange_strong(expected, false, std::memory_order_acq_rel);
  }

private:
  void RunAnalysis()
  {
    IPlugARAExampleAnalysisResult result;

    const auto nChans = static_cast<int>(getChannelCount());
    const auto nSamples = getSampleCount();
    const double sampleRate = getSampleRate();

    // mono sum, so that the detection below works on a single signal
    std::vector<float> mono(static_cast<size_t>(nSamples), 0.f);
    for (auto c = 0; c < nChans; c++)
    {
      const float* pData = GetChannelData(c);
      for (ARA::ARASampleCount s = 0; s < nSamples; s++)
        mono[static_cast<size_t>(s)] += pData[s] / static_cast<float>(nChans);
    }

    // frame-wise RMS envelope, which drives both the overall analysis values and note onsets
    constexpr int kHop = 512;
    constexpr int kFrame = 2048;

    const int nFrames = static_cast<int>(std::max<ARA::ARASampleCount>(0, (nSamples - kFrame) / kHop));
    std::vector<float> envelope(static_cast<size_t>(std::max(0, nFrames)), 0.f);

    double sumSq = 0.;
    double peak = 0.;

    for (int f = 0; f < nFrames; f++)
    {
      if (mAnalysisAbort.load(std::memory_order_relaxed))
      {
        mAnalysing.store(false, std::memory_order_release);
        return;
      }

      double frameSumSq = 0.;
      const auto start = static_cast<size_t>(f) * kHop;

      for (int i = 0; i < kFrame; i++)
      {
        const double s = mono[start + static_cast<size_t>(i)];
        frameSumSq += s * s;
        peak = std::max(peak, std::abs(s));
      }

      envelope[static_cast<size_t>(f)] = static_cast<float>(std::sqrt(frameSumSq / kFrame));
      sumSq += frameSumSq;

      // report progress to the host - this is the visible payoff of analysing off the main thread.
      // Deliberately throttled, since every update crosses the ARA API boundary.
      if ((f % 16) == 0)
      {
        const float progress = static_cast<float>(f) / static_cast<float>(std::max(1, nFrames));
        mAnalysisProgress.store(progress, std::memory_order_relaxed);
        getDocumentController()->notifyAudioSourceAnalysisProgressUpdated(this, progress);

        // an example analysis is far too fast to ever show progress, so pace it artificially
        std::this_thread::sleep_for(std::chrono::milliseconds(4));
      }
    }

    result.peak = peak;
    result.rms = (nFrames > 0) ? std::sqrt(sumSq / (static_cast<double>(nFrames) * kFrame)) : 0.;
    result.notes = DetectNotes(mono, envelope, kHop, kFrame, sampleRate);

    {
      std::lock_guard<std::mutex> lock(mAnalysisMutex);
      mAnalysis = std::move(result);
    }

    mAnalysisProgress.store(1.f, std::memory_order_relaxed);
    mHasAnalysis.store(true, std::memory_order_release);
    mAnalysing.store(false, std::memory_order_release);

    getDocumentController()->notifyAudioSourceAnalysisProgressCompleted(this);

    // the content update itself must go out from the main thread - flag it for the document
    // controller to pick up in willNotifyModelUpdates()
    mAnalysisCompleted.store(true, std::memory_order_release);
  }

  /** Segment the envelope into notes and estimate a pitch for each by autocorrelation.
   * Crude by design - it stands in for the real detection a pitch editor would perform. */
  static std::vector<ARA::ARAContentNote> DetectNotes(const std::vector<float>& mono, const std::vector<float>& envelope, int hop, int frame, double sampleRate)
  {
    std::vector<ARA::ARAContentNote> notes;

    if (envelope.empty() || sampleRate <= 0.)
      return notes;

    const float threshold = std::max(0.005f, *std::max_element(envelope.begin(), envelope.end()) * 0.2f);

    int noteStartFrame = -1;

    const auto emitNote = [&](int startFrame, int endFrame) {
      const auto startSample = static_cast<size_t>(startFrame) * static_cast<size_t>(hop);
      const auto endSample = static_cast<size_t>(endFrame) * static_cast<size_t>(hop);

      if (endSample <= startSample || endSample > mono.size())
        return;

      ARA::ARAContentNote note {};
      note.startPosition = static_cast<double>(startSample) / sampleRate;
      note.noteDuration = static_cast<double>(endSample - startSample) / sampleRate;
      note.signalDuration = note.noteDuration;
      note.attackDuration = std::min(0.01, note.noteDuration);

      float envPeak = 0.f;
      for (int f = startFrame; f < endFrame && f < static_cast<int>(envelope.size()); f++)
        envPeak = std::max(envPeak, envelope[static_cast<size_t>(f)]);

      note.volume = std::min(1.f, envPeak);

      const double freq = EstimatePitch(mono, startSample, static_cast<size_t>(frame), sampleRate);

      if (freq > 0.)
      {
        note.frequency = static_cast<float>(freq);
        note.pitchNumber = static_cast<ARA::ARAPitchNumber>(std::lround(69. + 12. * std::log2(freq / 440.)));
      }
      else
      {
        note.frequency = ARA::kARAInvalidFrequency;
        note.pitchNumber = ARA::kARAInvalidPitchNumber;
      }

      notes.push_back(note);
    };

    for (int f = 0; f < static_cast<int>(envelope.size()); f++)
    {
      const bool active = envelope[static_cast<size_t>(f)] >= threshold;

      if (active && noteStartFrame < 0)
        noteStartFrame = f;
      else if (!active && noteStartFrame >= 0)
      {
        emitNote(noteStartFrame, f);
        noteStartFrame = -1;
      }
    }

    if (noteStartFrame >= 0)
      emitNote(noteStartFrame, static_cast<int>(envelope.size()));

    return notes;
  }

  /** Normalized autocorrelation pitch estimate over one window,
   * @return frequency in Hz, or 0 if the window has no clear pitch */
  static double EstimatePitch(const std::vector<float>& mono, size_t start, size_t windowSize, double sampleRate)
  {
    constexpr double kMinFreq = 50.;
    constexpr double kMaxFreq = 2000.;
    constexpr double kPeakThreshold = 0.85; // fraction of the best score a peak must reach to count

    const auto minLag = static_cast<size_t>(sampleRate / kMaxFreq);
    const auto maxLag = static_cast<size_t>(sampleRate / kMinFreq);

    if (start + windowSize + maxLag > mono.size() || maxLag <= minLag)
      return 0.;

    double energy = 0.;
    for (size_t i = 0; i < windowSize; i++)
      energy += static_cast<double>(mono[start + i]) * mono[start + i];

    if (energy <= 0.)
      return 0.;

    // normalized correlation per lag - dividing by the energy of the lagged window as well keeps
    // the score comparable across lags, which matters for the peak picking below
    std::vector<double> scores(maxLag - minLag + 1, 0.);

    for (size_t lag = minLag; lag <= maxLag; lag++)
    {
      double corr = 0.;
      double lagEnergy = 0.;

      for (size_t i = 0; i < windowSize; i++)
      {
        const double a = mono[start + i];
        const double b = mono[start + i + lag];
        corr += a * b;
        lagEnergy += b * b;
      }

      const double norm = std::sqrt(energy * lagEnergy);
      scores[lag - minLag] = (norm > 0.) ? (corr / norm) : 0.;
    }

    const double maxScore = *std::max_element(scores.begin(), scores.end());

    if (maxScore < 0.5)
      return 0.;

    // A periodic signal correlates just as well at every multiple of its period, so simply taking
    // the highest score picks an arbitrary subharmonic (an octave-down error). Take the *first*
    // local peak that gets close to the best score instead - that is the fundamental period.
    for (size_t i = 1; i + 1 < scores.size(); i++)
    {
      if (scores[i] >= kPeakThreshold * maxScore && scores[i] >= scores[i - 1] && scores[i] >= scores[i + 1])
      {
        // parabolic interpolation around the peak for sub-sample period accuracy
        const double s0 = scores[i - 1];
        const double s1 = scores[i];
        const double s2 = scores[i + 1];
        const double denom = (2. * s1) - s0 - s2;
        const double offset = (denom != 0.) ? (0.5 * (s0 - s2) / denom) : 0.;

        const double lag = static_cast<double>(minLag + i) + iplug::Clip(offset, -1., 1.);
        return (lag > 0.) ? (sampleRate / lag) : 0.;
      }
    }

    return 0.;
  }

  std::vector<float> mSampleCache;

  mutable std::mutex mAnalysisMutex;
  IPlugARAExampleAnalysisResult mAnalysis;

  std::thread mAnalysisThread;
  std::atomic<bool> mAnalysisAbort {false};
  std::atomic<bool> mAnalysing {false};
  std::atomic<bool> mHasAnalysis {false};
  std::atomic<bool> mAnalysisCompleted {false};
  std::atomic<float> mAnalysisProgress {0.f};
};

/** An ARA audio modification - the layer of the ARA model that holds the user's *edits* of an
 * audio source. Several modifications can share one audio source (one per take), and the host
 * clones them when the user duplicates a take, which is what optionalModificationToClone is for.
 * Here the "edit" is a simple gain and reverse switch, applied by the playback renderer. */
class IPlugARAExampleAudioModification : public ARA::PlugIn::AudioModification
{
public:
  IPlugARAExampleAudioModification(ARA::PlugIn::AudioSource* pAudioSource, ARA::ARAAudioModificationHostRef hostRef, const ARA::PlugIn::AudioModification* pOptionalModificationToClone) noexcept
  : ARA::PlugIn::AudioModification(pAudioSource, hostRef, pOptionalModificationToClone)
  {
    if (auto* pClone = static_cast<const IPlugARAExampleAudioModification*>(pOptionalModificationToClone))
    {
      mGain.store(pClone->GetGain(), std::memory_order_relaxed);
      mReverse.store(pClone->GetReverse(), std::memory_order_relaxed);
    }
  }

  double GetGain() const { return mGain.load(std::memory_order_relaxed); }
  void SetGain(double gain) { mGain.store(gain, std::memory_order_relaxed); }

  bool GetReverse() const { return mReverse.load(std::memory_order_relaxed); }
  void SetReverse(bool reverse) { mReverse.store(reverse, std::memory_order_relaxed); }

private:
  std::atomic<double> mGain {1.};
  std::atomic<bool> mReverse {false};
};

/** Exports a list of ARA notes to the host. The host obtains one of these via the ARA content
 * reader API and uses it to read the plug-in's detection results. */
class IPlugARAExampleNoteContentReader : public ARA::PlugIn::ContentReader
{
public:
  IPlugARAExampleNoteContentReader(std::vector<ARA::ARAContentNote> notes, const ARA::ARAContentTimeRange* pRange)
  {
    if (pRange)
    {
      const double rangeStart = pRange->start;
      const double rangeEnd = pRange->start + pRange->duration;

      for (const auto& note : notes)
      {
        if ((note.startPosition + note.signalDuration) >= rangeStart && note.startPosition <= rangeEnd)
          mNotes.push_back(note);
      }
    }
    else
    {
      mNotes = std::move(notes);
    }
  }

  ARA::ARAInt32 getEventCount() noexcept override { return static_cast<ARA::ARAInt32>(mNotes.size()); }

  const void* getDataForEvent(ARA::ARAInt32 eventIndex) noexcept override { return &mNotes[static_cast<size_t>(eventIndex)]; }

private:
  std::vector<ARA::ARAContentNote> mNotes;
};

/** The ARA playback renderer - reads the samples of the playback regions that the host has
 * assigned to this plug-in instance, applies the audio modification's edits and the region's
 * playback transformation (timestretch), and mixes the result into the output. */
class IPlugARAExamplePlaybackRenderer : public ARA::PlugIn::PlaybackRenderer
{
public:
  using ARA::PlugIn::PlaybackRenderer::PlaybackRenderer;

  void RenderPlaybackRegions(iplug::sample** ppOutputs, int nChans, double sampleRate, ARA::ARASamplePosition samplePosition, int nFrames, bool isPlaying)
  {
    for (auto c = 0; c < nChans; c++)
      std::fill_n(ppOutputs[c], nFrames, 0.);

    if (!isPlaying)
      return;

    for (auto* pRegion : getPlaybackRegions())
      AddPlaybackRegion(ppOutputs, nChans, sampleRate, samplePosition, nFrames, pRegion);
  }

  /** Mix a single playback region into the output buffers. Shared with the editor renderer,
   * which uses it to audition a region independently of the host transport. */
  static void AddPlaybackRegion(iplug::sample** ppOutputs, int nChans, double sampleRate, ARA::ARASamplePosition samplePosition, int nFrames, ARA::PlugIn::PlaybackRegion* pRegion)
  {
    auto* pModification = pRegion->getAudioModification<IPlugARAExampleAudioModification>();

    if (pModification->isDeactivatedForUndoHistory())
      return;

    auto* pSource = pModification->getAudioSource<IPlugARAExampleAudioSource>();

    if (!pSource->HasSampleCache())
      return;

    // intersect the current buffer with the region borders, in playback samples at the host rate
    const auto regionStart = pRegion->getStartInPlaybackSamples(sampleRate);
    const auto regionEnd = pRegion->getEndInPlaybackSamples(sampleRate);

    const auto start = std::max(samplePosition, regionStart);
    const auto end = std::min(samplePosition + nFrames, regionEnd);

    if (end <= start)
      return;

    // Map the region's playback range onto its range within the audio modification. Because this
    // is expressed as a ratio of the two durations rather than a plain rate conversion, it covers
    // both sample rate conversion and the timestretch the host may have applied by dragging the
    // region's borders (we advertise kARAPlaybackTransformationTimestretch, so hosts allow that).
    const auto regionDurationInPlaybackSamples = std::max<ARA::ARASampleCount>(1, regionEnd - regionStart);
    const auto modStartInSrcSamples = pRegion->getStartInAudioModificationSamples();
    const auto modDurationInSrcSamples = pRegion->getDurationInAudioModificationSamples();
    const auto nSrcSamples = pSource->getSampleCount();
    const auto srcChans = static_cast<int>(pSource->getChannelCount());

    const double gain = pModification->GetGain();
    const bool reverse = pModification->GetReverse();

    for (auto pos = start; pos < end; pos++)
    {
      // normalized position within the region, then into the modification's sample range
      double u = static_cast<double>(pos - regionStart) / static_cast<double>(regionDurationInPlaybackSamples);

      if (reverse)
        u = 1. - u;

      const auto srcIdx = modStartInSrcSamples + static_cast<ARA::ARASamplePosition>((u * static_cast<double>(modDurationInSrcSamples)) + 0.5);

      if (srcIdx < 0 || srcIdx >= nSrcSamples)
        continue;

      const auto bufIdx = pos - samplePosition;

      for (auto c = 0; c < nChans; c++)
        ppOutputs[c][bufIdx] += gain * pSource->GetChannelData(std::min(c, srcChans - 1))[srcIdx];
    }
  }
};

/** The ARA editor renderer - the role responsible for auxiliary "editing" output such as
 * auditioning a region while the user works on it. Per the ARA spec an editor renderer that is
 * not also a playback renderer forwards its input and adds its preview signal on top. */
class IPlugARAExampleEditorRenderer : public ARA::PlugIn::EditorRenderer
{
public:
  using ARA::PlugIn::EditorRenderer::EditorRenderer;

  /** Start auditioning a region from its beginning, independently of the host transport */
  void StartPreview(ARA::PlugIn::PlaybackRegion* pRegion)
  {
    mPreviewRegion.store(pRegion, std::memory_order_relaxed);
    mPreviewPos.store(0, std::memory_order_release);
  }

  void StopPreview() { mPreviewRegion.store(nullptr, std::memory_order_relaxed); }

  /** Stop the preview if it is auditioning pRegion - called before the host destroys it, so that
   * the audio thread doesn't keep reading a dangling pointer */
  void StopPreviewIfRegion(const ARA::PlugIn::PlaybackRegion* pRegion)
  {
    auto* pPreviewRegion = const_cast<ARA::PlugIn::PlaybackRegion*>(pRegion);
    mPreviewRegion.compare_exchange_strong(pPreviewRegion, nullptr, std::memory_order_relaxed);
  }

  bool IsPreviewing() const { return mPreviewRegion.load(std::memory_order_relaxed) != nullptr; }

  /** Mix the preview signal into the output, on top of whatever is already there */
  void RenderPreview(iplug::sample** ppOutputs, int nChans, double sampleRate, int nFrames)
  {
    auto* pRegion = mPreviewRegion.load(std::memory_order_relaxed);

    if (!pRegion)
      return;

    // the preview runs on its own timeline, so feed the region renderer positions relative to
    // the region's own start rather than the host transport position
    const auto regionStart = pRegion->getStartInPlaybackSamples(sampleRate);
    const auto regionLength = pRegion->getEndInPlaybackSamples(sampleRate) - regionStart;
    const auto pos = mPreviewPos.load(std::memory_order_acquire);

    if (pos >= regionLength)
    {
      StopPreview();
      return;
    }

    IPlugARAExamplePlaybackRenderer::AddPlaybackRegion(ppOutputs, nChans, sampleRate, regionStart + pos, nFrames, pRegion);

    mPreviewPos.store(pos + nFrames, std::memory_order_release);
  }

private:
  std::atomic<ARA::PlugIn::PlaybackRegion*> mPreviewRegion {nullptr};
  std::atomic<ARA::ARASamplePosition> mPreviewPos {0};
};

/** The ARA editor view - receives view selection and region sequence hiding notifications from
 * the host while the plug-in UI is open. The base class stores the state - this subclass just
 * counts the notifications so the UI can show that they are arriving. */
class IPlugARAExampleEditorView : public ARA::PlugIn::EditorView
{
public:
  using ARA::PlugIn::EditorView::EditorView;

  int GetNumSelectionNotifications() const { return mNumSelectionNotifications; }

protected:
  void doNotifySelection(const ARA::PlugIn::ViewSelection* pSelection) noexcept override { mNumSelectionNotifications++; }

private:
  int mNumSelectionNotifications = 0;
};

/** The ARA document controller - creates our custom model/role objects, keeps the audio source
 * sample caches up to date, drives the background analysis, exports the detected notes back to
 * the host as ARA content, reads musical context content provided by the host, and
 * stores/restores the model state in the ARA document archive. */
class IPlugARAExampleDocumentController : public iplug::IPlugARADocumentController
{
public:
  using IPlugARADocumentController::IPlugARADocumentController;

  struct MusicalContextInfo
  {
    bool valid = false;
    double bpm = 0.;
    int timeSigNumerator = 0;
    int timeSigDenominator = 0;
    int rootKey = -1;
    bool hasKeySignature = false;
  };

  /** Read tempo, bar signature and key signature content from the first musical context, via the
   * host content access controller. Cached, and re-read when the host notifies a content update.
   * Must be called from the main thread. */
  MusicalContextInfo GetMusicalContextInfo()
  {
    if (mMusicalContextInfoDirty)
    {
      mMusicalContextInfoDirty = false;
      mMusicalContextInfo = MusicalContextInfo();

      const auto& contexts = getDocument()->getMusicalContexts();

      if (!contexts.empty())
      {
        auto* pContext = contexts.front();

        ARA::PlugIn::HostContentReader<ARA::kARAContentTypeTempoEntries> tempoReader(pContext);
        if (tempoReader && tempoReader.getEventCount() >= 2)
        {
          const auto e0 = tempoReader.getDataForEvent(0);
          const auto e1 = tempoReader.getDataForEvent(1);
          if (e1.timePosition > e0.timePosition)
          {
            mMusicalContextInfo.bpm = 60. * (e1.quarterPosition - e0.quarterPosition) / (e1.timePosition - e0.timePosition);
            mMusicalContextInfo.valid = true;
          }
        }

        ARA::PlugIn::HostContentReader<ARA::kARAContentTypeBarSignatures> sigReader(pContext);
        if (sigReader && sigReader.getEventCount() >= 1)
        {
          const auto sig = sigReader.getDataForEvent(0);
          mMusicalContextInfo.timeSigNumerator = sig.numerator;
          mMusicalContextInfo.timeSigDenominator = sig.denominator;
          mMusicalContextInfo.valid = true;
        }

        ARA::PlugIn::HostContentReader<ARA::kARAContentTypeKeySignatures> keyReader(pContext);
        if (keyReader && keyReader.getEventCount() >= 1)
        {
          const auto key = keyReader.getDataForEvent(0);
          mMusicalContextInfo.rootKey = key.root;
          mMusicalContextInfo.hasKeySignature = true;
          mMusicalContextInfo.valid = true;
        }
      }
    }

    return mMusicalContextInfo;
  }

  /** Ask the host to move its transport to @p timePosition and start playing.
   * Demonstrates the ARA playback controller - the one host interface that lets the plug-in
   * drive the host rather than the other way round. @return false if the host does not offer it */
  bool RequestHostPlaybackFrom(double timePosition)
  {
    auto* pController = getHostPlaybackController();

    if (!pController)
      return false;

    pController->requestSetPlaybackPosition(timePosition);
    pController->requestStartPlayback();
    return true;
  }

  bool RequestHostStopPlayback()
  {
    auto* pController = getHostPlaybackController();

    if (!pController)
      return false;

    pController->requestStopPlayback();
    return true;
  }

  /** @return true if the host provides the playback controller interface */
  bool HasHostPlaybackControl() { return getHostPlaybackController() != nullptr; }

  /** Notes of an audio modification, with the modification's edits applied - reversing the audio
   * mirrors the note positions, which is exactly why ARA has separate content hooks per model
   * layer rather than always inheriting the audio source's content. */
  static std::vector<ARA::ARAContentNote> GetNotesForModification(const IPlugARAExampleAudioModification* pModification)
  {
    const auto* pSource = pModification->getAudioSource<IPlugARAExampleAudioSource>();
    auto notes = pSource->GetAnalysis().notes;

    if (pModification->GetReverse())
    {
      const double duration = static_cast<double>(pSource->getSampleCount()) / std::max(1., pSource->getSampleRate());

      for (auto& note : notes)
        note.startPosition = duration - (note.startPosition + note.noteDuration);

      std::reverse(notes.begin(), notes.end());
    }

    return notes;
  }

protected:
  // -- Model object creation -------------------------------------------------------------------

  ARA::PlugIn::AudioSource* doCreateAudioSource(ARA::PlugIn::Document* pDoc, ARA::ARAAudioSourceHostRef hostRef) noexcept override
  {
    return new IPlugARAExampleAudioSource(pDoc, hostRef);
  }

  ARA::PlugIn::AudioModification* doCreateAudioModification(ARA::PlugIn::AudioSource* pAudioSource, ARA::ARAAudioModificationHostRef hostRef, const ARA::PlugIn::AudioModification* pOptionalModificationToClone) noexcept override
  {
    return new IPlugARAExampleAudioModification(pAudioSource, hostRef, pOptionalModificationToClone);
  }

  // -- Plug-in instance roles ------------------------------------------------------------------

  ARA::PlugIn::PlaybackRenderer* doCreatePlaybackRenderer() noexcept override
  {
    return new IPlugARAExamplePlaybackRenderer(this);
  }

  ARA::PlugIn::EditorRenderer* doCreateEditorRenderer() noexcept override
  {
    return new IPlugARAExampleEditorRenderer(this);
  }

  ARA::PlugIn::EditorView* doCreateEditorView() noexcept override
  {
    return new IPlugARAExampleEditorView(this);
  }

  // -- Model object destruction ----------------------------------------------------------------

  void willDestroyPlaybackRegion(ARA::PlugIn::PlaybackRegion* pRegion) noexcept override
  {
    // the preview is driven by the plug-in UI rather than by the host, so the host doesn't know
    // to stop it before destroying the region it is auditioning
    for (auto* pEditorRenderer : getEditorRenderers<IPlugARAExampleEditorRenderer>())
      pEditorRenderer->StopPreviewIfRegion(pRegion);
  }

  // -- Host sample access ----------------------------------------------------------------------

  void didEnableAudioSourceSamplesAccess(ARA::PlugIn::AudioSource* pAudioSource, bool enable) noexcept override
  {
    auto* pSource = static_cast<IPlugARAExampleAudioSource*>(pAudioSource);

    if (enable)
    {
      pSource->UpdateSampleCache();
      pSource->StartAnalysis();
    }
    else
    {
      pSource->AbortAnalysis();
    }
  }

  // -- Content updates from the host -----------------------------------------------------------

  void doUpdateAudioSourceContent(ARA::PlugIn::AudioSource* pAudioSource, const ARA::ARAContentTimeRange* pRange, ARA::ContentUpdateScopes scopeFlags) noexcept override
  {
    if (scopeFlags.affectSamples())
    {
      auto* pSource = static_cast<IPlugARAExampleAudioSource*>(pAudioSource);

      pSource->AbortAnalysis();

      if (pSource->isSampleAccessEnabled())
      {
        pSource->UpdateSampleCache();
        pSource->StartAnalysis();
      }
      else
      {
        pSource->ClearSampleCache();
      }
    }
  }

  void doUpdateMusicalContextContent(ARA::PlugIn::MusicalContext* pMusicalContext, const ARA::ARAContentTimeRange* pRange, ARA::ContentUpdateScopes scopeFlags) noexcept override
  {
    mMusicalContextInfoDirty = true;
  }

  void didAddMusicalContextToDocument(ARA::PlugIn::Document* pDoc, ARA::PlugIn::MusicalContext* pMusicalContext) noexcept override
  {
    mMusicalContextInfoDirty = true;
  }

  /** Called on the main thread before model updates are sent to the host - the safe place to turn
   * "the background analysis finished" into an ARA content update notification */
  void willNotifyModelUpdates() noexcept override
  {
    for (auto* pSource : getDocument()->getAudioSources<IPlugARAExampleAudioSource>())
    {
      if (pSource->TakeAnalysisCompletedFlag())
        notifyAudioSourceContentChanged(pSource, ARA::ContentUpdateScopes::notesAreAffected());
    }
  }

  // -- Analysis control ------------------------------------------------------------------------

  bool doIsAudioSourceContentAnalysisIncomplete(const ARA::PlugIn::AudioSource* pAudioSource, ARA::ARAContentType contentType) noexcept override
  {
    const auto* pSource = static_cast<const IPlugARAExampleAudioSource*>(pAudioSource);
    return !pSource->HasAnalysis();
  }

  void doRequestAudioSourceContentAnalysis(ARA::PlugIn::AudioSource* pAudioSource, std::vector<ARA::ARAContentType> const& contentTypes) noexcept override
  {
    static_cast<IPlugARAExampleAudioSource*>(pAudioSource)->StartAnalysis();
  }

  // -- Content export to the host --------------------------------------------------------------
  // This is how the host reads what the plug-in detected, e.g. to drag the notes out as MIDI.

  bool doIsAudioSourceContentAvailable(const ARA::PlugIn::AudioSource* pAudioSource, ARA::ARAContentType type) noexcept override
  {
    if (type != ARA::kARAContentTypeNotes)
      return false;

    return static_cast<const IPlugARAExampleAudioSource*>(pAudioSource)->HasAnalysis();
  }

  ARA::ARAContentGrade doGetAudioSourceContentGrade(const ARA::PlugIn::AudioSource* pAudioSource, ARA::ARAContentType type) noexcept override
  {
    return ARA::kARAContentGradeDetected;
  }

  ARA::PlugIn::ContentReader* doCreateAudioSourceContentReader(ARA::PlugIn::AudioSource* pAudioSource, ARA::ARAContentType type, const ARA::ARAContentTimeRange* pRange) noexcept override
  {
    if (type != ARA::kARAContentTypeNotes)
      return nullptr;

    return new IPlugARAExampleNoteContentReader(static_cast<IPlugARAExampleAudioSource*>(pAudioSource)->GetAnalysis().notes, pRange);
  }

  bool doIsAudioModificationContentAvailable(const ARA::PlugIn::AudioModification* pAudioModification, ARA::ARAContentType type) noexcept override
  {
    return doIsAudioSourceContentAvailable(pAudioModification->getAudioSource(), type);
  }

  ARA::ARAContentGrade doGetAudioModificationContentGrade(const ARA::PlugIn::AudioModification* pAudioModification, ARA::ARAContentType type) noexcept override
  {
    return ARA::kARAContentGradeDetected;
  }

  ARA::PlugIn::ContentReader* doCreateAudioModificationContentReader(ARA::PlugIn::AudioModification* pAudioModification, ARA::ARAContentType type, const ARA::ARAContentTimeRange* pRange) noexcept override
  {
    if (type != ARA::kARAContentTypeNotes)
      return nullptr;

    return new IPlugARAExampleNoteContentReader(GetNotesForModification(static_cast<IPlugARAExampleAudioModification*>(pAudioModification)), pRange);
  }

  bool doIsPlaybackRegionContentAvailable(const ARA::PlugIn::PlaybackRegion* pPlaybackRegion, ARA::ARAContentType type) noexcept override
  {
    return doIsAudioModificationContentAvailable(pPlaybackRegion->getAudioModification(), type);
  }

  ARA::ARAContentGrade doGetPlaybackRegionContentGrade(const ARA::PlugIn::PlaybackRegion* pPlaybackRegion, ARA::ARAContentType type) noexcept override
  {
    return ARA::kARAContentGradeDetected;
  }

  /** Notes of a playback region, mapped from modification time into the song timeline. This is
   * where the region's placement and its timestretch factor are applied, so the host receives
   * note positions that line up with what it hears. */
  ARA::PlugIn::ContentReader* doCreatePlaybackRegionContentReader(ARA::PlugIn::PlaybackRegion* pPlaybackRegion, ARA::ARAContentType type, const ARA::ARAContentTimeRange* pRange) noexcept override
  {
    if (type != ARA::kARAContentTypeNotes)
      return nullptr;

    auto notes = GetNotesForModification(pPlaybackRegion->getAudioModification<IPlugARAExampleAudioModification>());

    const double modStart = pPlaybackRegion->getStartInAudioModificationTime();
    const double modDuration = pPlaybackRegion->getDurationInAudioModificationTime();
    const double playbackStart = pPlaybackRegion->getStartInPlaybackTime();
    const double stretch = (modDuration > 0.) ? (pPlaybackRegion->getDurationInPlaybackTime() / modDuration) : 1.;

    std::vector<ARA::ARAContentNote> mapped;

    for (const auto& note : notes)
    {
      // drop notes that lie outside the part of the modification this region plays
      if ((note.startPosition + note.noteDuration) < modStart || note.startPosition > (modStart + modDuration))
        continue;

      ARA::ARAContentNote out = note;
      out.startPosition = playbackStart + ((note.startPosition - modStart) * stretch);
      out.noteDuration = note.noteDuration * stretch;
      out.signalDuration = note.signalDuration * stretch;
      out.attackDuration = note.attackDuration * stretch;
      mapped.push_back(out);
    }

    return new IPlugARAExampleNoteContentReader(std::move(mapped), pRange);
  }

  // -- Archiving -------------------------------------------------------------------------------
  // Persist the analysis results of each audio source and the edits of each audio modification,
  // keyed by persistent ID. Because supportsStoringAudioFileChunks is enabled, the same routine
  // also produces the per-source archive that gets embedded into an audio file chunk.
  // N.B. this simple example writes native-endian values - a real plug-in should use a
  // properly specified, endian-safe archive format.

  bool doStoreObjectsToArchive(ARA::PlugIn::HostArchiveWriter* pWriter, const ARA::PlugIn::StoreObjectsFilter* pFilter) noexcept override
  {
    std::vector<ARA::ARAByte> data;

    auto appendBytes = [&data](const void* pSrc, size_t size) {
      const auto* p = static_cast<const ARA::ARAByte*>(pSrc);
      data.insert(data.end(), p, p + size);
    };

    auto appendID = [&appendBytes](const std::string& id) {
      const uint32_t idLen = static_cast<uint32_t>(id.size());
      appendBytes(&idLen, sizeof(idLen));
      appendBytes(id.data(), idLen);
    };

    // audio sources: analysis results
    const auto& sources = pFilter->getAudioSourcesToStore<IPlugARAExampleAudioSource>();

    uint32_t sourceCount = 0;
    for (const auto* pSource : sources)
      sourceCount += pSource->HasAnalysis() ? 1 : 0;

    appendBytes(&sourceCount, sizeof(sourceCount));

    for (const auto* pSource : sources)
    {
      if (!pSource->HasAnalysis())
        continue;

      const auto analysis = pSource->GetAnalysis();

      appendID(pSource->getPersistentID());
      appendBytes(&analysis.peak, sizeof(analysis.peak));
      appendBytes(&analysis.rms, sizeof(analysis.rms));

      const uint32_t noteCount = static_cast<uint32_t>(analysis.notes.size());
      appendBytes(&noteCount, sizeof(noteCount));

      for (const auto& note : analysis.notes)
        appendBytes(&note, sizeof(note));
    }

    // audio modifications: user edits
    const auto& modifications = pFilter->getAudioModificationsToStore<IPlugARAExampleAudioModification>();

    const uint32_t modCount = static_cast<uint32_t>(modifications.size());
    appendBytes(&modCount, sizeof(modCount));

    for (const auto* pModification : modifications)
    {
      const double gain = pModification->GetGain();
      const uint8_t reverse = pModification->GetReverse() ? 1 : 0;

      appendID(pModification->getPersistentID());
      appendBytes(&gain, sizeof(gain));
      appendBytes(&reverse, sizeof(reverse));
    }

    return pWriter->writeBytesToArchive(0, data.size(), data.data());
  }

  bool doRestoreObjectsFromArchive(ARA::PlugIn::HostArchiveReader* pReader, const ARA::PlugIn::RestoreObjectsFilter* pFilter) noexcept override
  {
    const auto archiveSize = pReader->getArchiveSize();

    std::vector<ARA::ARAByte> data(static_cast<size_t>(archiveSize));
    if (archiveSize == 0 || !pReader->readBytesFromArchive(0, archiveSize, data.data()))
      return false;

    size_t pos = 0;
    auto readBytes = [&data, &pos](void* pDst, size_t size) {
      if (pos + size > data.size())
        return false;
      std::memcpy(pDst, data.data() + pos, size);
      pos += size;
      return true;
    };

    auto readID = [&data, &pos, &readBytes](std::string& id) {
      uint32_t idLen = 0;
      if (!readBytes(&idLen, sizeof(idLen)) || (pos + idLen > data.size()))
        return false;
      id.assign(reinterpret_cast<const char*>(data.data() + pos), idLen);
      pos += idLen;
      return true;
    };

    uint32_t sourceCount = 0;
    if (!readBytes(&sourceCount, sizeof(sourceCount)))
      return false;

    for (uint32_t i = 0; i < sourceCount; i++)
    {
      std::string id;
      if (!readID(id))
        return false;

      IPlugARAExampleAnalysisResult analysis;
      if (!readBytes(&analysis.peak, sizeof(analysis.peak)) || !readBytes(&analysis.rms, sizeof(analysis.rms)))
        return false;

      uint32_t noteCount = 0;
      if (!readBytes(&noteCount, sizeof(noteCount)))
        return false;

      for (uint32_t n = 0; n < noteCount; n++)
      {
        ARA::ARAContentNote note {};
        if (!readBytes(&note, sizeof(note)))
          return false;
        analysis.notes.push_back(note);
      }

      // the filter maps archived persistent IDs to the objects that should be restored
      if (auto* pSource = pFilter->getAudioSourceToRestoreStateWithID<IPlugARAExampleAudioSource>(id.c_str()))
        pSource->SetAnalysis(analysis);
    }

    uint32_t modCount = 0;
    if (!readBytes(&modCount, sizeof(modCount)))
      return true; // archives written before modification state was added end here

    for (uint32_t i = 0; i < modCount; i++)
    {
      std::string id;
      if (!readID(id))
        return false;

      double gain = 1.;
      uint8_t reverse = 0;
      if (!readBytes(&gain, sizeof(gain)) || !readBytes(&reverse, sizeof(reverse)))
        return false;

      if (auto* pModification = pFilter->getAudioModificationToRestoreStateWithID<IPlugARAExampleAudioModification>(id.c_str()))
      {
        pModification->SetGain(gain);
        pModification->SetReverse(reverse != 0);
      }
    }

    return true;
  }

private:
  MusicalContextInfo mMusicalContextInfo;
  bool mMusicalContextInfoDirty = true;
};
