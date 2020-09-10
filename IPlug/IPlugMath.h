/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for more info.

 ==============================================================================
*/

#pragma once

#include "IPlugMathConstants.h"

// temporary until finding a suitable location
namespace iplug
{
	inline constexpr bool IsLittleEndian()
	{
		return (PLATFORM_LITTLE_ENDIAN == 1);
	}
}  // namespace iplug


namespace iplug::math
{
	// True if value is smaller than given threshold or delta constant
	template <class T = tfloat>
	inline constexpr bool IsBelowThreshold(const T value, const T threshold = constants::delta_v<T>)
	{
		static_assert(std::is_floating_point_v<T>);
		return (abs(value) < threshold);
	}

	// True if value is, or is close to 0.0 based on given threshold or macheps32 constant
	template <class T = tfloat>
	inline constexpr bool IsNearlyZero(const T value, const T threshold = constants::macheps32_v<T>)
	{
		static_assert(std::is_floating_point_v<T>);
		return (abs(value) <= threshold);
	}

	// True if value A is, or is close to value B based on given threshold or macheps32 constant
	template <class T = tfloat>
	inline constexpr bool IsNearlyEqual(const T A, const T B, const T threshold = constants::macheps32_v<T>)
	{
		static_assert(std::is_floating_point_v<T>);
		return (abs(A - B) <= threshold);
	}

	template <class T> inline constexpr bool IsPowerOfTwo(const T value)
	{
		return ((value & (value - 1)) == 0);
	}

	// Returns nearest power of two value that is greater than or equal to value
	template <class T> inline constexpr T PowerOfTwoCeil(T value)
	{
		static_assert(std::is_arithmetic_v<T>);
		if constexpr (IsLittleEndian())
		{
			union
			{
				double f;
				uint64 i;
			} u;
			u.i = static_cast<uint64>(abs(value));
			u.i <<= ((u.i & (u.i - 1)) != 0);
			u.f = static_cast<double>(u.i);
			u.i &= 0xfff0000000000000u;
			if (value < 0)
				u.f = -u.f;
			return static_cast<T>(u.f);
		}

		uint64 v = static_cast<uint64>(abs(value));
		v <<= ((value & (value - 1)) != 0);
		T u = static_cast<T>(ldexp(1.0, ilogb(v)));
		if (value < 0)
			u = -u;
		return u;
	}

	// Returns nearest power of two value that is less than or equal to value
	template <class T> inline constexpr T PowerOfTwoFloor(T value)
	{
		static_assert(std::is_arithmetic_v<T>);
		if constexpr (IsLittleEndian())
		{
			union
			{
				double f;
				uint64 i;
			} u;
			u.f = static_cast<double>(value);
			u.i &= 0xfff0000000000000u;
			if (value < 0)
				u.f = -u.f;
			return static_cast<T>(u.f);
		}

		T u = static_cast<T>(ldexp(1.0, ilogb(value)));
		if (value < 0)
			u = -u;
		return u;
	}

	// Degrees to radians
	template <class T = tfloat> inline constexpr auto DegToRad(const T Degrees)
	{
		static_assert(std::is_arithmetic_v<T>);
		return Degrees * constants::inv_rad_v<T>;
	}

	// Radians to degrees
	template <class T = tfloat> inline constexpr auto RadToDeg(const T Radians)
	{
		static_assert(std::is_arithmetic_v<T>);
		return Radians * constants::rad_v<T>;
	}

	/**
	 * @brief Calculates amplitude from a given dB value
	 * @param dB Value
	 * @return Gain calculated as an approximation of e^(inv_Np*dB)
	 */
	template <class T = tfloat> inline constexpr auto DBToAmp(const T dB)
	{
		static_assert(std::is_floating_point_v<T>);
		return exp(constants::inv_Np_v<T> * dB);
	}

	/**
	 * @brief Calculates dB from a given amplitude value
	 * @param Amplitude Value
	 * @return dB calculated as an approximation of Np*log(Amplitude)
	 */
	template <class T = tfloat> inline constexpr auto AmpToDB(const T Amplitude)
	{
		static_assert(std::is_floating_point_v<T>);
		return constants::Np_v<T> * log(fabs(Amplitude));
	}

	// AmpToDB using fast approximation
	inline const auto AmpToDBf(const float Amplitude)
	{
		int E;
		float F = frexpf(fabsf(Amplitude), &E);
		float Y = 1.23149591368684f;
		Y *= F;
		Y += -4.11852516267426f;
		Y *= F;
		Y += 6.02197014179219f;
		Y *= F;
		Y += -3.13396450166353f;
		Y += E;
		Y *= 6.020599913279624f;
		return Y;
	}

	inline float fsqrt(const float value)
	{
		static_assert(IsLittleEndian(), "Big Endians not implemented yet");

		// internal lookup table for fsqrt
		static const uint32* _fsqrt_lut = [] {
			PLATFORM_CACHE_ALIGN
			static uint32 array[0x10000];
			union
			{
				float f;
				uint32 i;
			} u;

			for (int i = 0; i <= 0x7fff; ++i)
			{
				u.i               = (i << 8) | (0x7f << 23);
				u.f               = static_cast<float>(sqrt(u.f));
				array[i + 0x8000] = (u.i & 0x7fffff);
				u.i               = (i << 8) | (0x80 << 23);
				u.f               = static_cast<float>(sqrt(u.f));
				array[i]          = (u.i & 0x7fffff);
			}
			return array;
		}();

		uint32 fbits = *(uint32*) &value;
		if (fbits == 0)
			return 0.0f;

		*(uint32*) &value =
			_fsqrt_lut[(fbits >> 8) & 0xffff] | ((((fbits - 0x3f800000) >> 1) + 0x3f800000) & 0x7f800000);

		return value;
	}
}  // namespace iplug::math
