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

#if !PLATFORM_WINDOWS
	#include <unistd.h>
#endif

//#include <stdint.h>
#include <sys/stat.h>

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
#include <string>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

#include "IPlugPreprocessor.h"
#include "PAL/PlatformCompiler.h"
#include "PAL/Platform.h"
#include "IPlugMath.h"
