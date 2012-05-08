#import "IPlugMultiTargetsAppDelegate.h"
#include "IOSLink.h"

IOSLink::IOSLink(void* appDelegate)
{
  mAppDelegate = appDelegate;
}

void IOSLink::SendMidiMsg(IMidiMsg* pMsg)
{
  [((IPlugMultiTargetsAppDelegate*)mAppDelegate) sendMidiMsg:pMsg];
}