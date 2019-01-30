
#pragma once

#include "IPlugStructs.h"
#include "pluginterfaces/vst/ivstevents.h"

void DoMIDIn(IPlugProcessor<PLUG_SAMPLE_DST>& processor, IEventList* eventList, IPlugQueue<IMidiMsg>& editorQueue, IPlugQueue<IMidiMsg>& processorQueue)
{
  IMidiMsg msg;
  
  // Process events.. only midi note on and note off?
  
  if (eventList)
  {
    int32 numEvent = eventList->getEventCount();
    for (int32 i=0; i<numEvent; i++)
    {
      Event event;
      if (eventList->getEvent(i, event) == kResultOk)
      {
        switch (event.type)
        {
          case Event::kNoteOnEvent:
          {
            msg.MakeNoteOnMsg(event.noteOn.pitch, event.noteOn.velocity * 127, event.sampleOffset, event.noteOn.channel);
            processor.ProcessMidiMsg(msg);
            processorQueue.Push(msg);
            break;
          }
            
          case Event::kNoteOffEvent:
          {
            msg.MakeNoteOffMsg(event.noteOff.pitch, event.sampleOffset, event.noteOff.channel);
            processor.ProcessMidiMsg(msg);
            processorQueue.Push(msg);
            break;
          }
          case Event::kPolyPressureEvent:
          {
            msg.MakePolyATMsg(event.polyPressure.pitch, event.polyPressure.pressure * 127., event.sampleOffset, event.polyPressure.channel);
            processor.ProcessMidiMsg(msg);
            processorQueue.Push(msg);
            break;
          }
          case Event::kDataEvent:
          {
            ISysEx syx = ISysEx(event.sampleOffset, event.data.bytes, event.data.size);
            processor.ProcessSysEx(syx);
            //mSysexMsgsFromProcessor.Push
            break;
          }
        }
      }
    }
  }
  
  while (editorQueue.Pop(msg))
  {
    processor.ProcessMidiMsg(msg);
  }
}

void DoMIDIOut(IMidiQueue& queue, IPlugQueue<SysExData>& sysExQueue, SysExData& sysExBuf, IEventList* outputEvents, int32 numSamples)
{
  // MIDI
  if (!queue.Empty() && outputEvents)
  {
    Event toAdd = {0};
    IMidiMsg msg;
    
    while (!queue.Empty())
    {
      IMidiMsg& msg = queue.Peek();
      
      if (msg.StatusMsg() == IMidiMsg::kNoteOn)
      {
        toAdd.type = Event::kNoteOnEvent;
        toAdd.noteOn.channel = msg.Channel();
        toAdd.noteOn.pitch = msg.NoteNumber();
        toAdd.noteOn.tuning = 0.;
        toAdd.noteOn.velocity = (float) msg.Velocity() * (1.f / 127.f);
        toAdd.noteOn.length = -1;
        toAdd.noteOn.noteId = -1; // TODO ?
        toAdd.sampleOffset = msg.mOffset;
        outputEvents->addEvent(toAdd);
      }
      else if (msg.StatusMsg() == IMidiMsg::kNoteOff)
      {
        toAdd.type = Event::kNoteOffEvent;
        toAdd.noteOff.channel = msg.Channel();
        toAdd.noteOff.pitch = msg.NoteNumber();
        toAdd.noteOff.velocity = (float) msg.Velocity() * (1.f / 127.f);
        toAdd.noteOff.noteId = -1; // TODO ?
        toAdd.sampleOffset = msg.mOffset;
        outputEvents->addEvent(toAdd);
      }
      else if (msg.StatusMsg() == IMidiMsg::kPolyAftertouch)
      {
        toAdd.type = Event::kPolyPressureEvent;
        toAdd.polyPressure.channel = msg.Channel();
        toAdd.polyPressure.pitch = msg.NoteNumber();
        toAdd.polyPressure.pressure = (float) msg.PolyAfterTouch() * (1.f / 127.f);
        toAdd.polyPressure.noteId = -1; // TODO ?
        toAdd.sampleOffset = msg.mOffset;
        outputEvents->addEvent(toAdd);
      }
      
      queue.Remove();
      // don't add any midi messages other than noteon/noteoff
    }
  }
  
  queue.Flush(numSamples);
  
  // Output SYSEX from the editor, which has bypassed the processors' ProcessSysEx()
  if (sysExQueue.ElementsAvailable())
  {
    Event toAdd = {0};
    
    while (sysExQueue.Pop(sysExBuf))
    {
      toAdd.type = Event::kDataEvent;
      toAdd.sampleOffset = sysExBuf.mOffset;
      toAdd.data.type = DataEvent::kMidiSysEx;
      toAdd.data.size = sysExBuf.mSize;
      toAdd.data.bytes = (uint8*) sysExBuf.mData; // TODO!  this is a problem if more than one message in this block!
      outputEvents->addEvent(toAdd);
    }
  }
}
