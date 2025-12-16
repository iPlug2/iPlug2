/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for more info.

 ==============================================================================
*/

#include "AppState.h"

#include <cstdlib>

#ifdef OS_WIN
  #include <windows.h>
  #include <shlobj.h>
#else
  #include <sys/stat.h>
  #include <unistd.h>
  #include <pwd.h>
#endif

// For INI file access (WDL provides cross-platform support via SWELL)
#ifdef OS_WIN
  // Use Windows API directly
#else
  #include "IPlugSWELL.h"
#endif

BEGIN_IPLUG_NAMESPACE

bool AppState::operator==(const AppState& other) const
{
  return audioApi == other.audioApi &&
         inputDeviceName == other.inputDeviceName &&
         outputDeviceName == other.outputDeviceName &&
         sampleRate == other.sampleRate &&
         bufferSize == other.bufferSize &&
         selectedIOConfig == other.selectedIOConfig &&
         midiInputDevice == other.midiInputDevice &&
         midiOutputDevice == other.midiOutputDevice &&
         midiInputChannel == other.midiInputChannel &&
         midiOutputChannel == other.midiOutputChannel;
  // Note: Channel mappings intentionally not compared for equality
  // as they may be derived from selectedIOConfig
}

bool AppState::AudioSettingsDiffer(const AppState& other) const
{
  return audioApi != other.audioApi ||
         inputDeviceName != other.inputDeviceName ||
         outputDeviceName != other.outputDeviceName ||
         sampleRate != other.sampleRate ||
         bufferSize != other.bufferSize ||
         selectedIOConfig != other.selectedIOConfig;
}

bool AppState::MidiSettingsDiffer(const AppState& other) const
{
  return midiInputDevice != other.midiInputDevice ||
         midiOutputDevice != other.midiOutputDevice ||
         midiInputChannel != other.midiInputChannel ||
         midiOutputChannel != other.midiOutputChannel;
}

void AppState::Reset()
{
  audioApi = GetDefaultAudioApi();
  inputDeviceName.clear();
  outputDeviceName.clear();
  sampleRate = 44100;
  bufferSize = 512;
  selectedIOConfig = 0;
  inputMappings.clear();
  outputMappings.clear();
  midiInputDevice = "off";
  midiOutputDevice = "off";
  midiInputChannel = 0;
  midiOutputChannel = 0;
}

void AppState::CreateDefaultInputMappings(int nChannels)
{
  inputMappings.clear();
  for (int i = 0; i < nChannels; ++i)
  {
    inputMappings.emplace_back(i, i, 0);
  }
}

void AppState::CreateDefaultOutputMappings(int nChannels)
{
  outputMappings.clear();
  for (int i = 0; i < nChannels; ++i)
  {
    outputMappings.emplace_back(i, i, 0);
  }
}

//--- AppStateSerializer ---

std::string AppStateSerializer::GetSettingsPath(const char* appName)
{
  std::string path;

#ifdef OS_WIN
  char appDataPath[MAX_PATH];
  if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appDataPath)))
  {
    path = appDataPath;
    path += "\\";
    path += appName;
    path += "\\";

    // Create directory if it doesn't exist
    CreateDirectoryA(path.c_str(), NULL);

    path += "settings.ini";
  }
#elif defined(OS_MAC)
  // Check if sandboxed
  const char* home = getenv("HOME");
  if (!home)
  {
    struct passwd* pw = getpwuid(getuid());
    if (pw)
      home = pw->pw_dir;
  }

  if (home)
  {
    path = home;
    path += "/Library/Application Support/";
    path += appName;
    path += "/";

    // Create directory if it doesn't exist
    mkdir(path.c_str(), 0755);

    path += "settings.ini";
  }
#elif defined(OS_LINUX)
  // Use XDG config dir or fallback to ~/.config
  const char* configDir = getenv("XDG_CONFIG_HOME");
  if (configDir)
  {
    path = configDir;
  }
  else
  {
    const char* home = getenv("HOME");
    if (!home)
    {
      struct passwd* pw = getpwuid(getuid());
      if (pw)
        home = pw->pw_dir;
    }
    if (home)
    {
      path = home;
      path += "/.config";
    }
  }

  if (!path.empty())
  {
    path += "/";
    path += appName;
    path += "/";

    // Create directory if it doesn't exist
    mkdir(path.c_str(), 0755);

    path += "settings.ini";
  }
#endif

  return path;
}

bool AppStateSerializer::Load(AppState& state, const std::string& path)
{
  char buf[512];

  // Audio settings
  GetPrivateProfileStringA(kAudioSection, kKeyApi, "", buf, sizeof(buf), path.c_str());
  if (buf[0])
  {
    // Parse API name
    std::string apiStr = buf;
    auto apis = GetAvailableAudioApis();
    for (auto api : apis)
    {
      if (apiStr == GetAudioApiName(api))
      {
        state.audioApi = api;
        break;
      }
    }
  }

  GetPrivateProfileStringA(kAudioSection, kKeyInputDev, "", buf, sizeof(buf), path.c_str());
  state.inputDeviceName = buf;

  GetPrivateProfileStringA(kAudioSection, kKeyOutputDev, "", buf, sizeof(buf), path.c_str());
  state.outputDeviceName = buf;

  state.sampleRate = GetPrivateProfileIntA(kAudioSection, kKeySampleRate, 44100, path.c_str());
  state.bufferSize = GetPrivateProfileIntA(kAudioSection, kKeyBufferSize, 512, path.c_str());
  state.selectedIOConfig = GetPrivateProfileIntA(kRoutingSection, kKeyIOConfig, 0, path.c_str());

  // MIDI settings
  GetPrivateProfileStringA(kMidiSection, kKeyMidiInDev, "off", buf, sizeof(buf), path.c_str());
  state.midiInputDevice = buf;

  GetPrivateProfileStringA(kMidiSection, kKeyMidiOutDev, "off", buf, sizeof(buf), path.c_str());
  state.midiOutputDevice = buf;

  state.midiInputChannel = GetPrivateProfileIntA(kMidiSection, kKeyMidiInChan, 0, path.c_str());
  state.midiOutputChannel = GetPrivateProfileIntA(kMidiSection, kKeyMidiOutChan, 0, path.c_str());

  return true;
}

bool AppStateSerializer::Save(const AppState& state, const std::string& path)
{
  char buf[64];

  // Audio settings
  WritePrivateProfileStringA(kAudioSection, kKeyApi, GetAudioApiName(state.audioApi), path.c_str());
  WritePrivateProfileStringA(kAudioSection, kKeyInputDev, state.inputDeviceName.c_str(), path.c_str());
  WritePrivateProfileStringA(kAudioSection, kKeyOutputDev, state.outputDeviceName.c_str(), path.c_str());

  snprintf(buf, sizeof(buf), "%u", state.sampleRate);
  WritePrivateProfileStringA(kAudioSection, kKeySampleRate, buf, path.c_str());

  snprintf(buf, sizeof(buf), "%u", state.bufferSize);
  WritePrivateProfileStringA(kAudioSection, kKeyBufferSize, buf, path.c_str());

  snprintf(buf, sizeof(buf), "%d", state.selectedIOConfig);
  WritePrivateProfileStringA(kRoutingSection, kKeyIOConfig, buf, path.c_str());

  // MIDI settings
  WritePrivateProfileStringA(kMidiSection, kKeyMidiInDev, state.midiInputDevice.c_str(), path.c_str());
  WritePrivateProfileStringA(kMidiSection, kKeyMidiOutDev, state.midiOutputDevice.c_str(), path.c_str());

  snprintf(buf, sizeof(buf), "%d", state.midiInputChannel);
  WritePrivateProfileStringA(kMidiSection, kKeyMidiInChan, buf, path.c_str());

  snprintf(buf, sizeof(buf), "%d", state.midiOutputChannel);
  WritePrivateProfileStringA(kMidiSection, kKeyMidiOutChan, buf, path.c_str());

  return true;
}

END_IPLUG_NAMESPACE
