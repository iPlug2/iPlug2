#import <Cocoa/Cocoa.h>
#include <AudioUnit/AudioUnit.h>
#include <AudioUnit/AUCocoaUIView.h>

#include "Log.h"
#include "config.h"   // This is your plugin's config.h.

#ifdef NO_IGRAPHICS
#include "IPlugBase.h"
typedef IPlugBase IPLUG_BASE_CLASS;
#else
#include "IPlugBaseGraphics.h"
typedef IPlugBaseGraphics IPLUG_BASE_CLASS;
#endif
static const AudioUnitPropertyID kIPlugObjectPropertyID = UINT32_MAX-100;

@interface VIEW_CLASS : NSObject <AUCocoaUIBase>
{
  IPLUG_BASE_CLASS* mPlug;
}
- (id) init;
- (NSView*) uiViewForAudioUnit: (AudioUnit) audioUnit withSize: (NSSize) preferredSize;
- (unsigned) interfaceVersion;
- (NSString*) description;
@end

@implementation VIEW_CLASS

- (id) init
{
  TRACE;  
  mPlug = nullptr;
  return [super init];
}

- (NSView*) uiViewForAudioUnit: (AudioUnit) audioUnit withSize: (NSSize) preferredSize
{
  TRACE;

  void* pointers[1];
  UInt32 propertySize = sizeof (pointers);
  
  if (AudioUnitGetProperty (audioUnit, kIPlugObjectPropertyID,
                            kAudioUnitScope_Global, 0, pointers, &propertySize) == noErr)
  {
    mPlug = (IPLUG_BASE_CLASS*) pointers[0];
    if (mPlug) {
      if (mPlug->GetHasUI()) {
        NSView* pView = (NSView*) mPlug->OpenWindow(nullptr);
        mPlug->OnGUIOpen();
        return pView;
      }
    }
  }
  return 0;
}

- (unsigned) interfaceVersion
{
  return 0;
}

- (NSString *) description
{
  return [NSString stringWithCString:PLUG_NAME " View" encoding:NSUTF8StringEncoding];
}

@end


