/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for more info.

 ==============================================================================
*/

#pragma once


namespace iplug::math::constants
{
	// clang-format off

	// Declare floating point types
	#define FDECLARE_CONST(name, number)                                                              \
		template <class T>                                                                            \
		inline constexpr typename std::enable_if_t<std::is_floating_point_v<T>, T> name##_v = number; \
		inline constexpr tfloat name = name##_v<tfloat>;

	// Declare integral types
	#define IDECLARE_CONST(name, number)                                                              \
		template <class T>                                                                            \
		inline constexpr typename std::enable_if_t<std::is_integral_v<T>, T> name##_v = number;       \
		inline constexpr int name = name##_v<int>;

	FDECLARE_CONST( delta,        0.00001            )  // Δ precision threshold (=e^(1/20*ln(10)*-100dB))
	FDECLARE_CONST( macheps16,    9.77e-4            )  // Machine epsilon 16bit 9.77e-4
	FDECLARE_CONST( macheps32,    1.19e-7            )  // Machine epsilon 32bit 1.19e-7
	FDECLARE_CONST( macheps64,    2.22e-16           )  // Machine epsilon 64bit 2.22e-16
	FDECLARE_CONST( e,            2.718281828459045  )  // Euler's number
	FDECLARE_CONST( log2e,        1.4426950408889634 )  // log2(e)
	FDECLARE_CONST( log10e,       0.4342944819032518 )  // log10(e)
	FDECLARE_CONST( log10_2,      0.3010299956639812 )  // log10(2), 1/log2(10)
	FDECLARE_CONST( pi,           3.141592653589793  )  // pi
	FDECLARE_CONST( pi2,          1.570796326794897  )  // pi/2
	FDECLARE_CONST( pi4,          0.7853981633974483 )  // pi/4
	FDECLARE_CONST( inv_pi,       0.3183098861837907 )  // 1/pi
	FDECLARE_CONST( inv_pi2,      0.6366197723675814 )  // 2/pi
	FDECLARE_CONST( inv_sqrtpi,   0.5641895835477563 )  // 1/sqrt(pi)
	FDECLARE_CONST( inv_sqrtpi2,  1.128379167095513  )  // 2/sqrt(pi)
	FDECLARE_CONST( ln2,          0.6931471805599453 )  // ln(2)
	FDECLARE_CONST( ln10,         2.302585092994046  )  // ln(10)
	FDECLARE_CONST( sqrt2,        1.4142135623730951 )  // sqrt(2)
	FDECLARE_CONST( sqrt3,        1.7320508075688772 )  // sqrt(3)
	FDECLARE_CONST( inv_sqrt2,    0.7071067811865475 )  // 1/sqrt(2)
	FDECLARE_CONST( inv_sqrt3,    0.5773502691896257 )  // 1/sqrt(3)
	FDECLARE_CONST( egamma,       0.5772156649015329 )  // γ Euler–Mascheroni constant
	FDECLARE_CONST( phi,          1.618033988749895  )  // φ=(1+sqrt(5))/2 (golden ratio)
	FDECLARE_CONST( G,            0.000000000066742  )  // Gravitational constant
	FDECLARE_CONST( Np,           8.685889638065036  )  // 20*log10(e) dB
	FDECLARE_CONST( inv_Np,       0.115129254649702  )  // 1/20*ln(10) Np
	FDECLARE_CONST( rad,          57.295779513082321 )  // 180/pi deg
	FDECLARE_CONST( inv_rad,      0.0174532925199433 )  // pi/180 rad
	FDECLARE_CONST( C4,           261.6256           )  // C4 Hz
	FDECLARE_CONST( A4,           440.0              )  // A4 Hz
	FDECLARE_CONST( semitone,     1.0594630943592953 )  // Hz=2^(1/12) (Δhigh)
	FDECLARE_CONST( inv_semitone, 0.9438743126816935 )  // Hz=2^(-1/12) (Δlow)

	IDECLARE_CONST( ultimate_question_of_life, 42 )     // yes

	#undef FDECLARE_CONST
	#undef IDECLARE_CONST
	// clang-format on

}  // namespace iplug::math::constants
