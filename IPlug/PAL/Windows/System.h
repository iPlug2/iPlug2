/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for more info.

 ==============================================================================
*/


#pragma once

#include "../System.h"

namespace iplug::windows
{
	class WindowsSystem : public iplug::generic::GenericSystem
	{
	 private:
		class cpu_id;

	 public:

	 private:
		static const cpu_id m_cpu_id;
	};
}  // namespace iplug::windows


// Declare the class for direct usage in iplug namespace
namespace iplug
{
	using System = iplug::windows::WindowsSystem;
}
