#pragma once

/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for more info.

 ==============================================================================
*/

/**
 * @file
 * @brief IPlug2 Precompiled headers
 */

#include "PAL/Platform.h"

#include <array>
#include <bitset>
#include <chrono>
#include <codecvt>
#include <functional>
#include <map>
#include <numeric>
#include <stack>
#include <string>

//#include <algorithm>
//#include <atomic>
//#include <cassert>
//#include <cctype>
//#include <cmath>
//#include <complex>
//#include <cstdarg>
//#include <cstdio>
//#include <cstdlib>
//#include <cstring>
//#include <ctime>
//#include <iostream>
//#include <limits>
//#include <list>
//#include <locale>
//#include <memory>
//#include <set>
//#include <unordered_map>
//#include <utility>
//#include <vector>


BEGIN_INCLUDE_DEPENDENCIES
#define WDL_NO_SUPPORT_UTF8
#include <WDL/dirscan.h>
#include <WDL/heapbuf.h>
#include <WDL/jnetlib/jnetlib.h>
#include <WDL/mutex.h>
#include <WDL/ptrlist.h>
#include <WDL/wdl_base64.h>
#include <WDL/wdlendian.h>
#include <WDL/wdlstring.h>
#include <WDL/wdlutf8.h>
#include <yoga/Yoga.h>

#if VST3_API || VST3C_API || VST3P_API
	#include <pluginterfaces/base/ibstream.h>
	#include <pluginterfaces/base/keycodes.h>
	#include <pluginterfaces/base/ustring.h>
	#include "pluginterfaces/vst/ivstcomponent.h"
	#include "pluginterfaces/vst/ivsteditcontroller.h"
	#include "pluginterfaces/vst/ivstmidicontrollers.h"
	#include <pluginterfaces/vst/ivstchannelcontextinfo.h>
	#include <pluginterfaces/vst/ivstcontextmenu.h>
	#include <pluginterfaces/vst/ivstevents.h>
	#include <pluginterfaces/vst/ivstparameterchanges.h>
	#include <pluginterfaces/vst/ivstprocesscontext.h>
	#include <pluginterfaces/vst/vstspeaker.h>
	#include <pluginterfaces/vst/vsttypes.h>
	#include <pluginterfaces/gui/iplugviewcontentscalesupport.h>
	#include <public.sdk/source/vst/vstaudioeffect.h>
	#include <public.sdk/source/vst/vstbus.h>
	#include <public.sdk/source/vst/vsteditcontroller.h>
	#include <public.sdk/source/vst/vsteventshelper.h>
	#include <public.sdk/source/vst/vstparameters.h>
	#include <public.sdk/source/vst/vstsinglecomponenteffect.h>
	#include <public.sdk/source/vst/hosting/parameterchanges.h>
	#include "public.sdk/source/main/pluginfactory.h"
#endif
END_INCLUDE_DEPENDENCIES

#include "IPlugConstants.h"
#include "IPlugLogger.h"
#include "IPlugMath.h"
#include "Extras/Easing.h"
#include "IPlugStructs.h"
#include "IPlugUtilities.h"
#include "IPlugMidi.h"
#include "IPlugParameter.h"
#include "IPlugQueue.h"
#include "IPlugPaths.h"
#include "IPlugTimer.h"
#include "IPlugPluginBase.h"
#include "IPlugAPIBase.h"
#include "IPlugEditorDelegate.h"
#include "IPlugProcessor.h"
#include "ISender.h"
#ifndef NO_IGRAPHICS
	#include "IGraphicsConstants.h"
	#include "IGraphicsStructs.h"
	#include "IGraphicsPopupMenu.h"
	#include "IGraphicsUtilities.h"
	#include "IGraphics.h"
	#include "IGraphics_select.h"
	#include "IControl.h"
	#include "IControls.h"
	#include "IBubbleControl.h"
	#include "IColorPickerControl.h"
	#include "ICornerResizerControl.h"
	#include "IFPSDisplayControl.h"
	#include "IGraphicsLiveEdit.h"
	#include "ILEDControl.h"
	#include "IPopupMenuControl.h"
	#include "IRTTextControl.h"
	#include "ITextEntryControl.h"
	#include "IVDisplayControl.h"
	#include "IVKeyboardControl.h"
	#include "IVMeterControl.h"
	#include "IVMultiSliderControl.h"
	#include "IVScopeControl.h"
#endif
