#pragma once

/** The (deliberately crude) signal analysis behind IPlugARAExample's note detection: onset
 * segmentation of an RMS envelope plus a normalized-autocorrelation pitch estimate. It stands in
 * for the real detection a pitch editor would perform, and is kept apart from the ARA model code
 * because none of it is ARA specific. */

#include <algorithm>
#include <cmath>
#include <vector>

#include "ARA_API/ARAInterface.h"
#include "IPlugUtilities.h"

namespace IPlugARAExampleAnalysis
{

/** Normalized autocorrelation pitch estimate over one window,
 * @return frequency in Hz, or 0 if the window has no clear pitch */
inline double EstimatePitch(const std::vector<float>& mono, size_t start, size_t windowSize, double sampleRate)
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

/** Segment an RMS envelope (one value per hop) into notes and estimate a pitch for each */
inline std::vector<ARA::ARAContentNote> DetectNotes(const std::vector<float>& mono, const std::vector<float>& envelope, int hop, int frame, double sampleRate)
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

} // namespace IPlugARAExampleAnalysis
