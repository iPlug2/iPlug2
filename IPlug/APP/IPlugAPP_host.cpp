/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#include "IPlugAPP_host.h"
#include "NativeSettingsDialog.h"
#include "config.h"
#include "resource.h"

#ifdef OS_WIN
#include <sys/stat.h>
#endif

#include "IPlugLogger.h"

using namespace iplug;

#if !defined NO_IGRAPHICS
#include "IGraphics.h"
using namespace igraphics;
#endif

#ifndef SETTINGS_DIALOG
#define SETTINGS_DIALOG NativeSettingsDialog
#endif

#ifndef MAX_PATH_LEN
#define MAX_PATH_LEN 2048
#endif

#define STRBUFSZ 100

std::unique_ptr<IPlugAPPHost> IPlugAPPHost::sInstance;
UINT gSCROLLMSG;
extern HWND gPrefsHWND;

IPlugAPPHost::IPlugAPPHost()
: mIPlug(MakePlug(InstanceInfo{this}))
, mSettingsDialog(std::make_unique<SETTINGS_DIALOG>(*this))
{
  Init();
  TryToChangeAudio(true);
}

IPlugAPPHost::~IPlugAPPHost()
{
  mExiting = true;
  
#ifdef OS_MAC
  UnregisterDeviceNotifications();
#endif
  
  CloseAudio();
  
  if (mMidiIn)
    mMidiIn->cancelCallback();

  if (mMidiOut)
    mMidiOut->closePort();
}

//static
IPlugAPPHost* IPlugAPPHost::Create()
{
  sInstance = std::make_unique<IPlugAPPHost>();
  return sInstance.get();
}

//static
IPlugAPPHost::ISettingsDialog* IPlugAPPHost::GetSettingsDialog()
{
  return sInstance->mSettingsDialog.get();
}

bool IPlugAPPHost::Init()
{
  mIPlug->SetHost("standalone", mIPlug->GetPluginVersion(false));
    
  if (!InitSettings())
    return false;
  
  TryToChangeAudioDriverType(); // will init RTAudio with an API type based on gState->mAudioDriverType
  ProbeAudioIO(); // find out what audio IO devs are available and put their IDs in the global variables gAudioInputDevs / gAudioOutputDevs
  InitMidi(); // creates RTMidiIn and RTMidiOut objects
  ProbeMidiIO(); // find out what midi IO devs are available and put their names in the global variables gMidiInputDevs / gMidiOutputDevs
  SelectMIDIDevice(ERoute::kInput, mSettings.mMidiInDev.Get());
  SelectMIDIDevice(ERoute::kOutput, mSettings.mMidiOutDev.Get());
  
#ifdef OS_MAC
  RegisterDeviceNotifications();
#endif
  
  mIPlug->OnParamReset(kReset);
  mIPlug->OnActivate(true);
  
  return true;
}

bool IPlugAPPHost::OpenWindow(HWND pParent)
{
  return mIPlug->OpenWindow(pParent) != nullptr;
}

void IPlugAPPHost::CloseWindow()
{
  mIPlug->CloseWindow();
}

bool IPlugAPPHost::InitSettings()
{
#if defined OS_WIN
  TCHAR strPath[MAX_PATH_LEN];
  SHGetFolderPathA( NULL, CSIDL_LOCAL_APPDATA, NULL, 0, strPath );
  mINIPath.SetFormatted(MAX_PATH_LEN, "%s\\%s\\", strPath, BUNDLE_NAME);
#elif defined OS_MAC
  mINIPath.SetFormatted(MAX_PATH_LEN, "%s/Library/Application Support/%s/", getenv("HOME"), BUNDLE_NAME);
#else
  #error NOT IMPLEMENTED
#endif

  struct stat st;

  if (stat(mINIPath.Get(), &st) == 0) // if directory exists
  {
    mINIPath.Append("settings.ini"); // add file name to path

    char buf[STRBUFSZ];
    
    if (stat(mINIPath.Get(), &st) == 0) // if settings file exists read values into state
    {
      DBGMSG("Reading ini file from %s\n", mINIPath.Get());
      
      mSettings.mAudioDriverType = GetPrivateProfileInt("audio", "driver", 0, mINIPath.Get());

      GetPrivateProfileString("audio", "indev", "Built-in Input", buf, STRBUFSZ, mINIPath.Get()); mSettings.mAudioInDev.Set(buf);
      GetPrivateProfileString("audio", "outdev", "Built-in Output", buf, STRBUFSZ, mINIPath.Get()); mSettings.mAudioOutDev.Set(buf);

      //audio
      mSettings.mAudioInChanL = GetPrivateProfileInt("audio", "in1", 1, mINIPath.Get()); // 1 is first audio input
      mSettings.mAudioInChanR = GetPrivateProfileInt("audio", "in2", 2, mINIPath.Get());
      mSettings.mAudioOutChanL = GetPrivateProfileInt("audio", "out1", 1, mINIPath.Get()); // 1 is first audio output
      mSettings.mAudioOutChanR = GetPrivateProfileInt("audio", "out2", 2, mINIPath.Get());
      //mSettings.mAudioInIsMono = GetPrivateProfileInt("audio", "monoinput", 0, mINIPath.Get());

      mSettings.mBufferSize = GetPrivateProfileInt("audio", "buffer", 512, mINIPath.Get());
      mSettings.mAudioSR = GetPrivateProfileInt("audio", "sr", 44100, mINIPath.Get());

      //midi
      GetPrivateProfileString("midi", "indev", "no input", buf, STRBUFSZ, mINIPath.Get()); mSettings.mMidiInDev.Set(buf);
      GetPrivateProfileString("midi", "outdev", "no output", buf, STRBUFSZ, mINIPath.Get()); mSettings.mMidiOutDev.Set(buf);

      mSettings.mMidiInChan = GetPrivateProfileInt("midi", "inchan", 0, mINIPath.Get()); // 0 is any
      mSettings.mMidiOutChan = GetPrivateProfileInt("midi", "outchan", 0, mINIPath.Get()); // 1 is first chan
    }

    // if settings file doesn't exist, populate with default values, otherwise overwrite
    UpdateINI();
  }
  else // folder doesn't exist - make folder and make file
  {
#if defined OS_WIN
    // folder doesn't exist - make folder and make file
    CreateDirectory(mINIPath.Get(), NULL);
    mINIPath.Append("settings.ini");
    UpdateINI(); // will write file if doesn't exist
#elif defined OS_MAC
    mode_t process_mask = umask(0);
    int result_code = mkdir(mINIPath.Get(), S_IRWXU | S_IRWXG | S_IRWXO);
    umask(process_mask);

    if (!result_code)
    {
      mINIPath.Append("settings.ini");
      UpdateINI(); // will write file if doesn't exist
    }
    else
    {
      return false;
    }
#else
  #error NOT IMPLEMENTED
#endif
  }

  return true;
}

void IPlugAPPHost::UpdateINI()
{
  char buf[STRBUFSZ]; // temp buffer for writing integers to profile strings
  const char* ini = mINIPath.Get();

  sprintf(buf, "%u", mSettings.mAudioDriverType);
  WritePrivateProfileString("audio", "driver", buf, ini);

  WritePrivateProfileString("audio", "indev", mSettings.mAudioInDev.Get(), ini);
  WritePrivateProfileString("audio", "outdev", mSettings.mAudioOutDev.Get(), ini);

  sprintf(buf, "%u", mSettings.mAudioInChanL);
  WritePrivateProfileString("audio", "in1", buf, ini);
  sprintf(buf, "%u", mSettings.mAudioInChanR);
  WritePrivateProfileString("audio", "in2", buf, ini);
  sprintf(buf, "%u", mSettings.mAudioOutChanL);
  WritePrivateProfileString("audio", "out1", buf, ini);
  sprintf(buf, "%u", mSettings.mAudioOutChanR);
  WritePrivateProfileString("audio", "out2", buf, ini);
  //sprintf(buf, "%u", mSettings.mAudioInIsMono);
  //WritePrivateProfileString("audio", "monoinput", buf, ini);

  WDL_String str;
  str.SetFormatted(32, "%i", mSettings.mBufferSize);
  WritePrivateProfileString("audio", "buffer", str.Get(), ini);

  str.SetFormatted(32, "%i", mSettings.mAudioSR);
  WritePrivateProfileString("audio", "sr", str.Get(), ini);

  WritePrivateProfileString("midi", "indev", mSettings.mMidiInDev.Get(), ini);
  WritePrivateProfileString("midi", "outdev", mSettings.mMidiOutDev.Get(), ini);

  sprintf(buf, "%u", mSettings.mMidiInChan);
  WritePrivateProfileString("midi", "inchan", buf, ini);
  sprintf(buf, "%u", mSettings.mMidiOutChan);
  WritePrivateProfileString("midi", "outchan", buf, ini);
}

std::string IPlugAPPHost::GetAudioDeviceName(uint32_t deviceID) const
{
  auto str = mDAC->getDeviceInfo(deviceID).name;
  std::size_t pos = str.find(':');

  if (pos != std::string::npos)
  {
    std::string subStr = str.substr(pos + 2);
    return subStr;
  }
  else
  {
    return str;
  }
}

RtAudio::DeviceInfo IPlugAPPHost::GetDeviceInfo(uint32_t deviceID) const
{
  return mDAC->getDeviceInfo(deviceID);
}

std::optional<uint32_t> IPlugAPPHost::GetAudioDeviceID(const char* deviceNameToTest) const
{
  auto deviceIDs = mDAC->getDeviceIds();

  for (auto deviceID : deviceIDs)
  {
    auto name = GetAudioDeviceName(deviceID);

    if (std::string_view(deviceNameToTest) == name)
    {
      return deviceID;
    }
  }
  
  return std::nullopt;
}

int IPlugAPPHost::GetMIDIPortNumber(ERoute direction, const char* nameToTest) const
{
  int start = 1;
  
  auto nameStrView = std::string_view(nameToTest);
  
  if (direction == ERoute::kInput)
  {
    if (nameStrView == OFF_TEXT) return 0;
    
  #ifdef OS_MAC
    start = 2;
    if (nameStrView == "virtual input") return 1;
  #endif
    
    for (int i = 0; i < mMidiIn->getPortCount(); i++)
    {
      if (nameStrView == mMidiIn->getPortName(i).c_str())
        return (i + start);
    }
  }
  else
  {
    if (nameStrView == OFF_TEXT) return 0;
  
  #ifdef OS_MAC
    start = 2;
    if (nameStrView == "virtual output") return 1;
  #endif
  
    for (int i = 0; i < mMidiOut->getPortCount(); i++)
    {
      if (nameStrView == mMidiOut->getPortName(i).c_str())
        return (i + start);
    }
  }
  
  return -1;
}

void IPlugAPPHost::ProbeAudioIO()
{
  DBGMSG("\nRtAudio Version %s\n", RtAudio::getVersion().c_str());

  RtAudio::DeviceInfo info;

  mAudioInputDevIDs.clear();
  mAudioOutputDevIDs.clear();

  auto deviceIDs = mDAC->getDeviceIds();

  for (auto deviceID : deviceIDs)
  {
    info = mDAC->getDeviceInfo(deviceID);

    if (info.inputChannels > 0)
    {
      mAudioInputDevIDs.push_back(deviceID);
    }
    
    if (info.outputChannels > 0)
    {
      mAudioOutputDevIDs.push_back(deviceID);
    }
    
    if (info.isDefaultInput)
    {
      mDefaultInputDev = deviceID;
    }
    
    if (info.isDefaultOutput)
    {
      mDefaultOutputDev = deviceID;
    }
  }
}

void IPlugAPPHost::ProbeMidiIO()
{
  if (!IsMidiInitialised())
    return;
  else
  {
    int nInputPorts = mMidiIn->getPortCount();

    mMidiInputDevNames.push_back(OFF_TEXT);

#ifdef OS_MAC
    mMidiInputDevNames.push_back("virtual input");
#endif

    for (int i=0; i<nInputPorts; i++)
    {
      mMidiInputDevNames.push_back(mMidiIn->getPortName(i));
    }

    int nOutputPorts = mMidiOut->getPortCount();

    mMidiOutputDevNames.push_back(OFF_TEXT);

#ifdef OS_MAC
    mMidiOutputDevNames.push_back("virtual output");
#endif

    for (int i=0; i<nOutputPorts; i++)
    {
      mMidiOutputDevNames.push_back(mMidiOut->getPortName(i));
      //This means the virtual output port wont be added as an input
    }
  }
}

bool IPlugAPPHost::TryToChangeAudioDriverType()
{
  CloseAudio();
  
  if (mDAC)
  {
    mDAC = nullptr;
  }

#if defined OS_WIN
  if (mSettings.mAudioDriverType == kDeviceASIO)
    mDAC = std::make_unique<RtAudio>(RtAudio::WINDOWS_ASIO);
  else if (mSettings.mAudioDriverType == kDeviceDS)
    mDAC = std::make_unique<RtAudio>(RtAudio::WINDOWS_DS);
#elif defined OS_MAC
  if (mSettings.mAudioDriverType == kDeviceCoreAudio)
    mDAC = std::make_unique<RtAudio>(RtAudio::MACOSX_CORE);
  //else
  //mDAC = std::make_unique<RtAudio>(RtAudio::UNIX_JACK);
#else
  #error NOT IMPLEMENTED
#endif

  if (mDAC)
  {
    mDAC->setErrorCallback(ErrorCallback);
    return true;
  }

  return false;
}

bool IPlugAPPHost::TryToChangeAudio(bool start)
{
  std::optional<uint32_t> inputID;
  
#if defined OS_WIN
  // ASIO has one device, use the output for the input ID
  if (mState.mAudioDriverType == kDeviceASIO)
    inputID = GetAudioDeviceID(mState.mAudioOutDev.Get());
  else if (GetPlug()->MaxNChannels(ERoute::kInput) > 0) // Only get input device if plugin needs inputs
    inputID = GetAudioDeviceID(mState.mAudioInDev.Get());
#elif defined OS_MAC
  if (GetPlug()->MaxNChannels(ERoute::kInput) > 0) // Only get input device if plugin needs inputs
    inputID = GetAudioDeviceID(mSettings.mAudioInDev.Get());
#else
  #error NOT IMPLEMENTED
#endif

  auto outputID = GetAudioDeviceID(mSettings.mAudioOutDev.Get());

  bool failedToFindDevice = false;
  bool resetToDefault = false;

  // Only check input device if we need one
  if (inputID.has_value() && !inputID.value())
  {
    if (mDefaultInputDev)
    {
      resetToDefault = true;
      inputID = mDefaultInputDev;

      if (mAudioInputDevIDs.size())
        mSettings.mAudioInDev.Set(GetAudioDeviceName(inputID.value()).c_str());
    }
    else if (mSettings.mAudioDriverType == kDeviceASIO) // Only fail if ASIO
      failedToFindDevice = true;
    else
      inputID = std::nullopt; // Clear the input ID if we don't need it
  }

  if (!outputID)
  {
    if (mDefaultOutputDev)
    {
      resetToDefault = true;
      outputID = mDefaultOutputDev;

      if (mAudioOutputDevIDs.size())
        mSettings.mAudioOutDev.Set(GetAudioDeviceName(outputID.value()).c_str());
    }
    else
      failedToFindDevice = true;
  }

  if (resetToDefault)
  {
    DBGMSG("Couldn't find previous audio device, reseting to default\n");
    UpdateINI();
  }

  if (failedToFindDevice)
    MessageBox(gHWND, "Please check the audio settings", "Error", MB_OK);

  if (outputID && (!inputID.has_value() || inputID.value()))
  {
    uint32_t inId = inputID.has_value() ? inputID.value() : 0;
    return InitAudio(inId, outputID.value(), mSettings.mAudioSR, mSettings.mBufferSize);
  }

  return false;
}

bool IPlugAPPHost::SelectMIDIDevice(ERoute direction, const char* pPortName)
{
  int port = GetMIDIPortNumber(direction, pPortName);

  if (direction == ERoute::kInput)
  {
    if (port == -1)
    {
      mSettings.mMidiInDev.Set(OFF_TEXT);
      UpdateINI();
      port = 0;
    }

    //TODO: send all notes off?
    if (mMidiIn)
    {
      mMidiIn->closePort();

      if (port == 0)
      {
        return true;
      }
  #if defined OS_WIN
      else
      {
        mMidiIn->openPort(port-1);
        return true;
      }
  #elif defined OS_MAC
      else if (port == 1)
      {
        std::string virtualMidiInputName = "To ";
        virtualMidiInputName += BUNDLE_NAME;
        mMidiIn->openVirtualPort(virtualMidiInputName);
        return true;
      }
      else
      {
        mMidiIn->openPort(port-2);
        return true;
      }
  #else
   #error NOT IMPLEMENTED
  #endif
    }
  }
  else
  {
    if (port == -1)
    {
      mSettings.mMidiOutDev.Set(OFF_TEXT);
      UpdateINI();
      port = 0;
    }
    
    if (mMidiOut)
    {
      //TODO: send all notes off?
      mMidiOut->closePort();
      
      if (port == 0)
        return true;
#if defined OS_WIN
      else
      {
        mMidiOut->openPort(port-1);
        return true;
      }
#elif defined OS_MAC
      else if (port == 1)
      {
        std::string virtualMidiOutputName = "From ";
        virtualMidiOutputName += BUNDLE_NAME;
        mMidiOut->openVirtualPort(virtualMidiOutputName);
        return true;
      }
      else
      {
        mMidiOut->openPort(port-2);
        return true;
      }
#else
  #error NOT IMPLEMENTED
#endif
    }
  }
  
  return false;
}

void IPlugAPPHost::CloseAudio()
{
  if (mDAC && mDAC->isStreamOpen())
  {
    if (mDAC->isStreamRunning())
    {
      mAudioEnding = true;
    
      while (!mAudioDone)
        Sleep(10);
      
      mDAC->abortStream();
    }
    
    mDAC->closeStream();
  }
}

bool IPlugAPPHost::InitAudio(uint32_t inID, uint32_t outID, uint32_t sr, uint32_t iovs)
{
  CloseAudio();

  RtAudio::StreamParameters iParams, oParams;
  iParams.deviceId = inID;
  oParams.deviceId = outID;

  // Store the actual available channel counts
  bool hasInput = inID != 0 && GetPlug()->MaxNChannels(ERoute::kInput) > 0;
  
  mNumDeviceInputs = 0; // Default to 0 inputs
  mNumDeviceOutputs = 0;

  if (hasInput && inID != 0)
  {
    try {
      RtAudio::DeviceInfo inputInfo = mDAC->getDeviceInfo(inID);
      mNumDeviceInputs = inputInfo.inputChannels;
    }
    catch (const std::exception& e) {
      DBGMSG("Failed to get input device info: %s\n", e.what());
      return false;
    }
  }
    
  try {
    RtAudio::DeviceInfo outputInfo = mDAC->getDeviceInfo(outID);
    mNumDeviceOutputs = outputInfo.outputChannels;
  }
  catch (const std::exception& e) {
    DBGMSG("Failed to get output device info: %s\n", e.what());
    return false;
  }

  // Request max channels but allow less
  iParams.nChannels = hasInput ? GetPlug()->MaxNChannels(ERoute::kInput) : 0;
  oParams.nChannels = GetPlug()->MaxNChannels(ERoute::kOutput);
  
  iParams.firstChannel = 0;
  oParams.firstChannel = 0;

  mBufferSize = iovs;

  DBGMSG("Trying to start audio stream @ %i sr, buffer size %i\nOutput Device = %s (%d ch)\n%s%s",
         sr, mBufferSize,
         GetAudioDeviceName(outID).c_str(), mNumDeviceOutputs,
         hasInput ? "Input Device = " : "",
         hasInput ? (GetAudioDeviceName(inID) + " (" + std::to_string(mNumDeviceInputs) + " ch)\n").c_str() : "");

  RtAudio::StreamOptions options;
  options.flags = RTAUDIO_NONINTERLEAVED;

  // Reset state variables
  mBufIndex = 0;
  mSamplesElapsed = 0;
  mSampleRate = static_cast<double>(sr);
  mVecWait = 0;
  mAudioEnding = false;
  mAudioDone = false;
  
  mIPlug->SetBlockSize(APP_SIGNAL_VECTOR_SIZE);
  mIPlug->SetSampleRate(mSampleRate);
  mIPlug->OnReset();

  auto status = mDAC->openStream(
    &oParams,
    hasInput ? &iParams : nullptr,
    RTAUDIO_FLOAT64,
    sr,
    &mBufferSize,
    &AudioCallback,
    this,
    &options
  );

  // If we fail with requested channels, try with available channels
  if (status != RtAudioErrorType::RTAUDIO_NO_ERROR) {
    
    mDAC->closeStream();
    iParams.nChannels = std::min(iParams.nChannels, (unsigned int)mNumDeviceInputs);
    oParams.nChannels = std::min(oParams.nChannels, (unsigned int)mNumDeviceOutputs);

    auto status = mDAC->openStream(
      &oParams,
      iParams.nChannels > 0 ? &iParams : nullptr,
      RTAUDIO_FLOAT64,
      sr,
      &mBufferSize,
      &AudioCallback,
      this,
      &options
    );

    if (status != RtAudioErrorType::RTAUDIO_NO_ERROR) {
      DBGMSG("%s", mDAC->getErrorText().c_str());
      return false;
    }
  }

  // Allocate buffer pointers
  mInputBufPtrs.Empty();
  mOutputBufPtrs.Empty();

  for (int i = 0; i < GetPlug()->MaxNChannels(ERoute::kInput); i++)
  {
    mInputBufPtrs.Add(nullptr);
  }
    
  for (int i = 0; i < GetPlug()->MaxNChannels(ERoute::kOutput); i++)
  {
    mOutputBufPtrs.Add(nullptr);
  }
    
  if (mDAC->startStream() != RTAUDIO_NO_ERROR)
  {
    DBGMSG("Error starting stream: %s\n", mDAC->getErrorText().c_str());
    return false;
  }

  mActiveSettings = mSettings;

  return true;
}

bool IPlugAPPHost::InitMidi()
{
  try
  {
    mMidiIn = std::make_unique<RtMidiIn>();
  }
  catch (RtMidiError &error)
  {
    mMidiIn = nullptr;
    error.printMessage();
    return false;
  }

  try
  {
    mMidiOut = std::make_unique<RtMidiOut>();
  }
  catch (RtMidiError &error)
  {
    mMidiOut = nullptr;
    error.printMessage();
    return false;
  }

  mMidiIn->setCallback(&MIDICallback, this);
  mMidiIn->ignoreTypes(false, true, false );

  return true;
}

bool IPlugAPPHost::IsMidiInitialised() const
{
  return mMidiIn && mMidiOut;
}

bool IPlugAPPHost::IsAudioRunning() const
{
  return mDAC->isStreamRunning();
}

void ApplyFades(double *pBuffer, int nChans, int nFrames, bool down)
{
  for (int i = 0; i < nChans; i++)
  {
    double *pIO = pBuffer + (i * nFrames);
    
    if (down)
    {
      for (int j = 0; j < nFrames; j++)
        pIO[j] *= ((double) (nFrames - (j + 1)) / (double) nFrames);
    }
    else
    {
      for (int j = 0; j < nFrames; j++)
        pIO[j] *= ((double) j / (double) nFrames);
    }
  }
}

// static
int IPlugAPPHost::AudioCallback(void* pOutputBuffer, void* pInputBuffer, uint32_t nFrames, double streamTime, RtAudioStreamStatus status, void* pUserData)
{
  IPlugAPPHost* _this = (IPlugAPPHost*) pUserData;

  int nins = _this->mNumDeviceInputs;
  int nouts = _this->mNumDeviceOutputs;
  
  double* pInputBufferD = static_cast<double*>(pInputBuffer);
  double* pOutputBufferD = static_cast<double*>(pOutputBuffer);

  bool startWait = _this->mVecWait >= APP_N_VECTOR_WAIT; // wait APP_N_VECTOR_WAIT * iovs before processing audio, to avoid clicks
  bool doFade = _this->mVecWait == APP_N_VECTOR_WAIT || _this->mAudioEnding;
  
  if (startWait && !_this->mAudioDone)
  {
    if (doFade)
      ApplyFades(pInputBufferD, nins, nFrames, _this->mAudioEnding);
    
    for (int i = 0; i < nFrames; i++)
    {
      _this->mBufIndex %= APP_SIGNAL_VECTOR_SIZE;

      if (_this->mBufIndex == 0)
      {
        for (int c = 0; c < nins; c++)
        {
          _this->mInputBufPtrs.Set(c, (pInputBufferD + (c * nFrames)) + i);
        }
        
        for (int c = 0; c < nouts; c++)
        {
          _this->mOutputBufPtrs.Set(c, (pOutputBufferD + (c * nFrames)) + i);
        }
        
        _this->mIPlug->AppProcess(_this->mInputBufPtrs.GetList(), _this->mOutputBufPtrs.GetList(), APP_SIGNAL_VECTOR_SIZE);

        _this->mSamplesElapsed += APP_SIGNAL_VECTOR_SIZE;
      }
      
      for (int c = 0; c < nouts; c++)
      {
        pOutputBufferD[c * nFrames + i] *= APP_MULT;
      }

      _this->mBufIndex++;
    }
    
    if (doFade)
      ApplyFades(pOutputBufferD, nouts, nFrames, _this->mAudioEnding);
    
    if (_this->mAudioEnding)
      _this->mAudioDone = true;
  }
  else
  {
    memset(pOutputBufferD, 0, nFrames * nouts * sizeof(double));
  }
  
  _this->mVecWait = std::min(_this->mVecWait + 1, uint32_t(APP_N_VECTOR_WAIT + 1));

  return 0;
}

// static
void IPlugAPPHost::MIDICallback(double deltatime, std::vector<uint8_t>* pMsg, void* pUserData)
{
  IPlugAPPHost* _this = (IPlugAPPHost*) pUserData;
  
  if (pMsg->size() == 0 || _this->mExiting)
    return;
  
  if (pMsg->size() > 3)
  {
    if (pMsg->size() > MAX_SYSEX_SIZE)
    {
      DBGMSG("SysEx message exceeds MAX_SYSEX_SIZE\n");
      return;
    }
    
    SysExData data { 0, static_cast<int>(pMsg->size()), pMsg->data() };
    
    _this->mIPlug->mSysExMsgsFromCallback.Push(data);
    return;
  }
  else if (pMsg->size())
  {
    IMidiMsg msg;
    msg.mStatus = pMsg->at(0);
    pMsg->size() > 1 ? msg.mData1 = pMsg->at(1) : msg.mData1 = 0;
    pMsg->size() > 2 ? msg.mData2 = pMsg->at(2) : msg.mData2 = 0;

    _this->mIPlug->mMidiMsgsFromCallback.Push(msg);
  }
}

// static
void IPlugAPPHost::ErrorCallback(RtAudioErrorType type, const std::string &errorText)
{
  std::cerr << "\nerrorCallback: " << errorText << "\n\n";
}

#ifdef OS_MAC
void IPlugAPPHost::RegisterDeviceNotifications()
{
  // Register for audio device notifications
  AudioObjectPropertyAddress propertyAddress = {
    kAudioHardwarePropertyDevices,
    kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMain
  };
  
  AudioObjectAddPropertyListener(kAudioObjectSystemObject, 
                               &propertyAddress,
                               AudioDeviceListChanged,
                               this);

  // Register for MIDI device notifications
  MIDIClientRef client;
  MIDIClientCreate(CFSTR("IPlugAPPHost"), MIDIDeviceListChanged, this, &client);
}

void IPlugAPPHost::UnregisterDeviceNotifications()
{
  AudioObjectPropertyAddress propertyAddress = {
    kAudioHardwarePropertyDevices,
    kAudioObjectPropertyScopeGlobal,
    kAudioObjectPropertyElementMain
  };
  
  AudioObjectRemovePropertyListener(kAudioObjectSystemObject,
                                  &propertyAddress,
                                  AudioDeviceListChanged,
                                  this);
}

OSStatus IPlugAPPHost::AudioDeviceListChanged(AudioObjectID inObjectID, 
                                            UInt32 inNumberAddresses, 
                                            const AudioObjectPropertyAddress* inAddresses, 
                                            void* inClientData)
{
  IPlugAPPHost* _this = (IPlugAPPHost*)inClientData;
  _this->OnDeviceListChanged();
  return noErr;
}

void IPlugAPPHost::MIDIDeviceListChanged(const MIDINotification *message, void* refCon)
{
  IPlugAPPHost* _this = (IPlugAPPHost*)refCon;
  if (message->messageID == kMIDIMsgObjectAdded || 
      message->messageID == kMIDIMsgObjectRemoved)
  {
    _this->OnDeviceListChanged();
  }
}

#include "resource.h"

void IPlugAPPHost::OnDeviceListChanged()
{
  // Re-probe audio and MIDI devices
  ProbeAudioIO();
  ProbeMidiIO();
  
  if (gPrefsHWND)
  {
    GetSettingsDialog()->Refresh();
  }
}
#endif

static void ClientResize(HWND hWnd, int nWidth, int nHeight)
{
  RECT rcClient, rcWindow;
  POINT ptDiff;
  int screenwidth, screenheight;
  int x, y;
  
  screenwidth  = GetSystemMetrics(SM_CXSCREEN);
  screenheight = GetSystemMetrics(SM_CYSCREEN);
  x = (screenwidth / 2) - (nWidth / 2);
  y = (screenheight / 2) - (nHeight / 2);
  
  GetClientRect(hWnd, &rcClient);
  GetWindowRect(hWnd, &rcWindow);
  
  ptDiff.x = (rcWindow.right - rcWindow.left) - rcClient.right;
  ptDiff.y = (rcWindow.bottom - rcWindow.top) - rcClient.bottom;
  
  SetWindowPos(hWnd, 0, x, y, nWidth + ptDiff.x, nHeight + ptDiff.y, 0);
}

#ifdef OS_WIN
extern float GetScaleForHWND(HWND hWnd);
#endif

//static
WDL_DLGRET IPlugAPPHost::MainDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  IPlugAPPHost* pAppHost = IPlugAPPHost::sInstance.get();
  
  int width = 0;
  int height = 0;
  
  switch (uMsg)
  {
    case WM_INITDIALOG:
    {
      gHWND = hwndDlg;
      IPlugAPP* pPlug = pAppHost->GetPlug();
      
      if (!pAppHost->OpenWindow(gHWND))
        DBGMSG("couldn't attach gui\n");
      
      width = pPlug->GetEditorWidth();
      height = pPlug->GetEditorHeight();
      
      ClientResize(hwndDlg, width, height);
      
      ShowWindow(hwndDlg, SW_SHOW);
      return 1;
    }
    case WM_DESTROY:
      pAppHost->CloseWindow();
      gHWND = NULL;
      IPlugAPPHost::sInstance = nullptr;
      
#ifdef OS_WIN
      PostQuitMessage(0);
#else
      SWELL_PostQuitMessage(hwndDlg);
#endif
      
      return 0;
    case WM_CLOSE:
      DestroyWindow(hwndDlg);
      return 0;
    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case ID_QUIT:
        {
          DestroyWindow(hwndDlg);
          return 0;
        }
        case ID_ABOUT:
        {
          IPlugAPP* pPlug = pAppHost->GetPlug();
          
          bool pluginOpensAboutBox = pPlug->OnHostRequestingAboutBox();
          
          if (pluginOpensAboutBox == false)
          {
            WDL_String info;
            info.Append(PLUG_COPYRIGHT_STR"\nBuilt on " __DATE__);
            MessageBox(hwndDlg, info.Get(), PLUG_NAME, MB_OK);
          }
          
          return 0;
        }
        case ID_HELP:
        {
          IPlugAPP* pPlug = pAppHost->GetPlug();
          
          bool pluginOpensHelp = pPlug->OnHostRequestingProductHelp();
          
          if (pluginOpensHelp == false)
          {
            MessageBox(hwndDlg, "See the manual", PLUG_NAME, MB_OK);
          }
          return 0;
        }
        case ID_PREFERENCES:
        {
          INT_PTR ret = DialogBox(gHINSTANCE, MAKEINTRESOURCE(IDD_DIALOG_PREF), hwndDlg, pAppHost->mSettingsDialog->GetDlgProc());
          
          if (ret == IDOK)
            pAppHost->UpdateINI();
          
          return 0;
        }
#if defined _DEBUG && !defined NO_IGRAPHICS
        case ID_LIVE_EDIT:
        {
          IGEditorDelegate* pPlug = dynamic_cast<IGEditorDelegate*>(pAppHost->GetPlug());
          
          if (pPlug)
          {
            IGraphics* pGraphics = pPlug->GetUI();
            
            if (pGraphics)
            {
              bool enabled = pGraphics->LiveEditEnabled();
              pGraphics->EnableLiveEdit(!enabled);
              CheckMenuItem(GET_MENU(), ID_LIVE_EDIT, (MF_BYCOMMAND | enabled) ? MF_UNCHECKED : MF_CHECKED);
            }
          }
          
          return 0;
        }
        case ID_SHOW_DRAWN:
        {
          IGEditorDelegate* pPlug = dynamic_cast<IGEditorDelegate*>(pAppHost->GetPlug());
          
          if (pPlug)
          {
            IGraphics* pGraphics = pPlug->GetUI();
            
            if (pGraphics)
            {
              bool enabled = pGraphics->ShowAreaDrawnEnabled();
              pGraphics->ShowAreaDrawn(!enabled);
              CheckMenuItem(GET_MENU(), ID_SHOW_DRAWN, (MF_BYCOMMAND | enabled) ? MF_UNCHECKED : MF_CHECKED);
            }
          }
          
          return 0;
        }
        case ID_SHOW_BOUNDS:
        {
          IGEditorDelegate* pPlug = dynamic_cast<IGEditorDelegate*>(pAppHost->GetPlug());
          
          if (pPlug)
          {
            IGraphics* pGraphics = pPlug->GetUI();
            
            if (pGraphics)
            {
              bool enabled = pGraphics->ShowControlBoundsEnabled();
              pGraphics->ShowControlBounds(!enabled);
              CheckMenuItem(GET_MENU(), ID_SHOW_BOUNDS, (MF_BYCOMMAND | enabled) ? MF_UNCHECKED : MF_CHECKED);
            }
          }
          
          return 0;
        }
        case ID_SHOW_FPS:
        {
          IGEditorDelegate* pPlug = dynamic_cast<IGEditorDelegate*>(pAppHost->GetPlug());
          
          if (pPlug)
          {
            IGraphics* pGraphics = pPlug->GetUI();
            
            if (pGraphics)
            {
              bool enabled = pGraphics->ShowingFPSDisplay();
              pGraphics->ShowFPSDisplay(!enabled);
              CheckMenuItem(GET_MENU(), ID_SHOW_FPS, (MF_BYCOMMAND | enabled) ? MF_UNCHECKED : MF_CHECKED);
            }
          }
          
          return 0;
        }
#endif
      }
      return 0;
    case WM_GETMINMAXINFO:
    {
      if (!pAppHost)
        return 1;
      
      IPlugAPP* pPlug = pAppHost->GetPlug();
      
      MINMAXINFO* mmi = (MINMAXINFO*) lParam;
      mmi->ptMinTrackSize.x = pPlug->GetMinWidth();
      mmi->ptMinTrackSize.y = pPlug->GetMinHeight();
      mmi->ptMaxTrackSize.x = pPlug->GetMaxWidth();
      mmi->ptMaxTrackSize.y = pPlug->GetMaxHeight();
      
#ifdef OS_WIN
      float scale = GetScaleForHWND(hwndDlg);
      mmi->ptMinTrackSize.x = static_cast<LONG>(static_cast<float>(mmi->ptMinTrackSize.x) * scale);
      mmi->ptMinTrackSize.y = static_cast<LONG>(static_cast<float>(mmi->ptMinTrackSize.y) * scale);
      mmi->ptMaxTrackSize.x = static_cast<LONG>(static_cast<float>(mmi->ptMaxTrackSize.x) * scale);
      mmi->ptMaxTrackSize.y = static_cast<LONG>(static_cast<float>(mmi->ptMaxTrackSize.y) * scale);
#endif
      
      return 0;
    }
#ifdef OS_WIN
    case WM_DPICHANGED:
    {
      WORD dpi = HIWORD(wParam);
      RECT* rect = (RECT*)lParam;
      float scale = GetScaleForHWND(hwndDlg);
      
      POINT ptDiff;
      RECT rcClient;
      RECT rcWindow;
      
      GetClientRect(hwndDlg, &rcClient);
      GetWindowRect(hwndDlg, &rcWindow);
      
      ptDiff.x = (rcWindow.right - rcWindow.left) - rcClient.right;
      ptDiff.y = (rcWindow.bottom - rcWindow.top) - rcClient.bottom;
      
#ifndef NO_IGRAPHICS
      IGEditorDelegate* pPlug = dynamic_cast<IGEditorDelegate*>(pAppHost->GetPlug());
      
      if (pPlug)
      {
        IGraphics* pGraphics = pPlug->GetUI();
        
        if (pGraphics)
        {
          pGraphics->SetScreenScale(scale);
        }
      }
#else
      IEditorDelegate* pPlug = dynamic_cast<IEditorDelegate*>(pAppHost->GetPlug());
#endif
      
      int w = pPlug->GetEditorWidth();
      int h = pPlug->GetEditorHeight();
      
      SetWindowPos(hwndDlg, 0, rect->left, rect->top, w + ptDiff.x, h + ptDiff.y, 0);
      
      return 0;
    }
#endif
    case WM_SIZE:
    {
      IPlugAPP* pPlug = pAppHost->GetPlug();
      
      switch (LOWORD(wParam))
      {
        case SIZE_RESTORED:
        case SIZE_MAXIMIZED:
        {
          RECT r;
          GetClientRect(hwndDlg, &r);
          float scale = 1.f;
#ifdef OS_WIN
          scale = GetScaleForHWND(hwndDlg);
#endif
          pPlug->OnParentWindowResize(static_cast<int>(r.right / scale), static_cast<int>(r.bottom / scale));
          return 1;
        }
        default:
          return 0;
      }
    }
  }
  return 0;
}


