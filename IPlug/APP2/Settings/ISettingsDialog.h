/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for more info.

 ==============================================================================
*/

#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "IPlugPlatform.h"
#include "AppState.h"
#include "AudioDeviceInfo.h"

BEGIN_IPLUG_NAMESPACE

class AudioEngine;

/**
 * LevelMeter - Visual audio level display
 *
 * Renders a simple level meter using GDI (SWELL-compatible).
 * Thread-safe for level updates from audio callback.
 */
class LevelMeter
{
public:
  LevelMeter();

  /** Set current level (called from audio thread, lock-free) */
  void SetLevel(float peakdB);

  /** Get current peak level in dB */
  float GetPeakLevel() const;

  /** Paint the meter to a device context */
  void Paint(HDC hdc, const RECT& bounds, bool horizontal = false);

  /** Reset peak hold */
  void ResetPeak();

  //--- Appearance ---

  /** Set meter colors */
  void SetColors(COLORREF background, COLORREF green, COLORREF yellow, COLORREF red);

  /** Set dB thresholds for color changes */
  void SetThresholds(float yellowdB, float reddB);

private:
  std::atomic<float> mPeakLevel{-96.f};
  float mPeakHold = -96.f;

  COLORREF mColorBackground = RGB(40, 40, 40);
  COLORREF mColorGreen = RGB(0, 200, 0);
  COLORREF mColorYellow = RGB(200, 200, 0);
  COLORREF mColorRed = RGB(200, 0, 0);

  float mYellowThreshold = -12.f;
  float mRedThreshold = -3.f;
};

/**
 * ISettingsDialog - Abstract interface for settings dialog
 *
 * Allows different implementations (SWELL, IGraphics, WebView)
 * while maintaining a consistent interface with the host.
 */
class ISettingsDialog
{
public:
  virtual ~ISettingsDialog() = default;

  //--- Display ---

  /** Show the dialog (modal). Returns true if user clicked OK. */
  virtual bool ShowModal(void* parentWindow) = 0;

  //--- State Management ---

  /** Set the current state to display */
  virtual void SetState(const AppState& state) = 0;

  /** Get the modified state */
  virtual AppState GetState() const = 0;

  //--- Device Lists ---

  /** Set available audio input devices */
  virtual void SetInputDevices(const std::vector<AudioDeviceInfo>& devices) = 0;

  /** Set available audio output devices */
  virtual void SetOutputDevices(const std::vector<AudioDeviceInfo>& devices) = 0;

  /** Set available MIDI input devices */
  virtual void SetMidiInputDevices(const std::vector<std::string>& devices) = 0;

  /** Set available MIDI output devices */
  virtual void SetMidiOutputDevices(const std::vector<std::string>& devices) = 0;

  /** Set available I/O configurations */
  virtual void SetIOConfigs(const std::vector<IOConfig>& configs) = 0;

  //--- Level Metering ---

  /** Connect to audio engine for level metering */
  virtual void SetAudioEngine(AudioEngine* engine) = 0;

  //--- Callbacks ---

  /** Called when user requests to apply changes */
  std::function<bool(const AppState&)> OnApply;

  /** Called when driver type changes (to refresh device lists) */
  std::function<void(EAudioApi)> OnDriverChange;

  /** Called when device changes (to refresh channel lists) */
  std::function<void()> OnDeviceChange;
};

/**
 * Factory function to create platform-appropriate settings dialog
 */
std::unique_ptr<ISettingsDialog> CreateSettingsDialog();

END_IPLUG_NAMESPACE
