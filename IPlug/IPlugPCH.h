#pragma once

/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for more info.
 
 ==============================================================================
*/


/**
 * @file
 * @brief IPlug2 PreCompiledHeaders include file. Should be included first
 */


//-----------------------------------------------------------------------------
// STD headers

#include <algorithm>
#include <array>
#include <atomic>
#include <bitset>
#include <cassert>
#include <cctype>
#include <chrono>
#include <cmath>
#include <codecvt>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <functional>
#include <iostream>
#include <limits>
#include <list>
#include <locale>
#include <map>
#include <memory>
#include <numeric>
#include <set>
#include <stack>
#include <stdint.h>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include <sys/stat.h>

#if !PLATFORM_WINDOWS
	#include <unistd.h>
#endif

#define _USE_MATH_DEFINES
#include <math.h>
//-----------------------------------------------------------------------------

#include "IPlugPreprocessor.h"
#include "PAL/PlatformCompiler.h"
#include "PAL/Platform.h"
#include "IPlugMath.h"
