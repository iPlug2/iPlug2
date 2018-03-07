#include "IPlugAPP_host.h"

#ifdef OS_WIN
#include <sys/stat.h>
#endif

#include "IPlugLogger.h"

IPlugAPPHost* IPlugAPPHost::sInstance = nullptr;
HWND gHWND;
HINSTANCE gHINST;
UINT gSCROLLMSG;

//static
IPlugAPPHost* IPlugAPPHost::create ()
{
//  IPlugAPPHost* _this = new IPlugAPPHost ();
//  
//  if (_this->init () == false)
//  {
//    delete _this;
//    return nullptr;
//  }
//  else 
//  {
//    sInstance = _this;
//    return _this;
//  }

  return nullptr;
}

IPlugAPPHost::IPlugAPPHost()
{
}

IPlugAPPHost::~IPlugAPPHost()
{
  mMidiIn->cancelCallback();
  
  //shutdown timer

  delete mState;
  delete mTempState;
  delete mActiveState;
  delete mMidiIn;
  delete mMidiOut;
  delete mDAC;
}

//bool IPlugAPPHost::init ()
//{
//  TryToChangeAudioDriverType(); // will init RTAudio with an API type based on gState->mAudioDriverType
//  ProbeAudioIO(); // find out what audio IO devs are available and put their IDs in the global variables gAudioInputDevs / gAudioOutputDevs
//  InitialiseMidi(); // creates RTMidiIn and RTMidiOut objects
//  ProbeMidiIO(); // find out what midi IO devs are available and put their names in the global variables gMidiInputDevs / gMidiOutputDevs
//  ChooseMidiInput(mState->mMidiInDev);
//  ChooseMidiOutput(mState->mMidiOutDev);
//  return true;
//}

//bool IPlugAPPHost::InitialiseState()
//{
//  mState = new AppState();
//  mTempState = new AppState();
//  mActiveState = new AppState();
//
//#ifdef OS_WIN
//  TCHAR strPath[MAX_PATH_LEN];
//  SHGetFolderPathA( NULL, CSIDL_LOCAL_APPDATA, NULL, 0, strPath );
//  mINIPath.Set(strPath, MAX_PATH_LEN);
//#else
//  mINIPath.SetFormatted(MAX_PATH_LEN, "%s/Library/Application Support/%s/", getenv("HOME"), BUNDLE_NAME);
//#endif
//
//  struct stat st;
//
//  if(stat(mINIPath.Get(), &st) == 0) // if directory exists
//  {
//    mINIPath.Append("settings.ini"); // add file name to path
//
//    if(stat(mINIPath.Get(), &st) == 0) // if settings file exists read values into state
//    {
//      mState->mAudioDriverType = GetPrivateProfileInt("audio", "driver", 0, mINIPath.Get());
//
//      GetPrivateProfileString("audio", "indev", "Built-in Input", mState->mAudioInDev, 100, mINIPath.Get());
//      GetPrivateProfileString("audio", "outdev", "Built-in Output", mState->mAudioOutDev, 100, mINIPath.Get());
//
//      //audio
//      mState->mAudioInChanL = GetPrivateProfileInt("audio", "in1", 1, mINIPath.Get()); // 1 is first audio input
//      mState->mAudioInChanR = GetPrivateProfileInt("audio", "in2", 2, mINIPath.Get());
//      mState->mAudioOutChanL = GetPrivateProfileInt("audio", "out1", 1, mINIPath.Get()); // 1 is first audio output
//      mState->mAudioOutChanR = GetPrivateProfileInt("audio", "out2", 2, mINIPath.Get());
//      mState->mAudioInIsMono = GetPrivateProfileInt("audio", "monoinput", 0, mINIPath.Get());
//
//      GetPrivateProfileString("audio", "iovs", "512", mState->mAudioIOVS, 100, mINIPath.Get());
//      GetPrivateProfileString("audio", "sigvs", "32", mState->mAudioSigVS, 100, mINIPath.Get());
//      GetPrivateProfileString("audio", "sr", "44100", mState->mAudioSR, 100, mINIPath.Get());
//
//      //midi
//      GetPrivateProfileString("midi", "indev", "no input", mState->mMidiInDev, 100, mINIPath.Get());
//      GetPrivateProfileString("midi", "outdev", "no output", mState->mMidiOutDev, 100, mINIPath.Get());
//
//      mState->mMidiInChan = GetPrivateProfileInt("midi", "inchan", 0, mINIPath.Get()); // 0 is any
//      mState->mMidiOutChan = GetPrivateProfileInt("midi", "outchan", 0, mINIPath.Get()); // 1 is first chan
//
//      UpdateINI(); // this will write over any invalid values in the file
//    }
//    else // settings file doesn't exist, so populate with default values
//    {
//      UpdateINI();
//    }
//  }
//  else   // folder doesn't exist - make folder and make file
//  {
//#ifdef OS_WIN
//    if (SHGetFolderPathA( NULL, CSIDL_LOCAL_APPDATA, NULL, 0, mINIPath.Get()) != S_OK)
//    {
//      DBGMSG("could not retrieve the user's application data directory!\n");
//
//      return false;
//    }
//#else
//    mode_t process_mask = umask(0);
//    int result_code = mkdir(mINIPath.Get(), S_IRWXU | S_IRWXG | S_IRWXO);
//    umask(process_mask);
//
//    if(!result_code)
//    {
//      sprintf(mINIPath.Get(), "%s%s", mINIPath.Get(), "settings.ini"); // add file name to path
//      UpdateINI(); // will write file if doesn't exist
//    }
//    else
//    {
//      return false;
//    }
//
//#endif
//  }
//
//  return true;
//}
//
//void IPlugAPPHost::UpdateINI()
//{
//  char buf[100]; // temp buffer for writing integers to profile strings
//  const char* ini = mINIPath.Get();
//
//  sprintf(buf, "%u", mState->mAudioDriverType);
//  WritePrivateProfileString("audio", "driver", buf, ini);
//
//  WritePrivateProfileString("audio", "indev", mState->mAudioInDev, ini);
//  WritePrivateProfileString("audio", "outdev", mState->mAudioOutDev, ini);
//
//  sprintf(buf, "%u", mState->mAudioInChanL);
//  WritePrivateProfileString("audio", "in1", buf, ini);
//  sprintf(buf, "%u", mState->mAudioInChanR);
//  WritePrivateProfileString("audio", "in2", buf, ini);
//  sprintf(buf, "%u", mState->mAudioOutChanL);
//  WritePrivateProfileString("audio", "out1", buf, ini);
//  sprintf(buf, "%u", mState->mAudioOutChanR);
//  WritePrivateProfileString("audio", "out2", buf, ini);
//  sprintf(buf, "%u", mState->mAudioInIsMono);
//  WritePrivateProfileString("audio", "monoinput", buf, ini);
//
//  WritePrivateProfileString("audio", "iovs", mState->mAudioIOVS, ini);
//  WritePrivateProfileString("audio", "sigvs", mState->mAudioSigVS, ini);
//
//  WritePrivateProfileString("audio", "sr", mState->mAudioSR, ini);
//
//  WritePrivateProfileString("midi", "indev", mState->mMidiInDev, ini);
//  WritePrivateProfileString("midi", "outdev", mState->mMidiOutDev, ini);
//
//  sprintf(buf, "%u", mState->mMidiInChan);
//  WritePrivateProfileString("midi", "inchan", buf, ini);
//  sprintf(buf, "%u", mState->mMidiOutChan);
//  WritePrivateProfileString("midi", "outchan", buf, ini);
//}

std::string IPlugAPPHost::GetAudioDeviceName(int idx)
{
  return mAudioIDDevNames.at(idx);
}

// TODO: this can be replaced with something sensible
int IPlugAPPHost::GetAudioDeviceIdx(const char* deviceNameToTest)
{
  for(int i = 0; i < mAudioIDDevNames.size(); i++)
  {
    if(!strcmp(deviceNameToTest, mAudioIDDevNames.at(i).c_str() ))
      return i;
  }
  
  return -1;
}

// TODO: make it handle output
uint32_t IPlugAPPHost::GetMIDIPortNumber(ERoute direction, const char* nameToTest)
{
  int start = 1;
  
  if(!strcmp(nameToTest, "off")) return 0;
  
#ifndef OS_WIN
  start = 2;
  if(!strcmp(nameToTest, "virtual input")) return 1;
#endif
  
  for (int i = 0; i < mMidiIn->getPortCount(); i++)
  {
    if(!strcmp(nameToTest, mMidiIn->getPortName(i).c_str()))
      return (i + start);
  }
  
  return -1;
}

//UINT IPlugAPPHost::GetMIDIOutPortNumber(const char* nameToTest)
//{
//  int start = 1;
//  
//  if(!strcmp(nameToTest, "off")) return 0;
//  
//#ifndef OS_WIN
//  start = 2;
//  if(!strcmp(nameToTest, "virtual output")) return 1;
//#endif
//  
//  for (int i = 0; i < mMidiOut->getPortCount(); i++)
//  {
//    if(!strcmp(nameToTest, mMidiOut->getPortName(i).c_str()))
//      return (i + start);
//  }
//  
//  return -1;
//}

// find out which devices have input channels & which have output channels, add their ids to the lists
//void IPlugAPPHost::ProbeAudioIO()
//{
//  RtAudio::DeviceInfo info;
//
//  mAudioInputDevs.clear();
//  mAudioOutputDevs.clear();
//  mAudioIDDevNames.clear();
//
//  UINT nDevices = mDAC->getDeviceCount();
//
//  for (int i=0; i<nDevices; i++)
//  {
//    info = mDAC->getDeviceInfo(i);
//    std::string deviceName = info.name;
//
//#ifndef OS_WIN
//    size_t colonIdx = deviceName.rfind(": ");
//
//    if(colonIdx != std::string::npos && deviceName.length() >= 2)
//      deviceName = deviceName.substr(colonIdx + 2, deviceName.length() - colonIdx - 2);
//
//#endif
//
//    mAudioIDDevNames.push_back(deviceName);
//
//    if ( info.probed == false )
//      std::cout << deviceName << ": Probe Status = Unsuccessful\n";
//    else if ( !strcmp("Generic Low Latency ASIO Driver", deviceName.c_str() ))
//      std::cout << deviceName << ": Probe Status = Unsuccessful\n";
//    else
//    {
//      if(info.inputChannels > 0)
//        mAudioInputDevs.push_back(i);
//
//      if(info.outputChannels > 0)
//        mAudioOutputDevs.push_back(i);
//
//      if (info.isDefaultInput)
//        mDefaultInputDev = i;
//
//      if (info.isDefaultOutput)
//        mDefaultOutputDev = i;
//    }
//  }
//}
//
//void IPlugAPPHost::ProbeMidiIO()
//{
//  if ( !mMidiIn || !mMidiOut )
//    return;
//  else
//  {
//    int nInputPorts = mMidiIn->getPortCount();
//
//    mMidiInputDevNames.push_back("off");
//
//#ifndef OS_WIN
//    mMidiInputDevNames.push_back("virtual input");
//#endif
//
//    for (int i=0; i<nInputPorts; i++ )
//    {
//      mMidiInputDevNames.push_back(mMidiIn->getPortName(i));
//    }
//
//    int nOutputPorts = mMidiOut->getPortCount();
//
//    mMidiOutputDevNames.push_back("off");
//
//#ifndef OS_WIN
//    mMidiOutputDevNames.push_back("virtual output");
//#endif
//
//    for (int i=0; i<nOutputPorts; i++ )
//    {
//      mMidiOutputDevNames.push_back(mMidiOut->getPortName(i));
//      //This means the virtual output port wont be added as an input
//    }
//  }
//}
//
//bool IPlugAPPHost::AudioSettingsInstanceateAreEqual(AppState* os, AppState* ns)
//{
//  if (os->mAudioDriverType != ns->mAudioDriverType) return false;
//  if (strcmp(os->mAudioInDev, ns->mAudioInDev)) return false;
//  if (strcmp(os->mAudioOutDev, ns->mAudioOutDev)) return false;
//  if (strcmp(os->mAudioSR, ns->mAudioSR)) return false;
//  if (strcmp(os->mAudioIOVS, ns->mAudioIOVS)) return false;
//  if (strcmp(os->mAudioSigVS, ns->mAudioSigVS)) return false;
//  if (os->mAudioInChanL != ns->mAudioInChanL) return false;
//  if (os->mAudioInChanR != ns->mAudioInChanR) return false;
//  if (os->mAudioOutChanL != ns->mAudioOutChanL) return false;
//  if (os->mAudioOutChanR != ns->mAudioOutChanR) return false;
//  if (os->mAudioInIsMono != ns->mAudioInIsMono) return false;
//
//  return true;
//}
//
//bool IPlugAPPHost::MIDISettingsInstanceateAreEqual(AppState* os, AppState* ns)
//{
//  if (strcmp(os->mMidiInDev, ns->mMidiInDev)) return false;
//  if (strcmp(os->mMidiOutDev, ns->mMidiOutDev)) return false;
//  if (os->mMidiInChan != ns->mMidiInChan) return false;
//  if (os->mMidiOutChan != ns->mMidiOutChan) return false;
//
//  return true;
//}
//
//bool IPlugAPPHost::TryToChangeAudioDriverType()
//{
//  if (mDAC)
//  {
//    if (mDAC->isStreamOpen())
//    {
//      mDAC->closeStream();
//    }
//
//    delete mDAC;
//    mDAC = 0;
//  }
//
//#ifdef OS_WIN
//  if(mState->mAudioDriverType == DAC_ASIO)
//    mDAC = new RtAudio(RtAudio::WINDOWS_ASIO);
//  else
//    mDAC = new RtAudio(RtAudio::WINDOWS_DS);
//#else // OSX
//  if(mState->mAudioDriverType == DAC_COREAUDIO)
//    mDAC = new RtAudio(RtAudio::MACOSX_CORE);
//  //else
//  //mDAC = new RtAudio(RtAudio::UNIX_JACK);
//#endif
//
//  if(mDAC)
//    return true;
//  else
//    return false;
//}
//
//bool IPlugAPPHost::TryToChangeAudio()
//{
//  int inputID = -1;
//  int outputID = -1;
//
//#ifdef OS_WIN
//  if(mState->mAudioDriverType == DAC_ASIO)
//    inputID = GetAudioDeviceID(mState->mAudioOutDev);
//  else
//    inputID = GetAudioDeviceID(mState->mAudioInDev);
//#else // OSX
//  inputID = GetAudioDeviceID(mState->mAudioInDev);
//#endif
//
//  outputID = GetAudioDeviceID(mState->mAudioOutDev);
//
//  bool failedToFindDevice = false;
//  bool resetToDefault = false;
//
//  if (inputID == -1)
//  {
//    if (mDefaultInputDev > -1)
//    {
//      resetToDefault = true;
//      inputID = mDefaultInputDev;
//
//      if (mAudioInputDevs.size())
//        strcpy(mState->mAudioInDev, GetAudioDeviceName(inputID).c_str());
//    }
//    else
//      failedToFindDevice = true;
//  }
//
//  if (outputID == -1)
//  {
//    if (mDefaultOutputDev > -1)
//    {
//      resetToDefault = true;
//
//      outputID = mDefaultOutputDev;
//
//      if (mAudioOutputDevs.size())
//        strcpy(mState->mAudioOutDev, GetAudioDeviceName(outputID).c_str());
//    }
//    else
//      failedToFindDevice = true;
//  }
//
//  if (resetToDefault)
//  {
//    DBGMSG("couldn't find previous audio device, reseting to default\n");
//
//    UpdateINI();
//  }
//
//  if (failedToFindDevice)
//    MessageBox(gHWND, "Please check your soundcard settings in Preferences", "Error", MB_OK);
//
//  int samplerate = atoi(mState->mAudioSR);
//  int iovs = atoi(mState->mAudioIOVS);
//
//  if (inputID != -1 && outputID != -1)
//  {
//    return InitialiseAudio(inputID, outputID, samplerate, iovs, APP_NUM_CHANNELS, mState->mAudioInChanL - 1, mState->mAudioOutChanL - 1);
//  }
//
//  return false;
//}
//
//bool IPlugAPPHost::ChooseMidiInput(const char* pPortName)
//{
//  UINT port = GetMIDIInPortNumber(pPortName);
//
//  if(port == -1)
//  {
//    strcpy(mState->mMidiInDev, "off");
//    UpdateINI();
//    port = 0;
//  }
//
//  //TODO: send all notes off?
//
//  if (mMidiIn)
//  {
//    mMidiIn->closePort();
//
//    if (port == 0)
//    {
//      return true;
//    }
//#ifdef OS_WIN
//    else
//    {
//      mMidiIn->openPort(port-1);
//      return true;
//    }
//#else // OSX
//    else if(port == 1)
//    {
//      std::string virtualMidiInputName = "To ";
//      virtualMidiInputName += BUNDLE_NAME;
//      mMidiIn->openVirtualPort(virtualMidiInputName);
//      return true;
//    }
//    else
//    {
//      mMidiIn->openPort(port-2);
//      return true;
//    }
//#endif
//  }
//
//  return false;
//}
//
//bool IPlugAPPHost::ChooseMidiOutput(const char* portName)
//{
//  UINT port = GetMIDIOutPortNumber(portName);
//
//  if(port == -1)
//  {
//    strcpy(mState->mMidiOutDev, "off");
//    UpdateINI();
//    port = 0;
//  }
//
//  if (mMidiOut)
//  {
//    //TODO: send all notes off?
//    mMidiOut->closePort();
//
//    if (port == 0)
//      return true;
//#ifdef OS_WIN
//    else
//    {
//      mMidiOut->openPort(port-1);
//      return true;
//    }
//#elif defined (OS_MAC)
//    else if(port == 1)
//    {
//      std::string virtualMidiOutputName = "From ";
//      virtualMidiOutputName += BUNDLE_NAME;
//      mMidiOut->openVirtualPort(virtualMidiOutputName);
//      return true;
//    }
//    else
//    {
//      mMidiOut->openPort(port-2);
//      return true;
//    }
//#elif defined (OS_LINUX)
//
//#endif
//  }
//
//  return false;
//}

//bool IPlugAPPHost::InitialiseAudio(uint32_t inId, uint32_t outId, uint32_t sr, uint32_t iovs, uint32_t chnls, uint32_t inChanL, uint32_t outChanL)
//{
//  if (mDAC->isStreamOpen())
//  {
//    if (mDAC->isStreamRunning())
//    {
//      try
//      {
//        mDAC->abortStream();
//      }
//      catch (RtAudioError& e)
//      {
//        e.printMessage();
//      }
//    }
//
//    mDAC->closeStream();
//  }
//
//  RtAudio::StreamParameters iParams, oParams;
//  iParams.deviceId = inId;
//  iParams.nChannels = chnls;
//  iParams.firstChannel = inChanL;
//
//  oParams.deviceId = outId;
//  oParams.nChannels = chnls;
//  oParams.firstChannel = outChanL;
//
////  mIOVS = iovs; // gIOVS may get changed by stream
////  mSigVS = mState->mAudioSigVS; // This is done here so that it changes when the callback is stopped
//
//  //DBGMSG("\ntrying to start audio stream @ %i sr, %i iovs, %i sigvs\nindev = %i:%s\noutdev = %i:%s\n", sr, iovs, gSigVS, inId, GetAudioDeviceName(inId).c_str(), outId, GetAudioDeviceName(outId).c_str());
//
//  RtAudio::StreamOptions options;
//  options.flags = RTAUDIO_NONINTERLEAVED;
//  // options.streamName = BUNDLE_NAME; // JACK stream name, not used on other streams
//
//  mBufIndex = 0;
//  mVecElapsed = 0;
//  mFadeMult = 0.;
//  mSampleRate = (double) sr;
//
//  try
//  {
//    mDAC->openStream(&oParams, &iParams, RTAUDIO_FLOAT64, sr, &mIOVS, &AudioCallback, NULL, &options /*, &ErrorCallback */);
//    mDAC->startStream();
//
//    memcpy(mActiveState, mState, sizeof(AppState)); // copy state to active state
//  }
//  catch (RtAudioError& e)
//  {
//    e.printMessage();
//    return false;
//  }
//
//  return true;
//}

//bool IPlugAPPHost::InitialiseMidi()
//{
//  try
//  {
//    mMidiIn = new RtMidiIn();
//  }
//  catch ( RtMidiError &error )
//  {
//    delete mMidiIn;
//    mMidiIn = 0;
//    error.printMessage();
//    return false;
//  }
//
//  try
//  {
//    mMidiOut = new RtMidiOut();
//  }
//  catch ( RtMidiError &error )
//  {
//    delete mMidiOut;
//    mMidiOut = 0;
//    error.printMessage();
//    return false;
//  }
//
//  mMidiIn->setCallback( &MIDICallback );
//  mMidiIn->ignoreTypes( !APP_ENABLE_SYSEX, !APP_ENABLE_MIDICLOCK, !APP_ENABLE_ACTIVE_SENSING );
//
//  return true;
//}
// static 
int IPlugAPPHost::AudioCallback(void* pOutputBuffer, void* pInputBuffer, uint32_t nFrames, double streamTime, RtAudioStreamStatus status, void* pUserData)
{
//  if ( status )
//    std::cout << "Stream underflow detected!" << std::endl;
//
//  IPlugAPPHost* _this = sInstance;
//
//  AppState* mState = _this->mState;
//
//  double* inputBufferD = (double*)inputBuffer;
//  double* outputBufferD = (double*)outputBuffer;
//
//  int inRightOffset = 0;
//
//  if(!mState->mAudioInIsMono)
//    inRightOffset = nFrames;
//
//  if (_this->mVecElapsed > APP_N_VECTOR_WAIT) // wait APP_N_VECTOR_WAIT * iovs before processing audio, to avoid clicks
//  {
//    for (int i=0; i<nFrames; i++)
//    {
//      _this->mBufIndex %= _this->mSigVS;
//
//      if (_this->mBufIndex == 0)
//      {
//        double* inputs[2] = {inputBufferD + i, inputBufferD + inRightOffset + i};
//        double* outputs[2] = {outputBufferD + i, outputBufferD + nFrames + i};
//
//        _this->setProcessingBuffers<double>(inputs, outputs);
//
//        Event midiEvent;
//        while (_this->mMidiInputMessages.pop(midiEvent))
//        {
//          _this->inputEvents->addEvent (midiEvent);
//        }
//
//        _this->doProcess(_this->mSigVS);
//
//        _this->mSamplesElapsed += _this->mSigVS;
//      }
//
//      // fade in
//      if (_this->mFadeMult < 1.)
//      {
//        _this->mFadeMult += (1. / nFrames);
//      }
//
//      outputBufferD[i] *= _this->mFadeMult;
//      outputBufferD[i + nFrames] *= _this->mFadeMult;
//
//      outputBufferD[i] *= APP_MULT;
//      outputBufferD[i + nFrames] *= APP_MULT;
//
//      _this->mBufIndex++;
//    }
//  }
//  else
//  {
//    memset(outputBuffer, 0, nFrames * APP_NUM_CHANNELS * sizeof(double));
//  }
//
//  _this->mVecElapsed++;
//
  return 0;
}

// static
void IPlugAPPHost::MIDICallback(double deltatime, std::vector<uint8_t>* pMsg, void* pUserData)
{
  IPlugAPPHost* _this = sInstance;
  
  //TODO:
}

// static
void IPlugAPPHost::ErrorCallback(RtAudioError::Type type, const std::string &errorText )
{
  //TODO:
}

