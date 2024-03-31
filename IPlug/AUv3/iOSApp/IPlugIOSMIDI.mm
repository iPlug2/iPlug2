
#import <Foundation/Foundation.h>

#include "IPlugIOSMIDI.h"
#include "config.h"


// static
ItemCount IPlugIOSMIDI::GetNumSourcesOrDestinations(ERoute route)
{
  if (route == iplug::kInput)
  {
    return MIDIGetNumberOfSources();
  }
  else
  {
    return MIDIGetNumberOfDestinations();
  }
}

// static
MIDIEndpointRef IPlugIOSMIDI::GetEndpoint(ItemCount idx, ERoute route)
{
  if (route == iplug::kInput)
  {
    return MIDIGetSource(idx);
  }
  else
  {
    return MIDIGetDestination(idx);
  }
}

// static
CFStringRef IPlugIOSMIDI::CreateNameFromMIDEndpoint(MIDIEndpointRef endpoint)
{
  CFStringRef names[2];
  MIDIEntityRef entity;
  MIDIDeviceRef device;
  
  MIDIEndpointGetEntity(endpoint, &entity);
  MIDIEntityGetDevice(entity, &device);
  
  MIDIObjectGetStringProperty(device, kMIDIPropertyName, names + 0);
  MIDIObjectGetStringProperty(endpoint, kMIDIPropertyName, names + 1);
  
  CFStringRef name;
  
  if (CFStringCompare(names[0], names[1], kCFCompareCaseInsensitive) == kCFCompareEqualTo)
  {
    name = CFStringCreateCopy(NULL, names[0]);
  }
  else
  {
    CFStringRef separator = CFStringCreateWithCString(NULL, ": ", kCFStringEncodingUTF8);
    CFArrayRef array = CFArrayCreate(NULL, (const void **) names, 2, NULL);
    
    name = CFStringCreateByCombiningStrings(NULL, array, separator);
    
    CFRelease(array);
    CFRelease(separator);
  }
  
  return name;
}

// static
CFStringRef IPlugIOSMIDI::CreateNameFromIndex(ItemCount idx, ERoute route)
{
  return CreateNameFromMIDEndpoint(GetEndpoint(idx, route));
}

// static
void IPlugIOSMIDI::GetNameFromIndex(WDL_String &string, int idx, ERoute route)
{
  char cString[2048];
  CFStringRef str = CreateNameFromIndex(idx, route);
  CFStringGetCString(str, cString, sizeof(cString), kCFStringEncodingUTF8);
  CFRelease(str);
  string.Set(cString);
}

// static
long IPlugIOSMIDI::GetIndexFromName(CFStringRef name, ERoute route)
{
  auto numEndPoints = GetNumSourcesOrDestinations(route);
  long idx = -1;
  
  for (long i = 0; i < numEndPoints && idx < 0; i++)
  {
    auto str = CreateNameFromMIDEndpoint(GetEndpoint(i, route));
    if (CFStringCompare(name, str, 0) == kCFCompareEqualTo)
    {
      idx = i;
    }
    CFRelease(str);
  }
  
  return idx;
}

// static
long IPlugIOSMIDI::GetIndexFromName(WDL_String name, ERoute route)
{
  CFStringRef str = CFStringCreateWithCString(NULL, name.Get(), kCFStringEncodingUTF8);
  long idx = GetIndexFromName(str, route);
  CFRelease(str);
  
  return idx;
}

// static
void IPlugIOSMIDI::SetMidiPort(const char* name, ERoute route)
{
  NSDictionary* dict = @{@"name": @(name), @"direction": @(route == iplug::kInput ? "source" : "destination")};
  [[NSNotificationCenter defaultCenter] postNotificationName:@"SetMIDIPort" object:nil userInfo:dict];
}

// static
void IPlugIOSMIDI::GetMidiPort(WDL_String& name, ERoute route)
{
  NSDictionary* dict = @{@"name": [NSValue valueWithPointer:&name], @"direction": @(route == iplug::kInput ? "source" : "destination")};
  [[NSNotificationCenter defaultCenter] postNotificationName:@"GetMIDIPort" object:nil userInfo:dict];
}

@implementation IPlugIOSMIDIHost
{
  MIDIClientRef mClient;
  IPlugIOSMIDI::MIDICachedSource mSource;
  IPlugIOSMIDI::MIDICachedDestination mDestination;
  AUMIDIEventListBlock mReceiveBlock;
  AUEventSampleTime mTime;
};

- (IPlugIOSMIDIHost*) init
{
  CFStringRef midiClientName = CFStringCreateWithCString(NULL, "MIDI Client", kCFStringEncodingUTF8);
  
  MIDIClientCreateWithBlock(midiClientName, &mClient, 
                            ^(const MIDINotification* message) {
                              if (message->messageID == kMIDIMsgSetupChanged)
                                [self updateConnections];
                            });
  
  CFRelease(midiClientName);
  
#if PLUG_DOES_MIDI_IN
  MIDIPortRef inPort;
  CFStringRef inPortName = CFStringCreateWithCString(NULL, "MIDI Input", kCFStringEncodingUTF8);
  MIDIInputPortCreateWithProtocol(mClient, inPortName, kMIDIProtocol_1_0, &inPort, 
                                    ^(const MIDIEventList *list, void * __nullable refCon) {
                                    [self receiveMIDI:list];
                                  });
  CFRelease(inPortName);
  mSource.SetPort(inPort);
#endif
  
#if PLUG_DOES_MIDI_OUT
  MIDIPortRef outPort;
  CFStringRef outPortName = CFStringCreateWithCString(NULL, "MIDI Output", kCFStringEncodingUTF8);
  MIDIOutputPortCreate(mClient, outPortName, &outPort);
  CFRelease(outPortName);
  mDestination.SetPort(outPort);
#endif
  
  [self updateConnections];
  
  [[NSNotificationCenter defaultCenter] addObserver:self 
                                           selector:@selector(receiveNotification:) 
                                               name:@"SetMIDIPort" object:nil];
  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(receiveNotification:) 
                                               name:@"GetMIDIPort" object:nil];

  return self;
}

- (void) receiveMIDI:(const struct MIDIEventList*) list
{
  mReceiveBlock(mTime, 0, list);
}

- (OSStatus) sendMIDI:(const struct MIDIEventList*) list
{
  return MIDISendEventList(mDestination.GetPort(), mDestination.Get(), list);
}

- (void) setAUAudioUnit:(AUAudioUnit*) audiounit
{
  if (@available(iOS 15.0, *))
  {
    audiounit.hostMIDIProtocol = kMIDIProtocol_1_0;
    
#if PLUG_DOES_MIDI_IN
    mReceiveBlock = audiounit.scheduleMIDIEventListBlock;
#endif
    
#if PLUG_DOES_MIDI_OUT
    audiounit.MIDIOutputEventListBlock = ^(AUEventSampleTime eventSampleTime, 
                                           uint8_t cable,
                                           const struct MIDIEventList* list) {
      return [self sendMIDI:list];
    };
#endif
  }
  
  [audiounit tokenByAddingRenderObserver:^(AudioUnitRenderActionFlags actionFlags, 
                                           const AudioTimeStamp * _Nonnull timestamp,
                                           AUAudioFrameCount frameCount,
                                           NSInteger outputBusNumber) {
    self->mTime = timestamp->mSampleTime;
  }];
}

- (void) updateConnections
{
  mSource.Update();
  mDestination.Update();
}

- (void) receiveNotification:(NSNotification*) notification
{
  if ([notification.name isEqualToString:@"SetMIDIPort"])
  {
    NSDictionary* dict = notification.userInfo;
    NSString* name = (NSString*) dict[@"name"];
    NSString* direction = (NSString*) dict[@"direction"];
    
    if ([direction compare:[[NSString alloc] initWithUTF8String:"source"]] == NSOrderedSame)
    {
      mSource.SetName([name cStringUsingEncoding:NSUTF8StringEncoding]);
    }
    else
    {
      mDestination.SetName([name cStringUsingEncoding:NSUTF8StringEncoding]);
    }
  }
  else if ([notification.name isEqualToString:@"GetMIDIPort"])
  {
    NSDictionary* dict = notification.userInfo;
    WDL_String* name = (WDL_String*) [(NSValue*) dict[@"name"] pointerValue];
    NSString* direction = (NSString*) dict[@"direction"];
    
    if ([direction compare:[[NSString alloc] initWithUTF8String:"source"]] == NSOrderedSame)
    {
      mSource.GetName(*name);
    }
    else
    {
      mDestination.GetName(*name);
    }
  }
}

@end
