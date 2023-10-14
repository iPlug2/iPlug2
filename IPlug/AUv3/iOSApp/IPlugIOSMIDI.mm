
#import <Foundation/Foundation.h>

#include "IPlugIOSMIDI.h"
#include "config.h"

// Static Helpers

//static
ItemCount IPlugIOSMIDI::GetNumSourcesOrDestinations(ConnectionType type)
{
  if (type == ConnectionType::Source)
    return MIDIGetNumberOfSources();
  else
    return MIDIGetNumberOfDestinations();
}

//static
MIDIEndpointRef IPlugIOSMIDI::GetEndpoint(ItemCount idx, ConnectionType type)
{
  if (type == ConnectionType::Source)
    return MIDIGetSource(idx);
  else
    return MIDIGetDestination(idx);
}

//static
CFStringRef IPlugIOSMIDI::CreateNameFromMIDEndpoint(MIDIEndpointRef endpoint)
{
  CFStringRef names[2];
  MIDIEntityRef entity;
  MIDIDeviceRef device;
  
  MIDIEndpointGetEntity(endpoint, &entity);
  MIDIEntityGetDevice(entity, &device);
  
  MIDIObjectGetStringProperty(device, kMIDIPropertyName, names + 0);
  MIDIObjectGetStringProperty(endpoint, kMIDIPropertyName, names + 1);
  
  CFStringRef separator = CFStringCreateWithCString(NULL, ": ", kCFStringEncodingUTF8);
  
  CFArrayRef array = CFArrayCreate(NULL, (const void **) names, 2, NULL);
  CFStringRef name = CFStringCreateByCombiningStrings(NULL, array, separator);
  
  CFRelease(array);
  CFRelease(separator);
  
  return name;
}

//static
CFStringRef IPlugIOSMIDI::CreateNameFromIndex(ItemCount idx, ConnectionType type)
{
  return CreateNameFromMIDEndpoint(GetEndpoint(idx, type));
}

//static
void IPlugIOSMIDI::GetNameFromIndex(WDL_String &string, int idx, ConnectionType type)
{
  char cString[2048];
  
  CFStringRef str = CreateNameFromIndex(idx, type);
  CFStringGetCString(str, cString, sizeof(cString), kCFStringEncodingUTF8);
  CFRelease(str);
  string.Set(cString);
}

//static
long IPlugIOSMIDI::GetIndexFromName(CFStringRef name, ConnectionType type)
{
  auto numEndPoints = GetNumSourcesOrDestinations(type);
  long idx = -1;
  
  for (long i = 0; i < numEndPoints && idx < 0; i++)
  {
    auto str = CreateNameFromMIDEndpoint(GetEndpoint(i, type));
    if (CFStringCompare(name, str, 0) == kCFCompareEqualTo)
      idx = i;
    CFRelease(str);
  }
  
  return idx;
}

//static
long IPlugIOSMIDI::GetIndexFromName(WDL_String name, ConnectionType type)
{
  CFStringRef str = CFStringCreateWithCString(NULL, name.Get(), kCFStringEncodingUTF8);
  long idx = GetIndexFromName(str, type);
  CFRelease(str);
  
  return idx;
}

//static
void IPlugIOSMIDI::SetMidiPort(const char *name, ConnectionType type)
{
  NSDictionary* dic = @{@"name": @(name), @"direction": @(type == ConnectionType::Source ? "Source" : "Destination")};
  [[NSNotificationCenter defaultCenter] postNotificationName:@"SetMIDIPort" object:nil userInfo:dic];
}

//static
void IPlugIOSMIDI::GetMidiPort(WDL_String& name, ConnectionType type)
{
  NSDictionary* dic = @{@"name": [NSValue valueWithPointer:&name], @"direction": @(type == ConnectionType::Source ? "Source" : "Destination")};
  [[NSNotificationCenter defaultCenter] postNotificationName:@"GetMIDIPort" object:nil userInfo:dic];
}

// Class

@implementation IPlugIOSMIDIHost
{
  MIDIClientRef mClient;
  MIDIPortRef mInPort;
  MIDIPortRef mOutPort;
  IPlugIOSMIDI::MIDICachedSource mSource;
  IPlugIOSMIDI::MIDICachedDestination mDestination;
  AUMIDIEventListBlock mReceiveBlock;
};

- (IPlugIOSMIDIHost*) init
{
  CFStringRef midiClientName = CFStringCreateWithCString(NULL, "MIDI Client", kCFStringEncodingUTF8);
  
  MIDIClientCreateWithBlock(midiClientName, &mClient, ^(const MIDINotification *message)
                            {
    if (message->messageID == kMIDIMsgSetupChanged)
      [self updateConnections];
  });
  
  CFRelease(midiClientName);
  
#if PLUG_DOES_MIDI_IN
  CFStringRef midiInputPortName = CFStringCreateWithCString(NULL, "MIDI Input", kCFStringEncodingUTF8);
  MIDIInputPortCreateWithProtocol(mClient, midiInputPortName, kMIDIProtocol_1_0, &mInPort, ^(const MIDIEventList *evtlist, void * __nullable srcConnRefCon)
                                  {
    [self receiveMIDI:evtlist];
  });
  CFRelease(midiInputPortName);
#endif
  
#if PLUG_DOES_MIDI_OUT
  CFStringRef midiOutputPortName = CFStringCreateWithCString(NULL, "MIDI Output", kCFStringEncodingUTF8);
  MIDIOutputPortCreate(mClient, midiOutputPortName, &mOutPort);
  CFRelease(midiOutputPortName);
#endif
  
  [self updateConnections];
  
  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(receiveNotification:) name:@"SetMIDIPort" object:nil];
  [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(receiveNotification:) name:@"GetMIDIPort" object:nil];

  return self;
}

- (void) receiveMIDI:(const struct MIDIEventList*) list
{
  mReceiveBlock(0, 0, list);
}

- (OSStatus) sendMIDI:(const struct MIDIEventList*) list
{
  return MIDISendEventList(mOutPort, mDestination.Get(), list);
}

- (void) setMIDIReceiveBlock:(AUMIDIEventListBlock) block
{
  mReceiveBlock = block;
}

- (void) updateConnections
{
  mSource.Update(mInPort);
  mDestination.Update();
}

- (void) receiveNotification:(NSNotification*) notification
{
  if ([notification.name isEqualToString:@"SetMIDIPort"])
  {
    NSDictionary* dict = notification.userInfo;
    NSString* name = (NSString*) dict[@"name"];
    NSString* direction = (NSString*) dict[@"direction"];
    
    if ([direction compare:[[NSString alloc] initWithUTF8String:"Source"]] == NSOrderedSame)
      mSource.SetName([name cStringUsingEncoding:NSUTF8StringEncoding], mInPort);
    else
      mDestination.SetName([name cStringUsingEncoding:NSUTF8StringEncoding]);
  }
  else if ([notification.name isEqualToString:@"GetMIDIPort"])
  {
    NSDictionary* dict = notification.userInfo;
    WDL_String* name = (WDL_String*) [(NSValue*) dict[@"name"] pointerValue];
    NSString* direction = (NSString*) dict[@"direction"];
    
    if ([direction compare:[[NSString alloc] initWithUTF8String:"Source"]] == NSOrderedSame)
      mSource.GetName(*name);
    else
      mDestination.GetName(*name);
  }
}

@end
