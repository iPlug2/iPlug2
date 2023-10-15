/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
 */

#ifndef _IPLUGIOSMIDI_
#define _IPLUGIOSMIDI_

#include <CoreMIDI/CoreMIDI.h>
#include "wdlstring.h"

struct IPlugIOSMIDI
{
public:
  
  enum class ConnectionType
  {
    Source,
    Destination
  };
  
  template <ConnectionType Type>
  class MIDICachedEndpoint
  {
  public:
    
    void Update(MIDIPortRef port = NULL)
    {
      long idx = mLastName ? GetIndexFromName(mLastName, Type) : (GetNumSourcesOrDestinations(Type) ? 0 : -1);
      
      if (idx >= 0)
      {
        auto endpoint = GetEndpoint(idx, Type);;
        
        if (mEndpoint != endpoint)
        {
          if (port && mEndpoint)
            MIDIPortDisconnectSource(port, mEndpoint);
          
          if (Type == ConnectionType::Destination)
            MIDIFlushOutput(endpoint);
          
          mEndpoint = endpoint;
          if (mLastName)
            CFRelease(mLastName);
          mLastName = CreateNameFromMIDEndpoint(mEndpoint);
          
          if (port)
            MIDIPortConnectSource(port, endpoint, NULL);
        }
      }
      else
        mEndpoint = NULL;
    }
    
    MIDIEndpointRef Get() { return mEndpoint; }
    
    void SetName(const char *name, MIDIPortRef port = NULL)
    {
      mLastName = CFStringCreateWithCString(NULL, name, kCFStringEncodingUTF8);
      Update(port);
    }
    
    void GetName(WDL_String& name)
    {
      char cString[2048];

      CFStringGetCString(mLastName, cString, sizeof(cString), kCFStringEncodingUTF8);
      name.Set(cString);
    }
    
  private:
    
    MIDIEndpointRef mEndpoint = NULL;
    CFStringRef mLastName = NULL;
  };
  
  using MIDICachedSource = IPlugIOSMIDI::MIDICachedEndpoint<IPlugIOSMIDI::ConnectionType::Source>;
  using MIDICachedDestination = IPlugIOSMIDI::MIDICachedEndpoint<IPlugIOSMIDI::ConnectionType::Destination>;
  
  static ItemCount GetNumSourcesOrDestinations(ConnectionType type);
  static MIDIEndpointRef GetEndpoint(ItemCount idx, ConnectionType type);
  
  static CFStringRef CreateNameFromMIDEndpoint(MIDIEndpointRef endpoint);
  static CFStringRef CreateNameFromIndex(ItemCount idx, ConnectionType type);
  
  static void GetNameFromIndex(WDL_String &string, int idx, ConnectionType type);
  static long GetIndexFromName(CFStringRef name, ConnectionType type);
  static long GetIndexFromName(WDL_String name, ConnectionType type);
  
  static void SetMidiPort(const char *name, ConnectionType type);
  static void GetMidiPort(WDL_String& name, ConnectionType type);
};

#ifdef __OBJC__

#import <AVFoundation/AVFoundation.h>

@interface IPlugIOSMIDIHost : NSObject
- (IPlugIOSMIDIHost*) init;
- (void) setAUAudioUnit:(AUAudioUnit*) audiounit;
@end

#endif

#endif /* _IPLUGIOSMIDI_ */

