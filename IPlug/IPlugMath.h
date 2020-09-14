/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for more info.

 ==============================================================================
*/

#pragma once

#include "IPlugMathConstants.h"


namespace iplug::math
{
	enum class ERound
	{
		Ceil,
		Floor,
		Nearest
	};

	//-----------------------------------------------------------------------------

	template <class T>
	NODISCARD inline constexpr T Abs(const T value)
	{
		static_assert(type::IsArithmetic<T>);
		if constexpr (type::IsUnsigned<T>)
			return value;
		return value >= 0 ? value : -value;
	}

	template <class T>
	NODISCARD inline constexpr T Tan(const T value)
	{
		static_assert(type::IsMathArithmetic<T>);
		return tan(value);
	}

	//-----------------------------------------------------------------------------

	template <class Tr = bool, class T>
	NODISCARD inline constexpr Tr IsNegative(const T value)
	{
		static_assert(type::IsArithmetic<T>);
		return value < 0;
	}

	template <class Tr = bool, class T>
	NODISCARD inline constexpr Tr IsPositive(const T value)
	{
		static_assert(type::IsArithmetic<T>);
		return value >= 0;
	}

	// True if value is within min and max-1, to test min to max, use ClampEval
	template <class Tr = bool, class T>
	NODISCARD inline constexpr Tr IsWithin(const T value, const T min, const T max)
	{
		static_assert(type::IsArithmetic<T>);
		if constexpr (type::IsIntegral<T>)
			return ((value - min) | (max - value - 1)) >= 0;
		else
			return value < min ? false : value < max ? true : false;
	}

	// True if value is smaller than given threshold or delta constant (Amplitude of -100dB)
	template <class Tr = bool, class T>
	NODISCARD inline constexpr Tr IsBelowThreshold(const T value, const T threshold = constants::delta_v<T>)
	{
		static_assert(type::IsFloatingPoint<T>);
		return (Abs(value) < threshold);
	}

	// True if value is higher than given threshold or delta constant (Amplitude of -100dB )
	template <class Tr = bool, class T>
	NODISCARD inline constexpr Tr IsAboveThreshold(const T value, const T threshold = constants::delta_v<T>)
	{
		static_assert(type::IsFloatingPoint<T>);
		return (Abs(value) > threshold);
	}

	// True if value is, or is close to 0.0 based on given threshold or macheps32 constant
	template <class Tr = bool, class T>
	NODISCARD inline constexpr Tr IsNearlyZero(const T value, const T threshold = constants::macheps32_v<T>)
	{
		static_assert(type::IsFloatingPoint<T>);
		return (Abs(value) <= threshold);
	}

	// True if value A is, or is close to value B based on given threshold or macheps32 constant
	template <class Tr = bool, class T>
	NODISCARD inline constexpr Tr IsNearlyEqual(const T A, const T B, const T threshold = constants::macheps32_v<T>)
	{
		static_assert(type::IsFloatingPoint<T>);
		return (Abs(A - B) <= threshold);
	}

	template <class Tr = bool, class T>
	NODISCARD inline constexpr Tr IsPowerOfTwo(const T value)
	{
		static_assert(type::IsArithmetic<T>);
		if constexpr (type::IsIntegral<T>)
			return (value & (value - 1)) == 0;
		else
			return (static_cast<uint32>(value) & (static_cast<uint32>(value) - 1)) == 0;
	}

	template <class Tr = bool, class T>
	NODISCARD inline constexpr Tr SignBit(const T value)
	{
		static_assert(sizeof(T) <= 8 && type::IsArithmetic<T>);
		if constexpr (sizeof(T) == 1)
			return *(uint8*) &value >= 0x80u;
		if constexpr (sizeof(T) == 2)
			return *(uint16*) &value >= 0x8000u;
		if constexpr (sizeof(T) == 4)
			return *(uint32*) &value >= 0x80000000u;
		else if constexpr (sizeof(T) == 8)
			return *(uint64*) &value >= 0x8000000000000000u;
	}


	//-----------------------------------------------------------------------------

	/** Clamps a value between min and max
	 * @param value Input value
	 * @param min Minimum value to be allowed
	 * @param max Maximum value to be allowed
	 * If value is outside given range, it will be set to one of the boundaries */
	template <class T, class Tx>
	NODISCARD inline constexpr T Clamp(const T value, const Tx min, const Tx max)
	{
		static_assert(type::IsArithmetic<T>);
		static_assert(type::IsArithmetic<Tx>);
		return value < min ? static_cast<T>(min) : value < max ? value : static_cast<T>(max);
	}

	// True if value would be clamped
	template <class Tr = bool, class T, class Tx>
	NODISCARD inline constexpr Tr ClampEval(const T value, const Tx min, const Tx max)
	{
		static_assert(type::IsArithmetic<T>);
		static_assert(type::IsArithmetic<Tx>);
		if constexpr (type::IsIntegral<T> && type::IsIntegral<Tx>)
			return (((value - static_cast<T>(min)) | (static_cast<T>(max) - value)) >= 0);
		else
			return value < min ? false : value <= max ? true : false;
	}

	template <class T>
	NODISCARD inline constexpr T Round(const T value)
	{
		static_assert(type::IsFloatingPoint<T>);
		if constexpr (type::IsSame<T, float>)
			return floorf(value + 0.5f);
		else
			return floor(value + 0.5);
	}

	template <class T>
	NODISCARD inline constexpr T Ceil(const T value)
	{
		static_assert(type::IsFloatingPoint<T>);
		return ceil(value);
	}

	template <class T>
	NODISCARD inline constexpr T Floor(const T value)
	{
		static_assert(type::IsFloatingPoint<T>);
		return floor(value);
	}

	template <class T>
	NODISCARD inline constexpr int RoundToInt(const T value)
	{
		return static_cast<int>(Round(value));
	}

	template <class T>
	NODISCARD inline constexpr int FloorToInt(const T value)
	{
		return static_cast<int>(Floor(value));
	}

	template <class T>
	NODISCARD inline constexpr int CeilToInt(const T value)
	{
		return static_cast<int>(Ceil(value));
	}

	// Returns the signed fractional portion of value
	template <class T>
	NODISCARD inline constexpr T Fraction(const T value)
	{
		static_assert(type::IsFloatingPoint<T>);
		return value - FloorToInt(value) + -IsNegative<int>(value);
	}

	template <class T>
	NODISCARD inline constexpr T FractionAbs(const T value)
	{
		static_assert(type::IsFloatingPoint<T>);
		return value - Floor(value);
	}

	template <ERound rounding = ERound::Floor, class Tx, class Ty>
	NODISCARD inline constexpr auto IntegralDivide(const Tx dividend, const Ty divisor)
	{
		static_assert(type::IsIntegral<Tx> && type::IsIntegral<Ty>);
		if constexpr (rounding == ERound::Ceil)
			return (dividend + divisor - 1) / divisor;
		if constexpr (rounding == ERound::Floor)
			return dividend / divisor;
		if constexpr (rounding == ERound::Nearest)
			return (dividend >= 0) ? (dividend + divisor / 2) / divisor : (dividend - divisor / 2 + 1) / divisor;
	}

	template <class T, class Ta>
	NODISCARD inline constexpr T Align(const T value, const Ta alignment)
	{
		static_assert(type::IsArithmetic<T>);
		static_assert(type::IsArithmetic<Ta>);
		if (value == 0 || alignment == 0)
			return value;
		if constexpr (type::IsSame<T, float> && type::IsSame<Ta, float>)
			return Floor((value + 0.5f * alignment) / alignment) * alignment;
		else
		{
			float v = static_cast<float>(value);
			float a = static_cast<float>(alignment);
			return Floor<T>((v + 0.5f * a) / a) * a;
		}
	}

	template <class T>
	NODISCARD inline constexpr T Pow2(T value)
	{
		static_assert(type::IsFloatingPoint<T>);
		if (type::IsSame<T, float>)
			return powf(2.0f, value);
		else
			return pow(2.0, value);
	}

	// Fast approximation Pow2
	template <class T>
	NODISCARD inline constexpr T fPow2(T value)
	{
		static_assert(type::IsFloatingPoint<T>);
		float v  = static_cast<float>(value);  // Clamp(value, -126.0f, 128.0f);
		float fr = Fraction(v);
		union
		{
			uint32 i;
			float f;
		} u;
		u.i = static_cast<uint32>((1 << 23) * (v + 121.2740575f + 27.7280233f / (4.84252568f - fr) - 1.49012907f * fr));
		return u.f;
	}

	// Fast approximation sqrt using lut
	NODISCARD inline float fsqrt(const float value)
	{
		static_assert(Platform::IsLittleEndian(), "Big Endians not implemented yet");

		// internal lookup table for fsqrt
		static const uint32* _fsqrt_lut = [] {
			CACHE_ALIGN(32)
			static uint32 array[0x10000];
			union
			{
				float f;
				uint32 i;
			} u;

			for (int i = 0; i <= 0x7fff; ++i)
			{
				u.i               = (i << 8) | (0x7f << 23);
				u.f               = sqrtf(u.f);
				array[i + 0x8000] = (u.i & 0x7fffff);
				u.i               = (i << 8) | (0x80 << 23);
				u.f               = sqrtf(u.f);
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

	template <class T>
	NODISCARD inline constexpr T AlignPow2(T value, T alignment = 16)
	{
		static_assert(type::IsMathIntegral<T>);
		return (value + alignment - 1) & ~(alignment - 1);
	}

	// Returns nearest power of two value that is greater than or equal to value
	template <class T>
	NODISCARD inline constexpr T Pow2Ceil(T value)
	{
		static_assert(type::IsMathArithmetic<T>);
		if constexpr (Platform::IsLittleEndian())
		{
			union
			{
				double f;
				uint64 i;
			} u;
			u.i = static_cast<uint64>(Abs(value));
			u.i <<= IsPowerOfTwo<int>(u.i);
			u.f = static_cast<double>(u.i);
			u.i &= 0xfff0000000000000u;
			if (value < 0)
				u.f = -u.f;
			return static_cast<T>(u.f);
		}

		uint64 v = static_cast<uint64>(Abs(value));
		v <<= IsPowerOfTwo<int>(value);
		T u = static_cast<T>(ldexp(1.0, ilogb(v)));
		if (value < 0)
			u = -u;
		return u;
	}

	// Returns nearest power of two value that is less than or equal to value
	template <class T>
	NODISCARD inline constexpr T Pow2Floor(T value)
	{
		static_assert(type::IsMathArithmetic<T>);
		if constexpr (Platform::IsLittleEndian())
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


	//-----------------------------------------------------------------------------

	// Degrees to radians
	template <class T>
	NODISCARD inline constexpr auto DegToRad(const T Degrees)
	{
		static_assert(type::IsMathArithmetic<T>);
		return Degrees * constants::inv_rad_v<T>;
	}

	// Radians to degrees
	template <class T>
	NODISCARD inline constexpr auto RadToDeg(const T Radians)
	{
		static_assert(type::IsMathArithmetic<T>);
		return Radians * constants::rad_v<T>;
	}

	/**
	 * @brief Calculates amplitude from a given dB value
	 * @param dB Value
	 * @return Gain calculated as an approximation of e^(inv_Np*dB)
	 */
	template <class T>
	NODISCARD inline constexpr auto DBToAmp(const T dB)
	{
		static_assert(type::IsFloatingPoint<T>);
		return exp(constants::inv_Np_v<T> * dB);
	}

	/**
	 * @brief Calculates dB from a given amplitude value
	 * @param Amplitude Value
	 * @return dB calculated as an approximation of Np*log(Amplitude)
	 */
	template <class T>
	NODISCARD inline constexpr auto AmpToDB(const T Amplitude)
	{
		static_assert(type::IsFloatingPoint<T>);
		return constants::Np_v<T> * log(Abs(Amplitude));
	}

	// AmpToDB using fast approximation
	NODISCARD inline const auto AmpToDBf(const float Amplitude)
	{
		int e;
		float f = frexpf(Abs(Amplitude), &e);
		float y = 1.23149591368684f;
		y *= f;
		y += -4.11852516267426f;
		y *= f;
		y += 6.02197014179219f;
		y *= f;
		y += -3.13396450166353f;
		y += e;
		y *= 6.020599913279624f;
		return y;
	}


}  // namespace iplug::math
