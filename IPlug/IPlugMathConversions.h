/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for more info.

 ==============================================================================
*/

#pragma once

namespace iplug::math
{
	//-----------------------------------------------------------------------------

	// Degrees to radians
	template <class T>
	NODISCARD inline constexpr auto DegToRad(const T Degrees)
	{
		static_assert(type::IsArithmetic<T>);
		return Degrees * constants::inv_rad_v<T>;
	}

	// Radians to degrees
	template <class T>
	NODISCARD inline constexpr auto RadToDeg(const T Radians)
	{
		static_assert(type::IsArithmetic<T>);
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
