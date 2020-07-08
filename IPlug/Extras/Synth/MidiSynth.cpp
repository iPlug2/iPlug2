/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
 */

#include "MidiSynth.h"

using namespace iplug;

MidiSynth::MidiSynth(VoiceAllocator::EPolyMode mode, int blockSize)
: mBlockSize(blockSize)
{
  SetPolyMode(mode);

  for(int i=0; i<128; i++)
  {
    mVelocityLUT[i] = i / 127.f;
    mAfterTouchLUT[i] = i / 127.f;
  }

  // initialize Channel states
  for(int i=0; i<16; ++i)
  {
    mChannelStates[i] = ChannelState{0};
    mChannelStates[i].pitchBendRange = kDefaultPitchBendRange;
  }
}

MidiSynth::~MidiSynth()
{
}

VoiceInputEvent MidiSynth::MidiMessageToEventBasic(const IMidiMsg& msg)
{
  VoiceInputEvent event{};

  IMidiMsg::EStatusMsg status = msg.StatusMsg();
  event.mSampleOffset = msg.mOffset;
  event.mAddress.mChannel = msg.Channel();
  event.mAddress.mKey = msg.NoteNumber();

  switch (status)
  {
    case IMidiMsg::kNoteOn:
    {
      int v = Clip(msg.Velocity(), 0, 127);
      event.mAction = (v == 0) ? kNoteOffAction : kNoteOnAction;
      event.mValue = mVelocityLUT[v];
      break;
    }
    case IMidiMsg::kNoteOff:
    {
      int v = Clip(msg.Velocity(), 0, 127);
      event.mAction = kNoteOffAction;
      event.mValue = mVelocityLUT[v];
      break;
    }
    case IMidiMsg::kPolyAftertouch:
    {
      event.mAction = kPressureAction;
      event.mValue = mAfterTouchLUT[msg.PolyAfterTouch()];
      break;
    }
    case IMidiMsg::kChannelAftertouch:
    {
      event.mAction = kPressureAction;
      event.mValue = mAfterTouchLUT[msg.ChannelAfterTouch()];
      break;
    }
    case IMidiMsg::kPitchWheel:
    {
      event.mAction = kPitchBendAction;
      float bendRange = mChannelStates[event.mAddress.mChannel].pitchBendRange;
      event.mValue = static_cast<float>(msg.PitchWheel()) * bendRange / 12.f;
      break;
    }
    case IMidiMsg::kControlChange:
    {
      event.mControllerNumber = msg.ControlChangeIdx();
      event.mValue = static_cast<float>(msg.ControlChange(msg.ControlChangeIdx()));
      switch(event.mControllerNumber)
      {
        // handle special controllers
        case IMidiMsg::kCutoffFrequency:
        {
          event.mAction = kTimbreAction;
          break;
        }
        case IMidiMsg::kAllNotesOff:
        {
          event.mAddress.mFlags = kVoicesAll;
          event.mAction = kNoteOffAction;
          break;
        }
        // handle all other controllers
        default:
        {
          event.mAction = kControllerAction;
          break;
        }
      }
      break;
    }
    case IMidiMsg::kProgramChange:
    {
      event.mAction = kProgramChangeAction;
      event.mControllerNumber = msg.Program();
      break;
    }
    default:
    {
      break;
    }
  }

  return event;
}

// Here we handle the MIDI messages used by MPE as listed in the MPE spec, page 7
VoiceInputEvent MidiSynth::MidiMessageToEventMPE(const IMidiMsg& msg)
{
  VoiceInputEvent event{};
  IMidiMsg::EStatusMsg status = msg.StatusMsg();
  event.mSampleOffset = msg.mOffset;
  event.mAddress.mChannel = msg.Channel();
  event.mAddress.mKey = msg.NoteNumber();
  event.mAddress.mZone = MasterZoneFor(event.mAddress.mChannel);

  // handle pitch bend, channel pressure and CC#74 in the same way:
  // sum main and member channel values
  bool isPitchBend = status == IMidiMsg::kPitchWheel;
  bool isChannelPressure = status == IMidiMsg::kChannelAftertouch;
  bool isTimbre = (status == IMidiMsg::kControlChange) && (msg.ControlChangeIdx() == IMidiMsg::kCutoffFrequency);
  if(isPitchBend || isChannelPressure || isTimbre)
  {
    float* pChannelDestValue{};
    float masterChannelStoredValue{};
    if(isPitchBend)
    {
      event.mAction = kPitchBendAction;
      float bendRange = mChannelStates[event.mAddress.mChannel].pitchBendRange;
      event.mValue = static_cast<float>(msg.PitchWheel()) * bendRange / 12.f;

      pChannelDestValue = &(mChannelStates[event.mAddress.mChannel].pitchBend);
      masterChannelStoredValue = mChannelStates[MasterChannelFor(event.mAddress.mChannel)].pitchBend;
    }
    else if(isChannelPressure)
    {
      event.mAction = kPressureAction;
      event.mValue = mAfterTouchLUT[msg.ChannelAfterTouch()];
      pChannelDestValue = &(mChannelStates[event.mAddress.mChannel].pressure);
      masterChannelStoredValue = mChannelStates[MasterChannelFor(event.mAddress.mChannel)].pressure;
    }
    else if(isTimbre)
    {
      event.mAction = kTimbreAction;
      event.mValue = static_cast<float>(msg.ControlChange(msg.ControlChangeIdx()));
      pChannelDestValue = &(mChannelStates[event.mAddress.mChannel].timbre);
      masterChannelStoredValue = mChannelStates[MasterChannelFor(event.mAddress.mChannel)].timbre;
    }

    if(IsMasterChannel(event.mAddress.mChannel))
    {
      // store value in master channel
      *pChannelDestValue = event.mValue;

      // no action needed
      event.mAction = kNullAction;
    }
    else
    {
      // add stored master channel value to event value
      event.mValue += masterChannelStoredValue;

      // store sum in member channel
      *pChannelDestValue = event.mValue;
    }
    return event;
  }

  // pitch bend sensitiity (RPN 0) is handled in HandleRPN()

  // poly key pressure is ignored in MPE.

  switch (status)
  {
    // program change:
    // we are using MIDI mode 3. A program change sent to a master channel
    // affects all voices within the zone. Program changes sent to member channels are ignored.
    case IMidiMsg::kProgramChange:
    {
      if(IsMasterChannel(event.mAddress.mChannel))
      {
        event.mAction = kProgramChangeAction;
        event.mControllerNumber = msg.Program();
        break;
      }
      else
      {
        event.mAction = kNullAction;
      }
    }
    case IMidiMsg::kNoteOn:
    {
      int v = Clip(msg.Velocity(), 0, 127);
      event.mAction = (v == 0) ? kNoteOffAction : kNoteOnAction;
      event.mValue = mVelocityLUT[v];
      break;
    }
    case IMidiMsg::kNoteOff:
    {
      int v = Clip(msg.Velocity(), 0, 127);
      event.mAction = kNoteOffAction;
      event.mValue = mVelocityLUT[v];
      break;
    }
    case IMidiMsg::kControlChange:
    {
      event.mControllerNumber = msg.ControlChangeIdx();
      switch(event.mControllerNumber)
      {
        case IMidiMsg::kAllNotesOff:
        {
          event.mAddress.mFlags = kVoicesAll;
          event.mAction = kNoteOffAction;
          break;
        }

        default:
          // send all other controllers to matching channels using the generic control action
          // note: according to the MPE spec these messages should be sent to all channels in the zone,
          // but that is less useful IMO - to do so just add the line
          // event.mAddress.mChannel = kAllChannels;
          event.mAction = kControllerAction;
          break;
      }
      event.mValue = static_cast<float>(msg.ControlChange(msg.ControlChangeIdx()));
      break;
    }
    default:
    {
      break;
    }
  }

  return event;
}

VoiceInputEvent MidiSynth::MidiMessageToEvent(const IMidiMsg& msg)
{
  return (mMPEMode ? MidiMessageToEventMPE(msg) : MidiMessageToEventBasic(msg));
}

// sets the number of channels in the lo or hi MPE zones.
void MidiSynth::SetMPEZones(int channel, int nChans)
{
  // total channels = member channels + the master channel, or 0 if there is no Zone.
  // totalChannels is never 1.
  int memberChannels = Clip(nChans, 0, 15);
  int totalChannels = memberChannels ? (memberChannels + 1) : 0;
  if(channel == 0)
  {
    mMPELowerZoneChannels = totalChannels;
    mMPEUpperZoneChannels = Clip(mMPEUpperZoneChannels, 0, 16 - mMPELowerZoneChannels);
  }
  else if (channel == 15)
  {
    mMPEUpperZoneChannels = totalChannels;
    mMPELowerZoneChannels = Clip(mMPELowerZoneChannels, 0, 16 - mMPEUpperZoneChannels);
  }

  // activate / deactivate MPE mode if needed
  bool anyMPEChannelsActive = (mMPELowerZoneChannels || mMPEUpperZoneChannels);
  if(anyMPEChannelsActive && (!mMPEMode))
  {
    mMPEMode = true;
  }
  else if ((!anyMPEChannelsActive) && (mMPEMode))
  {
    mMPEMode = false;
  }

  // reset pitch bend ranges as per MPE spec
  if(mMPEMode)
  {
    if(channel == 0)
    {
      SetChannelPitchBendRange(kMPELowerZoneMasterChannel, 2);
      SetChannelPitchBendRange(kMPELowerZoneMasterChannel + 1, 48);
    }
    else if (channel == 15)
    {
      SetChannelPitchBendRange(kMPEUpperZoneMasterChannel, 2);
      SetChannelPitchBendRange(kMPEUpperZoneMasterChannel - 1, 48);
    }
  }
  else
    SetPitchBendRange(mNonMPEPitchBendRange);
  
  std::cout << "MPE mode: " << (mMPEMode ? "ON" : "OFF") << "\n";
  std::cout << "MPE channels: \n    lo: " << mMPELowerZoneChannels << " hi " << mMPEUpperZoneChannels << "\n";
}

void MidiSynth::SetChannelPitchBendRange(int channelParam, int rangeParam)
{
  int channelLo, channelHi;
  if(IsInLowerZone(channelParam))
  {
    channelLo = LowerZoneStart();
    channelHi = LowerZoneEnd();
  }
  else if (IsInUpperZone(channelParam))
  {
    channelLo = UpperZoneStart();
    channelHi = UpperZoneEnd();
  }
  else
  {
    channelLo = channelHi = Clip(channelParam, 0, 15);
  }

  int range = Clip(rangeParam, 0, 96);

  for(int i=channelLo; i <= channelHi; ++i)
  {
    mChannelStates[i].pitchBendRange = range;
  }
}

bool IsRPNMessage(IMidiMsg msg)
{
  if(msg.StatusMsg() != IMidiMsg::kControlChange) return false;
  int cc = msg.mData1;
  return(cc == 0x64)||(cc == 0x65)||(cc == 0x26)||(cc == 0x06);
}

void MidiSynth::HandleRPN(IMidiMsg msg)
{
  int channel = msg.Channel();
  ChannelState& state = mChannelStates[channel];

  uint8_t valueByte = msg.mData2;
  int param, value;
  switch (msg.mData1)
  {
    case 0x64:
      state.paramLSB = valueByte;
      state.valueMSB = state.valueLSB = 0xff;
      break;
    case 0x65:
      state.paramMSB = valueByte;
      state.valueMSB = state.valueLSB = 0xff;
      break;
    case 0x26:
      state.valueLSB = valueByte;
      break;
    case 0x06:
      // whenever the value MSB byte is received we constuct the value and take action on the RPN.
      // if only the MSB has been received, it is used as the entire value so the maximum possible value is 127.
      state.valueMSB = valueByte;
      param = ((state.paramMSB&0xFF) << 7) + (state.paramLSB&0xFF);
      if(state.valueLSB != 0xff)
      {
        value = ((state.valueMSB&0xFF) << 7) + (state.valueLSB&0xFF);
      }
      else
      {
        value = state.valueMSB&0xFF;
      }
      std::cout << "RPN received: channel " << channel << ", param " << param << ", value " << value << "\n";
      switch(param)
      {
        case 0: // RPN 0 : pitch bend range
          SetChannelPitchBendRange(channel, value);
          break;
        case 6: // RPN 6 : MPE zone configuration. These messages may turn MPE mode on or off.
          if(IsMasterChannel(channel))
          {
            SetMPEZones(channel, value);
          }
          break;
        default:
          break;
      }
      break;

    default:
      break;
  }
}

bool MidiSynth::ProcessBlock(sample** inputs, sample** outputs, int nInputs, int nOutputs, int nFrames)
{
  assert(NVoices());

  if (mVoicesAreActive | !mMidiQueue.Empty())
  {
    int blockSize = mBlockSize;
    int samplesRemaining = nFrames;
    int startIndex = 0;

    while(samplesRemaining > 0)
    {
      if(samplesRemaining < blockSize)
        blockSize = samplesRemaining;

      while (!mMidiQueue.Empty())
      {
        IMidiMsg msg = mMidiQueue.Peek();

        // we assume the messages are in chronological order. If we find one later than the current block we are done.
        if (msg.mOffset > startIndex + blockSize) break;

        if(IsRPNMessage(msg))
        {
          HandleRPN(msg);
        }
        else
        {
          // send performance messages to the voice allocator
          // message offset is relative to the start of this processSamples() block
          msg.mOffset -= startIndex;
          mVoiceAllocator.AddEvent(MidiMessageToEvent(msg));
        }
        mMidiQueue.Remove();
      }

      mVoiceAllocator.ProcessEvents(blockSize, mSampleTime);
      mVoiceAllocator.ProcessVoices(inputs, outputs, nInputs, nOutputs, startIndex, blockSize);

      samplesRemaining -= blockSize;
      startIndex += blockSize;
      mSampleTime += blockSize;
    }

    bool voicesbusy = false;
    int activeCount = 0;

    for(int v = 0; v < NVoices(); v++)
    {
      bool busy = GetVoice(v)->GetBusy();
      voicesbusy |= busy;

      activeCount += (busy==true);
#if DEBUG_VOICE_COUNT
      if(GetVoice(v)->GetBusy()) printf("X");
      else DBGMSG("_");
    }
    DBGMSG("\n");
    DBGMSG("Num Voices busy %i\n", activeCount);
#else
    }
#endif

    mVoicesAreActive = voicesbusy;

    mMidiQueue.Flush(nFrames);
  }
  else // empty block
  {
    return true;
  }

  return false; // made some noise
}

void MidiSynth::SetSampleRateAndBlockSize(double sampleRate, int blockSize)
{
  Reset();

  mSampleRate = sampleRate;
  mMidiQueue.Resize(blockSize);
  mVoiceAllocator.SetSampleRateAndBlockSize(sampleRate, blockSize);

  for(int v = 0; v < NVoices(); v++)
  {
    GetVoice(v)->SetSampleRateAndBlockSize(sampleRate, blockSize);
  }
}
