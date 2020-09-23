/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for more info.

 ==============================================================================
*/

#include "System.h"

namespace iplug::windows
{
	static const class WindowsSystem::cpu_id
	{
	 public:
		cpu_id()
		{
			std::array<int, 4> cpui;

			__cpuid(cpui.data(), 0);
			nIds = cpui[0];

			for (int i = 0; i <= nIds; ++i)
			{
				__cpuidex(cpui.data(), i, 0);
				data.push_back(cpui);
			}
		}

		std::string vendor;
		std::string brand;

		int nIds                 = 0;
		int nExIds               = 0;
		bool isIntel             = false;
		bool isAMD               = false;
		std::bitset<32> f_1_ECX  = 0;
		std::bitset<32> f_1_EDX  = 0;
		std::bitset<32> f_7_EBX  = 0;
		std::bitset<32> f_7_ECX  = 0;
		std::bitset<32> f_81_ECX = 0;
		std::bitset<32> f_81_EDX = 0;

		std::vector<std::array<int, 4>> data    = {};
		std::vector<std::array<int, 4>> extdata = {};
	};

	const WindowsSystem::cpu_id System::m_cpu_id;


}  // namespace iplug::windows
