#pragma once

#include "IPlugARAExample.h"
#include "IControl.h"

using namespace iplug;
using namespace iplug::igraphics;

/** Visualizes and edits the ARA model graph of the document this instance is bound to:
 * one row per region sequence (host track), with its playback regions drawn as blocks containing
 * the waveform of the underlying audio source's cached samples, the notes detected by the
 * background analysis, and the analysis progress while it runs.
 * Also shows the host's view selection (via the ARA editor view role), region sequence hiding,
 * host tempo/time signature/key signature (read via ARA content readers), the timestretch factor
 * the host applied to each region, and the transport position.
 * Clicking a region asks the host to move its transport there (ARA playback controller);
 * alt-clicking auditions it through the ARA editor renderer role.
 * Reading the ARA model graph from the UI thread is safe here because ARA hosts edit the model
 * on the main thread (bracketed by begin/endEditing). */
class ARATimelineControl : public IControl
{
public:
  static constexpr float kHeaderH = 46.f;
  static constexpr float kRulerH = 16.f;
  static constexpr float kLabelW = 110.f;

  ARATimelineControl(const IRECT& bounds, IPlugARAExample& plug)
  : IControl(bounds)
  , mPlug(plug)
  {
  }

  bool IsDirty() override { return true; } // redraw continuously at PLUG_FPS

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    auto* pRegion = HitTestRegion(x, y);

    if (!pRegion)
      return;

    auto* pDC = mPlug.GetARADocumentController<IPlugARAExampleDocumentController>();

    if (!pDC)
      return;

    if (mod.A) // alt-click: audition through the ARA editor renderer role
    {
      if (auto* pEditorRenderer = mPlug.GetARAEditorRenderer<IPlugARAExampleEditorRenderer>())
        pEditorRenderer->StartPreview(pRegion);
    }
    else if (mod.R) // right-click: toggle the audio modification's reverse edit
    {
      auto* pModification = pRegion->getAudioModification<IPlugARAExampleAudioModification>();
      pModification->SetReverse(!pModification->GetReverse());
    }
    else // plain click: ask the host to play from the region's start
    {
      pDC->RequestHostPlaybackFrom(pRegion->getStartInPlaybackTime());
    }
  }

  void Draw(IGraphics& g) override
  {
    const IRECT r = mRECT;
    g.FillRect(DEFAULT_FGCOLOR, r);
    g.DrawRect(DEFAULT_FRCOLOR, r);

    const IText headerText = DEFAULT_TEXT.WithSize(13.f).WithAlign(EAlign::Near);
    const IText dimText = headerText.WithSize(12.f).WithFGColor(COLOR_DARK_GRAY);
    const IText smallText = DEFAULT_TEXT.WithSize(11.f).WithAlign(EAlign::Near).WithVAlign(EVAlign::Top);

    auto* pDC = mPlug.GetARADocumentController<IPlugARAExampleDocumentController>();

    if (!pDC)
    {
      g.DrawText(DEFAULT_TEXT.WithSize(18.f), "Not bound to an ARA document controller", r);
      g.DrawText(DEFAULT_TEXT.WithSize(13.f).WithFGColor(COLOR_DARK_GRAY),
                 "Load this plug-in in an ARA host (e.g. REAPER: FX settings -> \"Allow use of ARA\")", r.GetVShifted(28.f));
      return;
    }

    auto* pDoc = pDC->getDocument();

    if (!pDoc)
      return;

    // gather selection & hiding state from the editor view role, if this instance has it
    auto* pView = mPlug.GetARAEditorView<IPlugARAExampleEditorView>();
    std::vector<ARA::PlugIn::PlaybackRegion*> selectedRegions;
    std::vector<ARA::PlugIn::RegionSequence*> hiddenSequences;

    if (pView)
    {
      selectedRegions = pView->getViewSelection().getEffectivePlaybackRegions();
      hiddenSequences = pView->getHiddenRegionSequences();
    }

    // header line 1: document + roles
    const char* docName = pDoc->getName();
    WDL_String str;
    str.SetFormatted(512, "ARA document: '%s'  |  roles:%s%s%s", docName ? docName : "unnamed",
                     mPlug.GetARAPlaybackRenderer() ? " PlaybackRenderer" : "",
                     mPlug.GetARAEditorRenderer() ? " EditorRenderer" : "",
                     pView ? " EditorView" : "");
    const float headerRowH = kHeaderH / 3.f;
    g.DrawText(headerText, str.Get(), IRECT(r.L + 6.f, r.T, r.R - 6.f, r.T + headerRowH));

    // header line 2: host musical content + selection state
    const auto mcInfo = pDC->GetMusicalContextInfo();

    if (mcInfo.valid)
      str.SetFormatted(512, "host tempo: %.2f BPM, %d/%d", mcInfo.bpm, mcInfo.timeSigNumerator, mcInfo.timeSigDenominator);
    else
      str.Set("host tempo: n/a");

    if (mcInfo.hasKeySignature)
      str.AppendFormatted(64, ", key root %d", mcInfo.rootKey);

    if (pView)
      str.AppendFormatted(512, "  |  %d regions selected (%d notifications)",
                          static_cast<int>(selectedRegions.size()), pView->GetNumSelectionNotifications());
    else
      str.Append("  |  editor view role not assigned");

    str.AppendFormatted(512, "  |  transport: %s @ %.2fs", mPlug.GetTransportWasRunning() ? "playing" : "stopped", mPlug.GetPlayheadPos());

    g.DrawText(dimText, str.Get(), IRECT(r.L + 6.f, r.T + headerRowH, r.R - 6.f, r.T + 2.f * headerRowH));

    // header line 3: analysis + capability diagnostics
    int nCachedSources = 0;
    int nAnalysed = 0;
    int nAnalysing = 0;
    int nNotes = 0;
    const auto& audioSources = pDoc->getAudioSources<IPlugARAExampleAudioSource>();

    for (auto* pSource : audioSources)
    {
      nCachedSources += pSource->HasSampleCache() ? 1 : 0;
      nAnalysing += pSource->IsAnalysing() ? 1 : 0;

      if (pSource->HasAnalysis())
      {
        nAnalysed++;
        nNotes += static_cast<int>(pSource->GetAnalysis().notes.size());
      }
    }

    str.SetFormatted(512, "analysis: %d/%d done", nAnalysed, static_cast<int>(audioSources.size()));

    if (nAnalysing > 0)
      str.AppendFormatted(64, " (%d running)", nAnalysing);

    str.AppendFormatted(512, ", %d notes exported  |  %d/%d sources cached  |  host transport control: %s",
                        nNotes, nCachedSources, static_cast<int>(audioSources.size()),
                        pDC->HasHostPlaybackControl() ? "yes" : "no");

    if (auto* pRenderer = mPlug.GetARAPlaybackRenderer<IPlugARAExamplePlaybackRenderer>())
      str.AppendFormatted(512, "  |  rendering %d regions", static_cast<int>(pRenderer->getPlaybackRegions().size()));

    g.DrawText(dimText, str.Get(), IRECT(r.L + 6.f, r.T + 2.f * headerRowH, r.R - 6.f, r.T + kHeaderH));

    // timeline geometry
    const auto& sequences = pDoc->getRegionSequences();

    double tEnd = 10.;
    for (auto* pSeq : sequences)
      for (auto* pReg : pSeq->getPlaybackRegions())
        tEnd = std::max(tEnd, pReg->getEndInPlaybackTime());
    tEnd *= 1.02;

    mTimelineL = r.L + kLabelW;
    mTimelineR = r.R - 6.f;
    mTimeEnd = tEnd;

    const float rulerT = r.T + kHeaderH;
    const float bodyT = rulerT + kRulerH;
    mBodyT = bodyT;


    // time ruler
    double tick = 0.1;
    for (double candidate : { 0.25, 0.5, 1., 2., 5., 10., 15., 30., 60., 120., 300. })
    {
      if ((tick / tEnd) * (mTimelineR - mTimelineL) >= 50.)
        break;
      tick = candidate;
    }

    const IText tickText = dimText.WithSize(10.f).WithVAlign(EVAlign::Top);
    for (double t = 0.; t < tEnd; t += tick)
    {
      const float x = TimeToX(t);
      g.DrawLine(DEFAULT_FRCOLOR.WithOpacity(0.2f), x, rulerT + kRulerH, x, r.B - 1.f);
      str.SetFormatted(32, "%gs", t);
      g.DrawText(tickText, str.Get(), IRECT(x + 2.f, rulerT + 2.f, x + 48.f, rulerT + kRulerH));
    }

    if (sequences.empty())
      g.DrawText(dimText, "No region sequences in the ARA document", IRECT(r.L + 6.f, bodyT, r.R, bodyT + 20.f));

    // one row per region sequence
    mRowH = iplug::Clip((r.B - 2.f - bodyT) / static_cast<float>(std::max<size_t>(1, sequences.size())), 30.f, 80.f);
    int rowIdx = 0;

    for (auto* pSeq : sequences)
    {
      float rowT, rowB;
      if (!GetRowBounds(rowIdx, rowT, rowB))
        break;
      rowIdx++;

      const bool hidden = std::find(hiddenSequences.begin(), hiddenSequences.end(), pSeq) != hiddenSequences.end();

      if (rowIdx % 2)
        g.FillRect(COLOR_BLACK.WithOpacity(0.06f), IRECT(r.L + 1.f, rowT, r.R - 1.f, rowB));

      // sequence label + host-provided track color
      const IColor seqColor = ARAColorToIColor(pSeq->getColor(), IColor(255, 90, 120, 180));
      g.FillRect(seqColor, IRECT(r.L + 4.f, rowT + 4.f, r.L + 10.f, rowB - 4.f));

      const char* seqName = pSeq->getName();
      str.SetFormatted(256, "%s%s", seqName ? seqName : "Sequence", hidden ? " (hidden)" : "");
      g.DrawText(hidden ? dimText : headerText, str.Get(), IRECT(r.L + 14.f, rowT, r.L + kLabelW - 4.f, rowB));

      // playback regions
      for (auto* pReg : pSeq->getPlaybackRegions())
      {
        auto* pModification = pReg->getAudioModification<IPlugARAExampleAudioModification>();

        if (pModification->isDeactivatedForUndoHistory())
          continue;

        auto* pSource = pModification->getAudioSource<IPlugARAExampleAudioSource>();

        float blockL, blockR;
        if (!GetRegionXBounds(pReg, blockL, blockR))
          continue;

        const IRECT block(blockL, rowT + 3.f, blockR, rowB - 3.f);
        const bool selected = std::find(selectedRegions.begin(), selectedRegions.end(), pReg) != selectedRegions.end();

        IColor regionColor = ARAColorToIColor(pReg->getEffectiveColor(), seqColor);

        g.FillRect(regionColor.WithOpacity(hidden ? 0.15f : (selected ? 0.5f : 0.3f)), block);
        DrawWaveform(g, block, pReg, pModification, pSource, regionColor);
        DrawNotes(g, block, pReg, pModification, pSource);
        DrawAnalysisProgress(g, block, pSource);
        g.DrawRect(selected ? COLOR_BLACK : regionColor, block, 0, selected ? 2.f : 1.f);

        if (block.W() > 40.f)
        {
          const char* regName = pReg->getEffectiveName();
          str.SetFormatted(256, "%s", regName ? regName : "region");

          // the timestretch the host applied by dragging the region's borders
          const double modDuration = pReg->getDurationInAudioModificationTime();
          if (modDuration > 0.)
          {
            const double stretch = pReg->getDurationInPlaybackTime() / modDuration;
            if (std::abs(stretch - 1.) > 0.001)
              str.AppendFormatted(64, "  [stretch %.2fx]", stretch);
          }

          if (pModification->GetReverse())
            str.Append("  [reversed]");

          if (pSource->HasAnalysis() && block.W() > 150.f)
          {
            const auto analysis = pSource->GetAnalysis();
            str.AppendFormatted(96, "  (pk %.1f dB, %d notes)", iplug::AmpToDB(analysis.peak), static_cast<int>(analysis.notes.size()));
          }

          g.DrawText(smallText, str.Get(), block.GetPadded(-3.f));
        }
      }
    }

    // playhead
    const float playheadX = TimeToX(mPlug.GetPlayheadPos());
    if (playheadX >= mTimelineL && playheadX <= mTimelineR)
      g.DrawLine(COLOR_BLACK, playheadX, rulerT, playheadX, r.B - 1.f, 0, 1.5f);
  }

private:
  static IColor ARAColorToIColor(const ARA::ARAColor* pColor, const IColor& fallback)
  {
    if (!pColor)
      return fallback;

    return IColor(255, static_cast<int>(pColor->r * 255.f), static_cast<int>(pColor->g * 255.f), static_cast<int>(pColor->b * 255.f));
  }

  /** @return the playback region under the given point, or nullptr */
  ARA::PlugIn::PlaybackRegion* HitTestRegion(float x, float y)
  {
    auto* pDC = mPlug.GetARADocumentController<IPlugARAExampleDocumentController>();

    if (!pDC || !pDC->getDocument() || mTimeEnd <= 0.)
      return nullptr;

    const auto& sequences = pDC->getDocument()->getRegionSequences();
    int rowIdx = 0;

    for (auto* pSeq : sequences)
    {
      float rowT, rowB;
      if (!GetRowBounds(rowIdx, rowT, rowB))
        break;
      rowIdx++;

      if (y < rowT || y > rowB)
        continue;

      for (auto* pReg : pSeq->getPlaybackRegions())
      {
        float blockL, blockR;
        if (GetRegionXBounds(pReg, blockL, blockR) && x >= blockL && x <= blockR)
          return pReg;
      }
    }

    return nullptr;
  }

  // Timeline geometry shared by Draw() and HitTestRegion(), using the layout cached by Draw()
  float TimeToX(double t) const
  {
    return mTimelineL + static_cast<float>((t / mTimeEnd) * (mTimelineR - mTimelineL));
  }

  /** @return false if the row with the given index does not fit in the control */
  bool GetRowBounds(int rowIdx, float& rowT, float& rowB) const
  {
    rowT = mBodyT + rowIdx * mRowH;
    rowB = std::min(rowT + mRowH, mRECT.B - 1.f);
    return rowT + 4.f <= mRECT.B;
  }

  /** @return false if the region is too narrow to be drawn or clicked */
  bool GetRegionXBounds(ARA::PlugIn::PlaybackRegion* pReg, float& blockL, float& blockR) const
  {
    blockL = std::max(TimeToX(pReg->getStartInPlaybackTime()), mTimelineL);
    blockR = std::min(TimeToX(pReg->getEndInPlaybackTime()), mTimelineR);
    return blockR - blockL >= 1.f;
  }

  /** Draw the waveform of the region, honouring the audio modification's reverse edit and the
   * host's timestretch, so that what is drawn matches what the renderer produces */
  void DrawWaveform(IGraphics& g, const IRECT& block, ARA::PlugIn::PlaybackRegion* pReg, IPlugARAExampleAudioModification* pModification, IPlugARAExampleAudioSource* pSource, const IColor& color)
  {
    if (!pSource->HasSampleCache() || block.W() < 2.f)
      return;

    const float midY = block.MH();
    const float halfH = (block.H() * 0.5f) - 2.f;
    const auto srcStart = pReg->getStartInAudioModificationSamples();
    const auto srcLen = pReg->getDurationInAudioModificationSamples();
    const auto nSrcSamples = pSource->getSampleCount();
    const float* pData = pSource->GetChannelData(0);
    const int nCols = static_cast<int>(block.W());
    const bool reverse = pModification->GetReverse();

    if (srcLen < 1)
      return;

    for (int i = 0; i < nCols; i++)
    {
      const int col = reverse ? (nCols - 1 - i) : i;

      const auto i0 = std::max<ARA::ARASamplePosition>(srcStart + (srcLen * col) / nCols, 0);
      const auto i1 = std::min<ARA::ARASamplePosition>(srcStart + (srcLen * (col + 1)) / nCols, nSrcSamples);

      if (i0 >= i1)
        continue;

      float peak = 0.f;
      const auto stride = std::max<ARA::ARASamplePosition>(1, (i1 - i0) / 32);
      for (auto s = i0; s < i1; s += stride)
        peak = std::max(peak, std::abs(pData[s]));

      const float x = block.L + static_cast<float>(i);
      g.DrawLine(color, x, midY - peak * halfH, x, midY + peak * halfH);
    }
  }

  /** Draw the notes the plug-in detected and exports to the host, as a small pitch lane */
  void DrawNotes(IGraphics& g, const IRECT& block, ARA::PlugIn::PlaybackRegion* pReg, IPlugARAExampleAudioModification* pModification, IPlugARAExampleAudioSource* pSource)
  {
    if (!pSource->HasAnalysis() || block.H() < 16.f)
      return;

    const auto analysis = pSource->GetAnalysis();

    if (analysis.notes.empty())
      return;

    const double modStart = pReg->getStartInAudioModificationTime();
    const double modDuration = pReg->getDurationInAudioModificationTime();

    if (modDuration <= 0.)
      return;

    // find the pitch range so the lane scales to the detected material
    int minPitch = 127;
    int maxPitch = 0;

    for (const auto& note : analysis.notes)
    {
      if (note.pitchNumber == ARA::kARAInvalidPitchNumber)
        continue;
      minPitch = std::min(minPitch, static_cast<int>(note.pitchNumber));
      maxPitch = std::max(maxPitch, static_cast<int>(note.pitchNumber));
    }

    if (minPitch > maxPitch)
      return;

    const bool reverse = pModification->GetReverse();
    const float laneT = block.T + 2.f;
    const float laneH = std::min(block.H() * 0.45f, 24.f);
    const int pitchSpan = std::max(1, maxPitch - minPitch);

    for (const auto& note : analysis.notes)
    {
      if (note.pitchNumber == ARA::kARAInvalidPitchNumber)
        continue;

      double u0 = (note.startPosition - modStart) / modDuration;
      double u1 = (note.startPosition + note.noteDuration - modStart) / modDuration;

      if (reverse) // mirror the note within the region, as the waveform and the renderer do
      {
        const double r0 = 1. - u1;
        u1 = 1. - u0;
        u0 = r0;
      }

      if (u1 < 0. || u0 > 1.)
        continue;

      const float x0 = block.L + static_cast<float>(iplug::Clip(u0, 0., 1.)) * block.W();
      const float x1 = block.L + static_cast<float>(iplug::Clip(u1, 0., 1.)) * block.W();

      const float pitchNorm = static_cast<float>(maxPitch - note.pitchNumber) / static_cast<float>(pitchSpan);
      const float y = laneT + pitchNorm * (laneH - 4.f);

      g.FillRect(IColor(230, 200, 90, 0), IRECT(x0, y, std::max(x1, x0 + 2.f), y + 3.f));
    }
  }

  /** Draw the progress of the background analysis, which is also being reported to the host */
  void DrawAnalysisProgress(IGraphics& g, const IRECT& block, IPlugARAExampleAudioSource* pSource)
  {
    if (!pSource->IsAnalysing())
      return;

    const float progress = pSource->GetAnalysisProgress();
    const IRECT bar = IRECT(block.L + 2.f, block.B - 7.f, block.R - 2.f, block.B - 2.f);

    g.FillRect(DEFAULT_FRCOLOR.WithOpacity(0.6f), bar);
    g.FillRect(IColor(255, 40, 140, 220), bar.FracRectHorizontal(progress));
  }

  IPlugARAExample& mPlug;

  // timeline geometry cached from Draw() so that mouse hit testing matches what is on screen
  float mTimelineL = 0.f;
  float mTimelineR = 0.f;
  float mBodyT = 0.f;
  float mRowH = 30.f;
  double mTimeEnd = 0.;
};
