#include "IPlugAPP_host_notifications.h"
#import <AVFoundation/AVFoundation.h>

using namespace iplug;

NotificationListener::NotificationListener()
{
  
  mpDevicesChangedID = [[NSNotificationCenter defaultCenter] addObserverForName:@"deviceListChanged" object:nil queue:nil usingBlock:^(NSNotification* pNotification) {
    
  }];
  
  mpSampleRateChangedID = [[NSNotificationCenter defaultCenter] addObserverForName:@"AVAudioSessionSampleRateDidChangeNotification" object:nil queue:nil usingBlock:^(NSNotification* pNotification) {
    
  }];
}

NotificationListener::~NotificationListener()
{
  [[NSNotificationCenter defaultCenter] removeObserver:(id) mpDevicesChangedID];
  [[NSNotificationCenter defaultCenter] removeObserver:(id) mpSampleRateChangedID];
}


