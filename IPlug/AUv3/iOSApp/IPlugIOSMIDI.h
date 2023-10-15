/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
 */

#ifndef _IPLUGIOSMIDI_
#define _IPLUGIOSMIDI_

#include <CoreMIDI/CoreMIDI.h>
#include "IPlugConstants.h"
#include "wdlstring.h"

struct IPlugIOSMIDI
{
  using ERoute = iplug::ERoute;
  
public:
  
  template <ERoute Route>
  class MIDICachedEndpoint
  {
  public:
    
    MIDICachedEndpoint() {}
    ~MIDICachedEndpoint() { CFRelease(mLastName); }
    
    MIDICachedEndpoint(const MIDICachedEndpoint&) = delete;
    MIDICachedEndpoint& operator=(const MIDICachedEndpoint&) = delete;
    
    void Update(MIDIPortRef port = NULL)
    {
      long idx = mLastName ? GetIndexFromName(mLastName, Route) : (GetNumSourcesOrDestinations(Route) ? 0 : -1);
      
      if (idx >= 0)
      {
        auto endpoint = GetEndpoint(idx, Route);
        
        if (mEndpoint != endpoint)
        {
          if (port && mEndpoint)
            MIDIPortDisconnectSource(port, mEndpoint);
          
          if (Route == iplug::kOutput)
            MIDIFlushOutput(endpoint);
          
          UpdateEndpoint(endpoint);
          
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
    
    void UpdateEndpoint(MIDIEndpointRef endpoint)
    {
      mEndpoint = endpoint;
      
      if (mLastName)
        CFRelease(mLastName);
      
      mLastName = CreateNameFromMIDEndpoint(mEndpoint);
    }
    
    MIDIEndpointRef mEndpoint = NULL;
    CFStringRef mLastName = NULL;
  };
  
  using MIDICachedSource = IPlugIOSMIDI::MIDICachedEndpoint<ERoute::kInput>;
  using MIDICachedDestination = IPlugIOSMIDI::MIDICachedEndpoint<ERoute::kOutput>;
  
  static ItemCount GetNumSourcesOrDestinations(ERoute route);
  static MIDIEndpointRef GetEndpoint(ItemCount idx, ERoute route);
  
  static CFStringRef CreateNameFromMIDEndpoint(MIDIEndpointRef endpoint);
  static CFStringRef CreateNameFromIndex(ItemCount idx, ERoute route);
  
  static void GetNameFromIndex(WDL_String &string, int idx, ERoute route);
  static long GetIndexFromName(CFStringRef name, ERoute route);
  static long GetIndexFromName(WDL_String name, ERoute route);
  
  static void SetMidiPort(const char *name, ERoute route);
  static void GetMidiPort(WDL_String& name, ERoute route);
};

#ifdef __OBJC__

#import <AVFoundation/AVFoundation.h>

@interface IPlugIOSMIDIHost : NSObject
- (IPlugIOSMIDIHost*) init;
- (void) setAUAudioUnit:(AUAudioUnit*) audiounit;
@end

#endif

#endif /* _IPLUGIOSMIDI_ */

