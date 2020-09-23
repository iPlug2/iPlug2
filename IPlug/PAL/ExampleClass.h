/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for more info.

 ==============================================================================
*/

#pragma once


namespace iplug::generic
{
	/**
	 * @brief Definition of a class used as the base for platform implementation.
	 *        In this case we create 'ExampleClass' with the prefix 'Generic' which
	 *        may or may not include functions that are available for all platforms.
	 *
	 *        If a class only have functions that works on all platforms without the
	 *        need for any specific implementations, then it shouldn't be defined
	 *        inside PAL.
	 */
	class GenericExampleClass
	{
	 protected:
		virtual ~GenericExampleClass() = default;
	};
}  // namespace iplug::generic

// Include after GenericXXX class definition in global namespace
#include PLATFORM_HEADER(ExampleClass.h)
