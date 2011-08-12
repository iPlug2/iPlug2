#include "digicode.h"

#ifdef OS_WIN
 #undef _UNICODE
 #undef UNICODE

 #include "DLLMain.cpp"
 #include "DefaultSwap.cpp"

#else // OS_OSX
 #include "PlugInInitialize.cpp"
 #include "Dispatcher.cpp"
#endif