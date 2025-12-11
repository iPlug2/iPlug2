//------------------------------------------------------------------------
// Project     : ASIO SDK
//
// Category    : Helpers
// Filename    : common/debugmessage.cpp
// Created by  : Steinberg, 05/1996
// Description : Steinberg Audio Stream I/O Helpers
//
//-----------------------------------------------------------------------------
// This file is part of a Steinberg SDK. It is subject to the license terms
// in the LICENSE file found in the top-level directory of this distribution
// and at www.steinberg.net/sdklicenses. 
// No part of the SDK, including this file, may be copied, modified, propagated,
// or distributed except according to the terms contained in the LICENSE file.
//-----------------------------------------------------------------------------

#include "asiosys.h"

#if DEBUG
#if MAC
#include <TextUtils.h>
void DEBUGGERMESSAGE(char *string)
{
	c2pstr(string);
	DebugStr((unsigned char *)string);
}
#else
#error debugmessage
#endif
#endif
