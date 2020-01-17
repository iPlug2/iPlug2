/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/
#ifndef __IPLUGVST3_RUNLOOP
#define __IPLUGVST3_RUNLOOP

#include <memory>

#include "IPlugPlatform.h"

#include "IPlugTimer.h"

#include "base/source/fobject.h"
#include "pluginterfaces/gui/iplugview.h"

#include "xcbt.h"

BEGIN_IPLUG_NAMESPACE

/** Create XCBT embed using VST3 IRunLoop interface
 * @param frame an object with IRunLoop interface
 * @return allocated embed structure in case the initialization could be done on nullptr*/
xcbt_embed *IPlugVST3_EmbedFactory(Steinberg::FUnknown *frame);

END_IPLUG_NAMESPACE

#endif
