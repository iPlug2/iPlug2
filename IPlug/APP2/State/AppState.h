/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for more info.

 ==============================================================================
*/

#pragma once

#include <string>
#include <vector>

#include "IPlugPlatform.h"
#include "AudioDeviceInfo.h"

BEGIN_IPLUG_NAMESPACE

/**
 * AppState - Persistent application state for standalone app
 *
 * Stores all user-configurable settings including audio/MIDI device selection,
 * channel routing, and I/O configuration selection.
 */
struct AppState
{
  //--- Audio Settings ---

  /** Audio API/driver type */
  EAudioApi audioApi = GetDefaultAudioApi();

  /** Input device name (empty for instruments with no audio input) */
  std::string inputDeviceName;

  /** Output device name */
  std::string outputDeviceName;

  /** Sample rate in Hz */
  uint32_t sampleRate = 44100;

  /** Buffer size in samples */
  uint32_t bufferSize = 512;

  //--- Channel Routing ---

  /** Index into available I/O configurations (from PLUG_CHANNEL_IO) */
  int selectedIOConfig = 0;

  /** Input channel mappings: maps plugin channels to device channels */
  std::vector<ChannelMapping> inputMappings;

  /** Output channel mappings: maps plugin channels to device channels */
  std::vector<ChannelMapping> outputMappings;

  //--- MIDI Settings ---

  /** MIDI input device name ("off" for disabled) */
  std::string midiInputDevice = "off";

  /** MIDI output device name ("off" for disabled) */
  std::string midiOutputDevice = "off";

  /** MIDI input channel filter (0 = all, 1-16 = specific channel) */
  int midiInputChannel = 0;

  /** MIDI output channel (0 = pass-through, 1-16 = force channel) */
  int midiOutputChannel = 0;

  //--- Comparison ---

  bool operator==(const AppState& other) const;
  bool operator!=(const AppState& other) const { return !(*this == other); }

  /** Check if audio settings differ (requires audio restart) */
  bool AudioSettingsDiffer(const AppState& other) const;

  /** Check if MIDI settings differ */
  bool MidiSettingsDiffer(const AppState& other) const;

  //--- Defaults ---

  /** Reset to default values */
  void Reset();

  /** Create default input mappings for given channel count */
  void CreateDefaultInputMappings(int nChannels);

  /** Create default output mappings for given channel count */
  void CreateDefaultOutputMappings(int nChannels);
};

/**
 * AppStateSerializer - Handles loading/saving AppState to INI file
 */
class AppStateSerializer
{
public:
  /** Get path to settings INI file for a given app name */
  static std::string GetSettingsPath(const char* appName);

  /** Load state from INI file */
  static bool Load(AppState& state, const std::string& path);

  /** Save state to INI file */
  static bool Save(const AppState& state, const std::string& path);

private:
  // Section names
  static constexpr const char* kAudioSection = "audio";
  static constexpr const char* kMidiSection = "midi";
  static constexpr const char* kRoutingSection = "routing";

  // Key names
  static constexpr const char* kKeyApi = "api";
  static constexpr const char* kKeyInputDev = "input_device";
  static constexpr const char* kKeyOutputDev = "output_device";
  static constexpr const char* kKeySampleRate = "sample_rate";
  static constexpr const char* kKeyBufferSize = "buffer_size";
  static constexpr const char* kKeyIOConfig = "io_config";
  static constexpr const char* kKeyMidiInDev = "midi_in_device";
  static constexpr const char* kKeyMidiOutDev = "midi_out_device";
  static constexpr const char* kKeyMidiInChan = "midi_in_channel";
  static constexpr const char* kKeyMidiOutChan = "midi_out_channel";
};

END_IPLUG_NAMESPACE
