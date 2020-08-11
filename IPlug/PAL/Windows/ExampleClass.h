/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for more info.
 
 ==============================================================================
*/

#pragma once

// Note. Do not include any platform specific headers in this file

#include "../ExampleClass.h"

namespace iplug
{
	class ExampleClass final : public Platform::Generic::GenericExampleClass
	{
	  public:
		ExampleClass() {};
		~ExampleClass() {};

	};
}  // namespace iplug
