/*
 ==============================================================================
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 See LICENSE.txt for more info.
 ==============================================================================
*/
#pragma once

// WDL headers use these allocation declarations without always including them.
#include <stdlib.h>

#if defined(__linux__) && defined(__clang__)
#include <endian.h>
// Clang omits GCC's floating-word-order macro; use the libc ABI definition.
#if !defined(__FLOAT_WORD_ORDER__) && defined(__FLOAT_WORD_ORDER)
#define __FLOAT_WORD_ORDER__ __FLOAT_WORD_ORDER
#endif
// Clang reports GCC 4.2 compatibility, which selects WDL's legacy Linux byte
// swap branch. glibc names these bswap_16/32/64 rather than bswap16/32/64.
// Supply the names expected by that branch without changing the vendored WDL.
#ifndef bswap16
#define bswap16(x) __builtin_bswap16(x)
#endif
#ifndef bswap32
#define bswap32(x) __builtin_bswap32(x)
#endif
#ifndef bswap64
#define bswap64(x) __builtin_bswap64(x)
#endif
#endif
