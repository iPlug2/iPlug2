#include "app_main.h"

#ifdef OS_WIN
  #include <windows.h>
  #include <shlobj.h>
  #include <sys/stat.h>
#endif

HWND gHWND;

HINSTANCE gHINST;
UINT gScrollMessage;
IPlug* gPluginInstance;
RtAudio* gDAC = 0;
RtMidiIn *gMidiIn = 0;
RtMidiOut *gMidiOut = 0;

AppState *gState;
AppState *gTempState;
AppState *gActiveState;

char *gINIPath = new char[200]; // path of ini file

unsigned int gIOVS = 512;
unsigned int gSigVS = 32;
unsigned int gBufIndex = 0; // Loops 0 to SigVS
unsigned int gVecElapsed = 0;
double gFadeMult = 0.; // Fade multiplier

std::vector<unsigned int> gAudioInputDevs;
std::vector<unsigned int> gAudioOutputDevs;
std::vector<std::string> gMIDIInputDevNames;
std::vector<std::string> gMIDIOutputDevNames;
std::vector<std::string> gAudioIDDevNames;

void UpdateINI()
{
  char buf[100]; // temp buffer for writing integers to profile strings

  sprintf(buf, "%u", gState->mAudioDriverType);
  WritePrivateProfileString("audio", "driver", buf, gINIPath);

  WritePrivateProfileString("audio", "indev", gState->mAudioInDev, gINIPath);
  WritePrivateProfileString("audio", "outdev", gState->mAudioOutDev, gINIPath);

  sprintf(buf, "%u", gState->mAudioInChanL);
  WritePrivateProfileString("audio", "in1", buf, gINIPath);
  sprintf(buf, "%u", gState->mAudioInChanR);
  WritePrivateProfileString("audio", "in2", buf, gINIPath);
  sprintf(buf, "%u", gState->mAudioOutChanL);
  WritePrivateProfileString("audio", "out1", buf, gINIPath);
  sprintf(buf, "%u", gState->mAudioOutChanR);
  WritePrivateProfileString("audio", "out2", buf, gINIPath);
  sprintf(buf, "%u", gState->mAudioInIsMono);
  WritePrivateProfileString("audio", "monoinput", buf, gINIPath);

  WritePrivateProfileString("audio", "iovs", gState->mAudioIOVS, gINIPath);
  WritePrivateProfileString("audio", "sigvs", gState->mAudioSigVS, gINIPath);

  WritePrivateProfileString("audio", "sr", gState->mAudioSR, gINIPath);

  WritePrivateProfileString("midi", "indev", gState->mMidiInDev, gINIPath);
  WritePrivateProfileString("midi", "outdev", gState->mMidiOutDev, gINIPath);

  sprintf(buf, "%u", gState->mMidiInChan);
  WritePrivateProfileString("midi", "inchan", buf, gINIPath);
  sprintf(buf, "%u", gState->mMidiOutChan);
  WritePrivateProfileString("midi", "outchan", buf, gINIPath);
}

// returns the device name. Core Audio device names are truncated
std::string GetAudioDeviceName(int idx)
{
  return gAudioIDDevNames.at(idx);
}

// returns the rtaudio device ID, based on the (truncated) device name
int GetAudioDeviceID(char* deviceNameToTest)
{
  TRACE;

  for(int i = 0; i < gAudioIDDevNames.size(); i++)
  {
    if(!strcmp(deviceNameToTest, gAudioIDDevNames.at(i).c_str() ))
      return i;
  }

  return -1;
}

unsigned int GetMIDIInPortNumber(const char* nameToTest)
{
  int start = 1;

  if(!strcmp(nameToTest, "off")) return 0;

  #ifdef OS_OSX
  start = 2;
  if(!strcmp(nameToTest, "virtual input")) return 1;
  #endif

  for (int i = 0; i < gMidiIn->getPortCount(); i++)
  {
    if(!strcmp(nameToTest, gMidiIn->getPortName(i).c_str()))
      return (i + start);
  }

  return -1;
}

unsigned int GetMIDIOutPortNumber(const char* nameToTest)
{
  int start = 1;

  if(!strcmp(nameToTest, "off")) return 0;

  #ifdef OS_OSX
  start = 2;
  if(!strcmp(nameToTest, "virtual output")) return 1;
  #endif

  for (int i = 0; i < gMidiOut->getPortCount(); i++)
  {
    if(!strcmp(nameToTest, gMidiOut->getPortName(i).c_str()))
      return (i + start);
  }

  return -1;
}

// find out which devices have input channels & which have output channels, add their ids to the lists
void ProbeAudioIO()
{
  TRACE;

  RtAudio::DeviceInfo info;

  gAudioInputDevs.clear();
  gAudioOutputDevs.clear();
  gAudioIDDevNames.clear();

  unsigned int nDevices = gDAC->getDeviceCount();

  for (int i=0; i<nDevices; i++)
  {
    info = gDAC->getDeviceInfo(i);
    std::string deviceName = info.name;

    #ifdef OS_OSX
    size_t colonIdx = deviceName.rfind(": ");

    if(colonIdx != std::string::npos && deviceName.length() >= 2)
      deviceName = deviceName.substr(colonIdx + 2, deviceName.length() - colonIdx - 2);

    #endif

    gAudioIDDevNames.push_back(deviceName);

    if ( info.probed == false )
      std::cout << deviceName << ": Probe Status = Unsuccessful\n";
    else if ( !strcmp("Generic Low Latency ASIO Driver", deviceName.c_str() ))
      std::cout << deviceName << ": Probe Status = Unsuccessful\n";
    else
    {
      if(info.inputChannels > 0)
        gAudioInputDevs.push_back(i);

      if(info.outputChannels > 0)
        gAudioOutputDevs.push_back(i);
    }
  }
}

void ProbeMidiIO()
{
  if ( !gMidiIn || !gMidiOut )
    return;
  else
  {
    int nInputPorts = gMidiIn->getPortCount();

    gMIDIInputDevNames.push_back("off");

    #ifndef OS_WIN
    gMIDIInputDevNames.push_back("virtual input");
    #endif

    for (int i=0; i<nInputPorts; i++ )
    {
      gMIDIInputDevNames.push_back(gMidiIn->getPortName(i));
    }

    int nOutputPorts = gMidiOut->getPortCount();

    gMIDIOutputDevNames.push_back("off");

#ifndef _WIN32
    gMIDIOutputDevNames.push_back("virtual output");
#endif

    for (int i=0; i<nOutputPorts; i++ )
    {
      gMIDIOutputDevNames.push_back(gMidiOut->getPortName(i));
      //This means the virtual output port wont be added as an input
    }
  }
}

bool AudioSettingsInStateAreEqual(AppState* os, AppState* ns)
{
  if (os->mAudioDriverType != ns->mAudioDriverType) return false;
  if (strcmp(os->mAudioInDev, ns->mAudioInDev)) return false;
  if (strcmp(os->mAudioOutDev, ns->mAudioOutDev)) return false;
  if (strcmp(os->mAudioSR, ns->mAudioSR)) return false;
  if (strcmp(os->mAudioIOVS, ns->mAudioIOVS)) return false;
  if (strcmp(os->mAudioSigVS, ns->mAudioSigVS)) return false;
  if (os->mAudioInChanL != ns->mAudioInChanL) return false;
  if (os->mAudioInChanR != ns->mAudioInChanR) return false;
  if (os->mAudioOutChanL != ns->mAudioOutChanL) return false;
  if (os->mAudioOutChanR != ns->mAudioOutChanR) return false;
  if (os->mAudioInIsMono != ns->mAudioInIsMono) return false;

  return true;
}

bool MIDISettingsInStateAreEqual(AppState* os, AppState* ns)
{
  if (strcmp(os->mMidiInDev, ns->mMidiInDev)) return false;
  if (strcmp(os->mMidiOutDev, ns->mMidiOutDev)) return false;
  if (os->mMidiInChan != ns->mMidiInChan) return false;
  if (os->mMidiOutChan != ns->mMidiOutChan) return false;

  return true;
}

void MIDICallback( double deltatime, std::vector< unsigned char > *message, void *userData )
{
  if ( message->size() )
  {
    IMidiMsg *myMsg;

    switch (message->size())
    {
      case 1:
        myMsg = new IMidiMsg(0, message->at(0), 0, 0);
        break;
      case 2:
        myMsg = new IMidiMsg(0, message->at(0), message->at(1), 0);
        break;
      case 3:
        myMsg = new IMidiMsg(0, message->at(0), message->at(1), message->at(2));
        break;
      default:
        DBGMSG("NOT EXPECTING %d midi callback msg len\n", (int) message->size());
        break;
    }

    IMidiMsg msg(*myMsg);

    delete myMsg;

    // filter midi messages based on channel, if gStatus.mMidiInChan != all (0)
    if (gState->mMidiInChan)
    {
      if (gState->mMidiInChan == msg.Channel() + 1 )
        gPluginInstance->ProcessMidiMsg(&msg);
    }
    else
    {
      gPluginInstance->ProcessMidiMsg(&msg);
    }
  }
}

int AudioCallback(void *outputBuffer,
                  void *inputBuffer,
                  unsigned int nFrames,
                  double streamTime,
                  RtAudioStreamStatus status,
                  void *userData )
{
  if ( status )
    std::cout << "Stream underflow detected!" << std::endl;

  double* inputBufferD = (double*)inputBuffer;
  double* outputBufferD = (double*)outputBuffer;

  int inRightOffset = 0;

  if(!gState->mAudioInIsMono)
    inRightOffset = nFrames;

  if (gVecElapsed > N_VECTOR_WAIT) // wait N_VECTOR_WAIT * iovs before processing audio, to avoid clicks
  {
    for (int i=0; i<nFrames; i++)
    {
      gBufIndex %= gSigVS;

      if (gBufIndex == 0)
      {
        double* inputs[2] = {inputBufferD + i, inputBufferD + inRightOffset + i};
        double* outputs[2] = {outputBufferD + i, outputBufferD + nFrames + i};

        gPluginInstance->LockMutexAndProcessDoubleReplacing(inputs, outputs, gSigVS);
      }

      // fade in
      if (gFadeMult < 1.)
      {
        gFadeMult += (1. / nFrames);
      }

      outputBufferD[i] *= gFadeMult;
      outputBufferD[i + nFrames] *= gFadeMult;

      outputBufferD[i] *= APP_MULT;
      outputBufferD[i + nFrames] *= APP_MULT;

      gBufIndex++;
    }
  }
  else
  {
    memset(outputBuffer, 0, nFrames * 2 * sizeof(double));
  }

  gVecElapsed++;

  return 0;
}

bool TryToChangeAudioDriverType()
{
  TRACE;

  if (gDAC)
  {
    if (gDAC->isStreamOpen())
    {
      gDAC->closeStream();
    }

    DELETE_NULL(gDAC);
  }

#ifdef OS_WIN
  if(gState->mAudioDriverType == DAC_ASIO)
    gDAC = new RtAudio(RtAudio::WINDOWS_ASIO);
  else
    gDAC = new RtAudio(RtAudio::WINDOWS_DS);
#elif defined OS_OSX
  if(gState->mAudioDriverType == DAC_COREAUDIO)
    gDAC = new RtAudio(RtAudio::MACOSX_CORE);
  //else
  //gDAC = new RtAudio(RtAudio::UNIX_JACK);
#endif

  if(gDAC)
    return true;
  else
    return false;
}

bool TryToChangeAudio()
{
  TRACE;

  int inputID = -1;
  int outputID = -1;

#ifdef OS_WIN
  if(gState->mAudioDriverType == DAC_ASIO)
    inputID = GetAudioDeviceID(gState->mAudioOutDev);
  else
    inputID = GetAudioDeviceID(gState->mAudioInDev);
#else
  inputID = GetAudioDeviceID(gState->mAudioInDev);
#endif

  outputID = GetAudioDeviceID(gState->mAudioOutDev);

  int samplerate = atoi(gState->mAudioSR);
  int iovs = atoi(gState->mAudioIOVS);

  if (inputID != -1 && outputID != -1)
  {
    return InitialiseAudio(inputID, outputID, samplerate, iovs, NUM_CHANNELS, gState->mAudioInChanL - 1, gState->mAudioOutChanL - 1);
  }

  return false;
}

bool InitialiseAudio(unsigned int inId,
                     unsigned int outId,
                     unsigned int sr,
                     unsigned int iovs,
                     unsigned int chnls,
                     unsigned int inChanL,
                     unsigned int outChanL
                    )
{
  TRACE;

  if (gDAC->isStreamOpen())
  {
    if (gDAC->isStreamRunning())
    {
      try
      {
        gDAC->abortStream();
      }
      catch (RtError& e)
      {
        e.printMessage();
      }
    }

    gDAC->closeStream();
  }

  RtAudio::StreamParameters iParams, oParams;
  iParams.deviceId = inId;
  iParams.nChannels = chnls;
  iParams.firstChannel = inChanL;

  oParams.deviceId = outId;
  oParams.nChannels = chnls;
  oParams.firstChannel = outChanL;

  gIOVS = iovs; // gIOVS may get changed by stream
  gSigVS = atoi(gState->mAudioSigVS); // This is done here so that it changes when the callback is stopped

  DBGMSG("\ntrying to start audio stream @ %i sr, %i iovs, %i sigvs\nindev = %i:%s\noutdev = %i:%s\n", sr, iovs, gSigVS, inId, GetAudioDeviceName(inId).c_str(), outId, GetAudioDeviceName(outId).c_str());

  RtAudio::StreamOptions options;
  options.flags = RTAUDIO_NONINTERLEAVED;
// options.streamName = BUNDLE_NAME; // JACK stream name, not used on other streams

  gBufIndex = 0;
  gVecElapsed = 0;
  gFadeMult = 0.;

  gPluginInstance->SetBlockSize(gSigVS);
  gPluginInstance->SetSampleRate(sr);
  gPluginInstance->Reset();

  try
  {
    TRACE;
    gDAC->openStream( &oParams, &iParams, RTAUDIO_FLOAT64, sr, &gIOVS, &AudioCallback, NULL, &options);
    gDAC->startStream();

    memcpy(gActiveState, gState, sizeof(AppState)); // copy state to active state
  }
  catch ( RtError& e )
  {
    e.printMessage();
    return false;
  }

  return true;
}

bool InitialiseMidi()
{
  try
  {
    gMidiIn = new RtMidiIn();
  }
  catch ( RtError &error )
  {
    FREE_NULL(gMidiIn);
    error.printMessage();
    return false;
  }

  try
  {
    gMidiOut = new RtMidiOut();
  }
  catch ( RtError &error )
  {
    FREE_NULL(gMidiOut);
    error.printMessage();
    return false;
  }

  gMidiIn->setCallback( &MIDICallback );
  gMidiIn->ignoreTypes( !ENABLE_SYSEX, !ENABLE_MIDICLOCK, !ENABLE_ACTIVE_SENSING );

  return true;
}

bool ChooseMidiInput(const char* pPortName)
{
  unsigned int port = GetMIDIInPortNumber(pPortName);

  if(port == -1)
  {
    strcpy(gState->mMidiInDev, "off");
    UpdateINI();
    port = 0;
  }
  /*
  IMidiMsg msg;
  msg.MakeControlChangeMsg(IMidiMsg::kAllNotesOff, 127, 0);

  std::vector<unsigned char> message;
  message.push_back( msg.mStatus );
  message.push_back( msg.mData1 );
  message.push_back( msg.mData2 );

  gPluginInstance->ProcessMidiMsg(&msg);
  */
  if (gMidiIn)
  {
    gMidiIn->closePort();

    if (port == 0)
    {
      return true;
    }
    #ifdef OS_WIN
    else
    {
      gMidiIn->openPort(port-1);
      return true;
    }
    #else
    else if(port == 1)
    {
      std::string virtualMidiInputName = "To ";
      virtualMidiInputName += BUNDLE_NAME;
      gMidiIn->openVirtualPort(virtualMidiInputName);
      return true;
    }
    else
    {
      gMidiIn->openPort(port-2);
      return true;
    }
    #endif
  }

  return false;
}

bool ChooseMidiOutput(const char* pPortName)
{
  unsigned int port = GetMIDIOutPortNumber(pPortName);

  if(port == -1)
  {
    strcpy(gState->mMidiOutDev, "off");
    UpdateINI();
    port = 0;
  }

  if (gMidiOut)
  {
    /*
    IMidiMsg msg;
    msg.MakeControlChangeMsg(IMidiMsg::kAllNotesOff, 127, 0);

    std::vector<unsigned char> message;
    message.push_back( msg.mStatus );
    message.push_back( msg.mData1 );
    message.push_back( msg.mData2 );

    gMidiOut->sendMessage( &message );
    */
    gMidiOut->closePort();

    if (port == 0)
    {
      return true;
    }
    #ifdef OS_WIN
    else
    {
      gMidiOut->openPort(port-1);
      return true;
    }
    #else
    else if(port == 1)
    {
      std::string virtualMidiOutputName = "From ";
      virtualMidiOutputName += BUNDLE_NAME;
      gMidiOut->openVirtualPort(virtualMidiOutputName);
      return true;
    }
    else
    {
      gMidiOut->openPort(port-2);
      return true;
    }
    #endif
  }

  return false;
}

extern bool AttachGUI()
{
  IGraphics* pGraphics = gPluginInstance->GetGUI();

  if (pGraphics)
  {
#ifdef OS_WIN
    if (!pGraphics->OpenWindow(gHWND))
      pGraphics=0;
#else // Cocoa OSX
    if (!pGraphics->OpenWindow(gHWND))
      pGraphics=0;
#endif
    if (pGraphics)
    {
      gPluginInstance->OnGUIOpen();
      return true;
    }
  }

  return false;
}

void Init()
{
  TryToChangeAudioDriverType(); // will init RTAudio with an API type based on gState->mAudioDriverType
  ProbeAudioIO(); // find out what audio IO devs are available and put their IDs in the global variables gAudioInputDevs / gAudioOutputDevs
  InitialiseMidi(); // creates RTMidiIn and RTMidiOut objects
  ProbeMidiIO(); // find out what midi IO devs are available and put their names in the global variables gMidiInputDevs / gMidiOutputDevs

  // Initialise the plugin
  gPluginInstance = MakePlug(gMidiOut, &gState->mMidiOutChan);
  gPluginInstance->RestorePreset(0);

  ChooseMidiInput(gState->mMidiInDev);
  ChooseMidiOutput(gState->mMidiOutDev);

  TryToChangeAudio();
}

void Cleanup()
{
  try
  {
    // Stop the stream
    gDAC->stopStream();
  }
  catch (RtError& e)
  {
    e.printMessage();
  }

  gMidiIn->cancelCallback();
  gMidiIn->closePort();
  gMidiOut->closePort();

  if ( gDAC->isStreamOpen() ) gDAC->closeStream();

  delete gPluginInstance;
  delete gState;
  delete gTempState;
  delete gActiveState;
  delete gMidiIn;
  delete gMidiOut;
  delete gDAC;
  delete [] gINIPath;
}

#ifdef OS_WIN
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nShowCmd)
{
  // first check to make sure this is the only instance running
  // http://www.bcbjournal.org/articles/vol3/9911/Single-instance_applications.htm
  try
  {
    // Try to open the mutex.
    HANDLE hMutex = OpenMutex(
                      MUTEX_ALL_ACCESS, 0, BUNDLE_NAME);

    // If hMutex is 0 then the mutex doesn't exist.
    if (!hMutex)
      hMutex = CreateMutex(0, 0, BUNDLE_NAME);
    else
    {
      // This is a second instance. Bring the
      // original instance to the top.
      HWND hWnd = FindWindow(0, BUNDLE_NAME);
      SetForegroundWindow(hWnd);

      return 0;
    }

    gHINST=hInstance;

    InitCommonControls();
    gScrollMessage = RegisterWindowMessage("MSWHEEL_ROLLMSG");

    gState = new AppState();
    gTempState = new AppState();
    gActiveState = new AppState();

    if (SHGetFolderPathA( NULL, CSIDL_LOCAL_APPDATA, NULL, 0, gINIPath ) != S_OK)
    {
      DBGMSG("could not retrieve the user's application data directory!\n");

      //TODO error msg?
      return 1;
    }

    sprintf(gINIPath, "%s\\%s", gINIPath, BUNDLE_NAME); // Add the app name to the path

    struct stat st;
    if(stat(gINIPath, &st) == 0) // if directory exists
    {
      sprintf(gINIPath, "%s\\%s", gINIPath, "settings.ini"); // add file name to path

      if(stat(gINIPath, &st) == 0) // if settings file exists read values into state
      {
        gState->mAudioDriverType = GetPrivateProfileInt("audio", "driver", 0, gINIPath);

        GetPrivateProfileString("audio", "indev", DEFAULT_INPUT_DEV, gState->mAudioInDev, 100, gINIPath);
        GetPrivateProfileString("audio", "outdev", DEFAULT_OUTPUT_DEV, gState->mAudioOutDev, 100, gINIPath);

        //audio
        gState->mAudioInChanL = GetPrivateProfileInt("audio", "in1", 1, gINIPath); // 1 is first audio input
        gState->mAudioInChanR = GetPrivateProfileInt("audio", "in2", 2, gINIPath);
        gState->mAudioOutChanL = GetPrivateProfileInt("audio", "out1", 1, gINIPath); // 1 is first audio output
        gState->mAudioOutChanR = GetPrivateProfileInt("audio", "out2", 2, gINIPath);
        gState->mAudioInIsMono = GetPrivateProfileInt("audio", "monoinput", 0, gINIPath);

        GetPrivateProfileString("audio", "iovs", "512", gState->mAudioIOVS, 100, gINIPath);
        GetPrivateProfileString("audio", "sigvs", "32", gState->mAudioSigVS, 100, gINIPath);
        GetPrivateProfileString("audio", "sr", "44100", gState->mAudioSR, 100, gINIPath);

        //midi
        GetPrivateProfileString("midi", "indev", "no input", gState->mMidiInDev, 100, gINIPath);
        GetPrivateProfileString("midi", "outdev", "no output", gState->mMidiOutDev, 100, gINIPath);

        gState->mMidiInChan = GetPrivateProfileInt("midi", "inchan", 0, gINIPath); // 0 is any
        gState->mMidiOutChan = GetPrivateProfileInt("midi", "outchan", 0, gINIPath); // 1 is first chan

        UpdateINI(); // this will write over any invalid values in the file
      }
      else // settings file doesn't exist, so populate with default values
      {
        UpdateINI();
      }
    }
    else
    {
      // folder doesn't exist - make folder and make file
      CreateDirectory(gINIPath, NULL);
      sprintf(gINIPath, "%s%s", gINIPath, "settings.ini"); // add file name to path
      UpdateINI(); // will write file if doesn't exist
    }

    Init();

    CreateDialog(gHINST,MAKEINTRESOURCE(IDD_DIALOG_MAIN),GetDesktopWindow(),MainDlgProc);

    for(;;)
    {
      MSG msg= {0,};
      int vvv = GetMessage(&msg,NULL,0,0);
      if (!vvv)  break;

      if (vvv<0)
      {
        Sleep(10);
        continue;
      }
      if (!msg.hwnd)
      {
        DispatchMessage(&msg);
        continue;
      }

      if (gHWND && IsDialogMessage(gHWND,&msg)) continue;

      // default processing for other dialogs
      HWND hWndParent=NULL;
      HWND temphwnd = msg.hwnd;
      do
      {
        if (GetClassLong(temphwnd, GCW_ATOM) == (INT)32770)
        {
          hWndParent=temphwnd;
          if (!(GetWindowLong(temphwnd,GWL_STYLE)&WS_CHILD)) break; // not a child, exit
        }
      }
      while (temphwnd = GetParent(temphwnd));

      if (hWndParent && IsDialogMessage(hWndParent,&msg)) continue;

      TranslateMessage(&msg);
      DispatchMessage(&msg);

    }

    // in case gHWND didnt get destroyed -- this corresponds to SWELLAPP_DESTROY roughly
    if (gHWND) DestroyWindow(gHWND);

    Cleanup();

    ReleaseMutex(hMutex);
  }
  catch(...)
  {
    //TODO proper error catching
    DBGMSG("another instance running");
  }
  return 0;
}
#else

extern HMENU SWELL_app_stocksysmenu;
const char *homeDir;

INT_PTR SWELLAppMain(int msg, INT_PTR parm1, INT_PTR parm2)
{
  switch (msg)
  {
    case SWELLAPP_ONLOAD:

      gState = new AppState();
      gTempState = new AppState();
      gActiveState = new AppState();

      homeDir = getenv("HOME");
      sprintf(gINIPath, "%s/Library/Application Support/%s/", homeDir, BUNDLE_NAME);

      struct stat st;
      if(stat(gINIPath, &st) == 0) // if directory exists
      {
        sprintf(gINIPath, "%s%s", gINIPath, "settings.ini"); // add file name to path

        if(stat(gINIPath, &st) == 0) // if settings file exists read values into state
        {
          gState->mAudioDriverType = GetPrivateProfileInt("audio", "driver", 0, gINIPath);

          GetPrivateProfileString("audio", "indev", "Built-in Input", gState->mAudioInDev, 100, gINIPath);
          GetPrivateProfileString("audio", "outdev", "Built-in Output", gState->mAudioOutDev, 100, gINIPath);

          //audio
          gState->mAudioInChanL = GetPrivateProfileInt("audio", "in1", 1, gINIPath); // 1 is first audio input
          gState->mAudioInChanR = GetPrivateProfileInt("audio", "in2", 2, gINIPath);
          gState->mAudioOutChanL = GetPrivateProfileInt("audio", "out1", 1, gINIPath); // 1 is first audio output
          gState->mAudioOutChanR = GetPrivateProfileInt("audio", "out2", 2, gINIPath);
          gState->mAudioInIsMono = GetPrivateProfileInt("audio", "monoinput", 0, gINIPath);

          GetPrivateProfileString("audio", "iovs", "512", gState->mAudioIOVS, 100, gINIPath);
          GetPrivateProfileString("audio", "sigvs", "32", gState->mAudioSigVS, 100, gINIPath);
          GetPrivateProfileString("audio", "sr", "44100", gState->mAudioSR, 100, gINIPath);

          //midi
          GetPrivateProfileString("midi", "indev", "no input", gState->mMidiInDev, 100, gINIPath);
          GetPrivateProfileString("midi", "outdev", "no output", gState->mMidiOutDev, 100, gINIPath);

          gState->mMidiInChan = GetPrivateProfileInt("midi", "inchan", 0, gINIPath); // 0 is any
          gState->mMidiOutChan = GetPrivateProfileInt("midi", "outchan", 0, gINIPath); // 1 is first chan

          UpdateINI(); // this will write over any invalid values in the file
        }
        else // settings file doesn't exist, so populate with default values
        {
          UpdateINI();
        }

      }
      else   // folder doesn't exist - make folder and make file
      {
        // http://blog.tremend.ro/2008/10/06/create-directories-in-c-using-mkdir-with-proper-permissions/

        mode_t process_mask = umask(0);
        int result_code = mkdir(gINIPath, S_IRWXU | S_IRWXG | S_IRWXO);
        umask(process_mask);

        if(result_code) return 1;
        else
        {
          sprintf(gINIPath, "%s%s", gINIPath, "settings.ini"); // add file name to path
          UpdateINI(); // will write file if doesn't exist
        }
      }
      break;
#pragma mark loaded
    case SWELLAPP_LOADED:
    {
      Init();

      HMENU menu = SWELL_GetCurrentMenu();

      if (menu)
      {
        // other windows will get the stock (bundle) menus
        //SWELL_SetDefaultModalWindowMenu(menu);
        //SWELL_SetDefaultWindowMenu(menu);

        // work on a new menu
        menu = SWELL_DuplicateMenu(menu);
        HMENU src = LoadMenu(NULL,MAKEINTRESOURCE(IDR_MENU1));
        int x;
        for (x=0; x<GetMenuItemCount(src)-1; x++)
        {
          HMENU sm = GetSubMenu(src,x);
          if (sm)
          {
            char str[1024];
            MENUITEMINFO mii= {sizeof(mii),MIIM_TYPE,};
            mii.dwTypeData=str;
            mii.cch=sizeof(str);
            str[0]=0;
            GetMenuItemInfo(src,x,TRUE,&mii);
            MENUITEMINFO mi= {sizeof(mi),MIIM_STATE|MIIM_SUBMENU|MIIM_TYPE,MFT_STRING,0,0,SWELL_DuplicateMenu(sm),NULL,NULL,0,str};
            InsertMenuItem(menu,x+1,TRUE,&mi);
          }
        }
      }

      if (menu)
      {
        HMENU sm=GetSubMenu(menu,1);
        DeleteMenu(sm,ID_QUIT,MF_BYCOMMAND); // remove QUIT from our file menu, since it is in the system menu on OSX
        DeleteMenu(sm,ID_PREFERENCES,MF_BYCOMMAND); // remove PREFERENCES from the file menu, since it is in the system menu on OSX

        // remove any trailing separators
        int a= GetMenuItemCount(sm);
        while (a > 0 && GetMenuItemID(sm,a-1)==0) DeleteMenu(sm,--a,MF_BYPOSITION);

        DeleteMenu(menu,1,MF_BYPOSITION); // delete file menu
      }

      // if we want to set any default modifiers for items in the menus, we can use:
      // SetMenuItemModifier(menu,commandID,MF_BYCOMMAND,'A',FCONTROL) etc.

      HWND hwnd = CreateDialog(gHINST,MAKEINTRESOURCE(IDD_DIALOG_MAIN),NULL,MainDlgProc);
      if (menu)
      {
        SetMenu(hwnd, menu); // set the menu for the dialog to our menu (on Windows that menu is set from the .rc, but on SWELL
        SWELL_SetDefaultModalWindowMenu(menu); // other windows will get the stock (bundle) menus
      }
      // we need to set it manually (and obviously we've edited the menu anyway)
    }

    if(!AttachGUI()) DBGMSG("couldn't attach gui\n"); //todo error

    break;
    case SWELLAPP_ONCOMMAND:
      // this is to catch commands coming from the system menu etc
      if (gHWND && (parm1&0xffff)) SendMessage(gHWND,WM_COMMAND,parm1&0xffff,0);
      break;
#pragma mark destroy
    case SWELLAPP_DESTROY:

      if (gHWND) DestroyWindow(gHWND);
      Cleanup();
      break;
    case SWELLAPP_PROCESSMESSAGE: // can hook keyboard input here
      // parm1 = (MSG*), should we want it -- look in swell.h to see what the return values refer to
      break;
  }
  return 0;
}

#endif


#ifndef OS_WIN
#include "swell-dlggen.h"

#define SET_IDD_SCALE 1.
#define SET_IDD_STYLE SWELL_DLG_WS_FLIPPED|SWELL_DLG_WS_NOAUTOSIZE

SWELL_DEFINE_DIALOG_RESOURCE_BEGIN(IDD_DIALOG_MAIN, SET_IDD_STYLE, BUNDLE_NAME, GUI_WIDTH, GUI_HEIGHT, SET_IDD_SCALE)
BEGIN
//   EDITTEXT        IDC_EDIT1,59,50,145,14,ES_AUTOHSCROLL
//   LTEXT           "Enter some text here:",IDC_STATIC,59,39,73,8
END
SWELL_DEFINE_DIALOG_RESOURCE_END(IDD_DIALOG_MAIN)

SWELL_DEFINE_DIALOG_RESOURCE_BEGIN(IDD_DIALOG_PREF,SET_IDD_STYLE,"Preferences",320,420,SET_IDD_SCALE)
BEGIN
GROUPBOX        "Audio Settings", IDC_STATIC, 5, 10, 300, 230

LTEXT           "Driver Type", IDC_STATIC, 20, 32, 60, 20
COMBOBOX        IDC_COMBO_AUDIO_DRIVER, 20, 50, 150, 100, CBS_DROPDOWNLIST

LTEXT           "Input Device", IDC_STATIC, 20, 75, 80, 20
COMBOBOX        IDC_COMBO_AUDIO_IN_DEV, 20, 90, 150, 100, CBS_DROPDOWNLIST

LTEXT           "Output Device", IDC_STATIC, 20, 115, 80, 20
COMBOBOX        IDC_COMBO_AUDIO_OUT_DEV, 20, 130, 150, 100, CBS_DROPDOWNLIST

LTEXT           "In 1 (L)", IDC_STATIC, 20, 155, 90, 20
COMBOBOX        IDC_COMBO_AUDIO_IN_L, 20, 170, 46, 100, CBS_DROPDOWNLIST

LTEXT           "In 2 (R)", IDC_STATIC, 75, 155, 90, 20
COMBOBOX        IDC_COMBO_AUDIO_IN_R, 75, 170, 46, 100, CBS_DROPDOWNLIST

CHECKBOX        "Mono", IDC_CB_MONO_INPUT, 125, 128, 56, 100, 0

LTEXT           "Out 1 (L)", IDC_STATIC, 20, 195, 60, 20
COMBOBOX        IDC_COMBO_AUDIO_OUT_L, 20, 210, 46, 100, CBS_DROPDOWNLIST

LTEXT           "Out 2 (R)", IDC_STATIC, 75, 195, 60, 20
COMBOBOX        IDC_COMBO_AUDIO_OUT_R, 75, 210, 46, 100, CBS_DROPDOWNLIST

LTEXT           "IO Vector Size", IDC_STATIC, 200, 32, 80, 20
COMBOBOX        IDC_COMBO_AUDIO_IOVS, 200, 50, 90, 100, CBS_DROPDOWNLIST

LTEXT           "Signal Vector Size", IDC_STATIC, 200, 75, 100, 20
COMBOBOX        IDC_COMBO_AUDIO_SIGVS, 200, 90, 90, 100, CBS_DROPDOWNLIST

LTEXT           "Sampling Rate", IDC_STATIC, 200, 115, 80, 20
COMBOBOX        IDC_COMBO_AUDIO_SR, 200, 130, 90, 100, CBS_DROPDOWNLIST

PUSHBUTTON      "Audio Midi Setup...", IDC_BUTTON_ASIO, 180, 170, 110, 40

GROUPBOX        "MIDI Settings", IDC_STATIC, 5, 255, 300, 120

LTEXT           "Input Device", IDC_STATIC, 20, 275, 100, 20
COMBOBOX        IDC_COMBO_MIDI_IN_DEV, 20, 293, 150, 100, CBS_DROPDOWNLIST

LTEXT           "Output Device", IDC_STATIC, 20, 320, 100, 20
COMBOBOX        IDC_COMBO_MIDI_OUT_DEV, 20, 338, 150, 100, CBS_DROPDOWNLIST

LTEXT           "Input Channel", IDC_STATIC, 200, 275, 100, 20
COMBOBOX        IDC_COMBO_MIDI_IN_CHAN, 200, 293, 90, 100, CBS_DROPDOWNLIST

LTEXT           "Output Channel", IDC_STATIC, 200, 320, 100, 20
COMBOBOX        IDC_COMBO_MIDI_OUT_CHAN, 200, 338, 90, 100, CBS_DROPDOWNLIST

DEFPUSHBUTTON   "OK", IDOK, 192, 383, 50, 20
PUSHBUTTON      "Apply", IDAPPLY, 132, 383, 50, 20
PUSHBUTTON      "Cancel", IDCANCEL, 252, 383, 50, 20
END
SWELL_DEFINE_DIALOG_RESOURCE_END(IDD_DIALOG_PREF)

#include "swell-menugen.h"

SWELL_DEFINE_MENU_RESOURCE_BEGIN(IDR_MENU1)
POPUP "&File"
BEGIN
// MENUITEM SEPARATOR
MENUITEM "Preferences...",              ID_PREFERENCES
MENUITEM "&Quit",                       ID_QUIT
END
POPUP "&Help"
BEGIN
MENUITEM "&About",                      ID_ABOUT
END
SWELL_DEFINE_MENU_RESOURCE_END(IDR_MENU1)

#endif