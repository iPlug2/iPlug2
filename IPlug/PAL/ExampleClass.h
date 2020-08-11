/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for more info.
 
 ==============================================================================
*/

#pragma once

#include "IPlugPlatform.h"

namespace iplug::Platform::Generic
{
	class GenericExampleClass
	{
	  protected:
		GenericExampleClass()  = default;
		~GenericExampleClass() = default;
	};
}  // namespace iplug::Generic

// Include last
#include PLATFORM_HEADER(ExampleClass.h)
