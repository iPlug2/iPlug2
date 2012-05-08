#ifndef _IOSLINK_
#define _IOSLINK_

// this is just used so that it's possible to call into the objective c code from the C++ - in order to send midi

#include "IPlugStructs.h"

class IOSLink
{
public:
  IOSLink(void* appDelegate);

  void SendMidiMsg(IMidiMsg* pMsg);

private:
  void* mAppDelegate;
};

#endif // _IOSLINK_