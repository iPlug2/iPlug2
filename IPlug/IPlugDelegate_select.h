#ifdef NO_IGRAPHICS
#include "IPlugDelegate.h"
typedef IDelegate IPLUG_DELEGATE;
#else
#include "IPlugGraphicsDelegate.h"
typedef IGraphicsDelegate IPLUG_DELEGATE;
#endif
