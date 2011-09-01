#include "digicode.h"

#ifdef OS_WIN
 #undef _UNICODE
 #undef UNICODE
#endif

#include "CEffectGroup.cpp"
#include "CEffectGroupMIDI.cpp"
#include "CEffectMIDIUtils.cpp"
#include "CEffectProcess.cpp"
#include "CEffectProcessAS.cpp"
#include "CEffectType.cpp"
#include "CEffectTypeRTAS.cpp"
#include "CEffectTypeAS.cpp"
#include "ChunkDataParser.cpp"