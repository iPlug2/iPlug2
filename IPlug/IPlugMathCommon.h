/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for more info.

 ==============================================================================
*/


#pragma once

namespace iplug::math
{
	enum class ERound
	{
		None,
		Ceil,
		Floor,
		Nearest
	};


	//-----------------------------------------------------------------------------

	template <class T>
	NODISCARD inline constexpr T Abs(const T& value)
	{
		static_assert(type::IsArithmetic<T>);
		if constexpr (type::IsUnsigned<T>)
			return value;
		return value >= 0 ? value : -value;
	}


	template <class T>
	NODISCARD inline constexpr T Tan(const T& value)
	{
		static_assert(type::IsArithmetic<T>);
		return tan(value);
	}


	template <class T>
	NODISCARD inline constexpr T Round(const T& value)
	{
		static_assert(type::IsFloatingPoint<T>);
		if constexpr (type::IsSame<T, float>)
			return floorf(value + 0.5f);
		else
			return floor(value + 0.5);
	}


	template <class T>
	NODISCARD inline constexpr T Ceil(const T& value)
	{
		static_assert(type::IsFloatingPoint<T>);
		return ceil(value);
	}


	template <class T>
	NODISCARD inline constexpr T Floor(const T& value)
	{
		static_assert(type::IsFloatingPoint<T>);
		return floor(value);
	}


	template <class T>
	NODISCARD inline constexpr int RoundToInt(const T& value)
	{
		return static_cast<int>(Round(value));
	}


	template <class T>
	NODISCARD inline constexpr int FloorToInt(const T& value)
	{
		return static_cast<int>(Floor(value));
	}


	template <class T>
	NODISCARD inline constexpr int CeilToInt(const T& value)
	{
		return static_cast<int>(Ceil(value));
	}


	// Returns the signed fractional portion of value
	template <class T>
	NODISCARD inline constexpr T Fraction(const T& value)
	{
		static_assert(type::IsFloatingPoint<T>);
		return value - FloorToInt(value) + -(value < 0);
	}


	template <class T>
	NODISCARD inline constexpr T FractionAbs(const T& value)
	{
		static_assert(type::IsFloatingPoint<T>);
		return value - Floor(value);
	}


	//-----------------------------------------------------------------------------

	template <class Tr = bool, class T>
	NODISCARD inline constexpr Tr IsNegative(const T& value)
	{
		static_assert(type::IsArithmetic<T>);
		return value < 0;
	}


	template <class Tr = bool, class T>
	NODISCARD inline constexpr Tr IsPositive(const T& value)
	{
		static_assert(type::IsArithmetic<T>);
		return value >= 0;
	}


	// True if value is within min and max-1, to test min to max, use ClampEval
	template <class Tr = bool, class T>
	NODISCARD inline constexpr Tr IsWithin(const T& value, const T& min, const T& max)
	{
		static_assert(type::IsArithmetic<T>);
		if constexpr (type::IsIntegral<T>)
			return ((value - min) | (max - value - 1)) >= 0;
		else
			return value < min ? false : value < max ? true : false;
	}


	// True if value A is, or is close to value B based on given threshold or macheps32 constant
	template <class Tr = bool, class T>
	NODISCARD inline constexpr Tr IsNearlyEqual(const T& A, const T& B, const T& threshold = constants::macheps32_v<T>)
	{
		static_assert(type::IsFloatingPoint<T>);
		return Abs(A - B) <= threshold;
	}


	// True if the absolute value is less or equal to the specified threshold, or macheps32 constant (1.19e-7)
	template <class Tr = bool, class T>
	NODISCARD inline constexpr Tr IsNearlyZero(const T& value, const T& threshold = constants::macheps32_v<T>)
	{
		static_assert(type::IsFloatingPoint<T>);
		return Abs(value) <= threshold;
	}


	// True if all 4 absolute values are less or equal to the specified threshold, or macheps32 constant (1.19e-7)
	template <class Tr = bool, class T>
	NODISCARD inline constexpr Tr IsNearlyZero(
		const T& A, const T& B, const T& C, const T& D, const T& threshold = constants::macheps32_v<T>)
	{
		return IsNearlyZero(A, threshold) && IsNearlyZero(B, threshold) && IsNearlyZero(C, threshold) &&
			   IsNearlyZero(D, threshold);
	}


	// True if the absolute value is less or equal to the delta constant (0.00001 = Amplitude of -100dB)
	template <class Tr = bool, class T>
	NODISCARD inline constexpr Tr IsNearlyZeroDelta(const T& value)
	{
		return IsNearlyZero(value, constants::delta_v<T>);
	}


	// True if all 4 values are smaller or equal to delta constant (0.00001 = Amplitude of -100dB)
	template <class Tr = bool, class T>
	NODISCARD inline constexpr Tr IsNearlyZeroDelta(const T& A, const T& B, const T& C, const T& D)
	{
		return IsNearlyZero(A, B, C, D, constants::delta_v<T>);
	}


	// True if a floating point value is nearly a integer value based on threshold or delta constant
	template <class Tr = bool, class T>
	NODISCARD inline constexpr Tr IsNearlyInteger(const T& value, const T& threshold = constants::delta_v<T>)
	{
		return IsNearlyEqual(value, Round(value), threshold);
	}


	// True if all 4 floating point values are nearly integer values based on threshold or delta constant
	template <class Tr = bool, class T>
	NODISCARD inline constexpr Tr IsNearlyInteger(
		const T& A, const T& B, const T& C, const T& D, const T& threshold = constants::delta_v<T>)
	{
		return IsNearlyInteger(A, threshold) && IsNearlyInteger(B, threshold) && IsNearlyInteger(C, threshold) &&
			   IsNearlyInteger(D, threshold);
	}


	template <class Tr = bool, class T>
	NODISCARD inline constexpr Tr IsPow2(const T& value)
	{
		static_assert(type::IsArithmetic<T>);
		if (value <= 0)
			return false;
		if constexpr (type::IsIntegral<T>)
			return (value & (value - 1)) == 0;
		else
		{
			using Tx = type::ConditionalUIntSize<T>;
			Tx v     = static_cast<Tx>(value);
			return (v & (v - 1)) == 0;
		}
	}


	template <class Tr = bool, class T>
	NODISCARD inline constexpr Tr SignBit(const T& value)
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
	template <class T>
	NODISCARD inline constexpr const T& Clamp(const T& value, const T& min, const T& max)
	{
		static_assert(type::IsArithmetic<T>);
		DEBUG_ASSERT(!(max < min));
		return value < min ? min : value < max ? value : max;
	}


	/** Clamps a value between min and max
	 * @param value Input value
	 * @param min Minimum value to be allowed
	 * @param max Maximum value to be allowed
	 * If value is outside given range, it will be set to one of the boundaries */
	template <class T, class Tx>
	NODISCARD inline constexpr T Clamp(const T& value, const Tx& min, const Tx& max)
	{
		static_assert(type::IsArithmetic<T>);
		static_assert(type::IsArithmetic<Tx>);
		DEBUG_ASSERT(!(max < min));
		return value < min ? static_cast<T>(min) : value < max ? value : static_cast<T>(max);
	}


	// True if value would be clamped
	template <class Tr = bool, class T, class Tx>
	NODISCARD inline constexpr Tr ClampEval(const T& value, const Tx& min, const Tx& max)
	{
		static_assert(type::IsArithmetic<T>);
		static_assert(type::IsArithmetic<Tx>);
		DEBUG_ASSERT(!(max < min));
		if constexpr (type::IsIntegral<T> && type::IsIntegral<Tx>)
			return (((value - static_cast<T>(min)) | (static_cast<T>(max) - value)) >= 0);
		else
			return value < min ? false : value <= max ? true : false;
	}


	//-----------------------------------------------------------------------------

	template <ERound rounding = ERound::Floor, class Tx, class Ty>
	NODISCARD inline constexpr auto IntegralDivide(const Tx& dividend, const Ty& divisor)
	{
		static_assert(type::IsIntegral<Tx> && type::IsIntegral<Ty>);
		if constexpr (rounding == ERound::Ceil)
			return (dividend + divisor - 1) / divisor;
		if constexpr (rounding == ERound::Floor || rounding == ERound::None)
			return dividend / divisor;
		if constexpr (rounding == ERound::Nearest)
			return (dividend >= 0) ? (dividend + divisor / 2) / divisor : (dividend - divisor / 2 + 1) / divisor;
	}


	template <class T, class Ta>
	NODISCARD inline constexpr T Align(const T& value, const Ta& alignment)
	{
		static_assert(type::IsArithmetic<T>);
		static_assert(type::IsArithmetic<Ta>);
		if (value == 0 || alignment == 0)
			return value;
		if constexpr (type::IsFloat<T> && type::IsFloat<Ta>)
			return Floor((value + 0.5f * alignment) / alignment) * alignment;
		else
		{
			float v = static_cast<float>(value);
			float a = static_cast<float>(alignment);
			return Floor<T>((v + 0.5f * a) / a) * a;
		}
	}


	template <class T, type::EnableWhenFloatingtPoint<T> = 0>
	NODISCARD inline constexpr auto Pow2(const T& value)
	{
		if constexpr (type::IsFloat<T>)
			return powf(2.0f, value);
		else
			return pow(2.0, value);
	}


	template <class Tr = tfloat, class T, type::EnableWhenIntegral<T> = 0>
	NODISCARD inline constexpr auto Pow2(const T& value)
	{
		uint64 i = Abs(value);
		if (i < 64)
		{
			i = 1ull << i;
			return static_cast<Tr>(value < 0 ? 1.0 / i : i);
		}
		return pow(static_cast<Tr>(2.0f), static_cast<Tr>(value));
	}


	// Return integral value aligned to a pow2 alignment value.
	template <class T, class Ta = uint32>
	NODISCARD inline constexpr T AlignPow2(const T& value, const Ta& alignment = (1 << 4))
	{
		static_assert(type::IsArithmetic<T>);
		static_assert(type::IsIntegral<Ta>);
		static_assert(type::IsUnsigned<Ta>);
		DEBUG_ASSERT(IsPow2(alignment));  // alignment must be a power of 2 number

		if constexpr (type::IsIntegral<T>)
			return (value + alignment - 1) & ~(alignment - 1);
		else
		{
			using Tx = type::ConditionalUIntSize<std::conditional_t<sizeof(T) < sizeof(Ta), Ta, T>>;
			Tx v     = static_cast<Tx>(value);
			return (value + alignment - 1) & ~(alignment - 1);
		}
	}


	// Returns nearest power of two value
	template <class T>
	NODISCARD inline constexpr T Pow2Nearest(const T& value)
	{
		static_assert(type::IsArithmetic<T>);
		DEBUG_ASSERT(value >= 1);
		if (IsPow2(value))
			return value;
		using Tx = type::ConditionalUIntSize<T>;
		Tx val   = static_cast<Tx>(value);
		Tx x     = val - 1;
		x |= x >> 1;
		x |= x >> 2;
		x |= x >> 4;
		if constexpr (sizeof(Tx) >= 2)
			x |= x >> 8;
		if constexpr (sizeof(Tx) >= 4)
			x |= x >> 16;
		if constexpr (sizeof(Tx) >= 8)
			x |= x >> 32;
		Tx y = ++x >> 1;
		T v  = static_cast<T>((x - val) > (val - y) ? y : x);
		return v;
	}


	// Returns nearest power of two value that is greater than or equal to value
	template <class T>
	NODISCARD inline constexpr T Pow2Ceil(const T& value)
	{
		static_assert(type::IsArithmetic<T>);
		DEBUG_ASSERT(value >= 1);
		if (IsPow2(value))
			return value;
		using Tx = type::ConditionalUIntSize<T>;
		Tx val   = static_cast<Tx>(value);
		Tx x     = val - 1;
		x |= x >> 1;
		x |= x >> 2;
		x |= x >> 4;
		if constexpr (sizeof(Tx) >= 2)
			x |= x >> 8;
		if constexpr (sizeof(Tx) >= 4)
			x |= x >> 16;
		if constexpr (sizeof(Tx) >= 8)
			x |= x >> 32;
		T v = static_cast<T>(++x);
		return v;
	}


	// Returns nearest power of two value that is less than or equal to value
	template <class T>
	NODISCARD inline constexpr T Pow2Floor(const T& value)
	{
		static_assert(type::IsArithmetic<T>);
		DEBUG_ASSERT(value >= 1);
		if (IsPow2(value))
			return value;
		using Tx = type::ConditionalUIntSize<T>;
		Tx val   = static_cast<Tx>(value);
		Tx x     = val - 1;
		x |= x >> 1;
		x |= x >> 2;
		x |= x >> 4;
		if constexpr (sizeof(Tx) >= 2)
			x |= x >> 8;
		if constexpr (sizeof(Tx) >= 4)
			x |= x >> 16;
		if constexpr (sizeof(Tx) >= 8)
			x |= x >> 32;
		T v = static_cast<T>(++x >> 1);
		return v;
	}

}  // namespace iplug::math


//-----------------------------------------------------------------------------


namespace iplug::math
{
	// Fast approximation Pow2
	template <class T>
	NODISCARD inline constexpr float Pow2f(const T& value)
	{
		static_assert(type::IsArithmetic<T>);
		DEBUG_ASSERT(value < 128.0f - math::constants::delta);  // value must be less than 128-0.0000150203704834
		DEBUG_ASSERT(value >= -126.0f);                         // value must be equal or higher than -126
		float v  = static_cast<float>(value);
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
	// NODISCARD float fsqrt(const float& value)
	//{
	//	static_assert(Platform::IsLittleEndian(), "Big Endians not implemented yet");

	//	// internal lookup table for fsqrt
	//	static uint32* _fsqrt_lut = [] {
	//		CACHE_ALIGN(16)
	//		static uint32 array[0x10000];
	//		union
	//		{
	//			float f;
	//			uint32 i;
	//		} u;

	//		for (int i = 0; i <= 0x7fff; ++i)
	//		{
	//			u.i               = (i << 8) | (0x7f << 23);
	//			u.f               = sqrtf(u.f);
	//			array[i + 0x8000] = (u.i & 0x7fffff);
	//			u.i               = (i << 8) | (0x80 << 23);
	//			u.f               = sqrtf(u.f);
	//			array[i]          = (u.i & 0x7fffff);
	//		}
	//		return array;
	//	}();

	//	uint32 fbits = *(uint32*) &value;
	//	if (fbits == 0)
	//		return 0.0f;

	//	*(uint32*) &value =
	//		_fsqrt_lut[(fbits >> 8) & 0xffff] | ((((fbits - 0x3f800000) >> 1) + 0x3f800000) & 0x7f800000);

	//	return value;
	//}
}  // namespace iplug::math
