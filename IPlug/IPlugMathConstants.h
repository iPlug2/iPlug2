/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for more info.

 ==============================================================================
*/

#pragma once

namespace iplug::math::constants
{
	struct ultimate_question_of_life
	{
		template <class T> static inline constexpr T answer = 42;
	};

	template <class T> inline constexpr T delta_v       = _Invalid<T> {};
	template <class T> inline constexpr T macheps16_v   = _Invalid<T> {};
	template <class T> inline constexpr T macheps32_v   = _Invalid<T> {};
	template <class T> inline constexpr T macheps64_v   = _Invalid<T> {};
	template <class T> inline constexpr T e_v           = _Invalid<T> {};
	template <class T> inline constexpr T log2e_v       = _Invalid<T> {};
	template <class T> inline constexpr T log10e_v      = _Invalid<T> {};
	template <class T> inline constexpr T log10_2_v     = _Invalid<T> {};
	template <class T> inline constexpr T pi_v          = _Invalid<T> {};
	template <class T> inline constexpr T pi2_v         = _Invalid<T> {};
	template <class T> inline constexpr T pi4_v         = _Invalid<T> {};
	template <class T> inline constexpr T inv_pi_v      = _Invalid<T> {};
	template <class T> inline constexpr T inv_pi2_v     = _Invalid<T> {};
	template <class T> inline constexpr T inv_sqrtpi_v  = _Invalid<T> {};
	template <class T> inline constexpr T inv_sqrtpi2_v = _Invalid<T> {};
	template <class T> inline constexpr T ln2_v         = _Invalid<T> {};
	template <class T> inline constexpr T ln10_v        = _Invalid<T> {};
	template <class T> inline constexpr T sqrt2_v       = _Invalid<T> {};
	template <class T> inline constexpr T sqrt3_v       = _Invalid<T> {};
	template <class T> inline constexpr T inv_sqrt2_v   = _Invalid<T> {};
	template <class T> inline constexpr T inv_sqrt3_v   = _Invalid<T> {};
	template <class T> inline constexpr T egamma_v      = _Invalid<T> {};
	template <class T> inline constexpr T phi_v         = _Invalid<T> {};
	template <class T> inline constexpr T G_v           = _Invalid<T> {};
	template <class T> inline constexpr T Np_v          = _Invalid<T> {};
	template <class T> inline constexpr T inv_Np_v      = _Invalid<T> {};
	template <class T> inline constexpr T rad_v         = _Invalid<T> {};
	template <class T> inline constexpr T inv_rad_v     = _Invalid<T> {};

	template <> inline constexpr double delta_v<double>       = 0.00001;
	template <> inline constexpr double macheps16_v<double>   = 9.77e-4;
	template <> inline constexpr double macheps32_v<double>   = 1.19e-7;
	template <> inline constexpr double macheps64_v<double>   = 2.22e-16;
	template <> inline constexpr double e_v<double>           = 2.718281828459045;
	template <> inline constexpr double log2e_v<double>       = 1.4426950408889634;
	template <> inline constexpr double log10e_v<double>      = 0.4342944819032518;
	template <> inline constexpr double log10_2_v<double>     = 0.3010299956639812;
	template <> inline constexpr double pi_v<double>          = 3.141592653589793;
	template <> inline constexpr double pi2_v<double>         = 1.570796326794897;
	template <> inline constexpr double pi4_v<double>         = 0.7853981633974483;
	template <> inline constexpr double inv_pi_v<double>      = 0.3183098861837907;
	template <> inline constexpr double inv_pi2_v<double>     = 0.6366197723675814;
	template <> inline constexpr double inv_sqrtpi_v<double>  = 0.5641895835477563;
	template <> inline constexpr double inv_sqrtpi2_v<double> = 1.128379167095513;
	template <> inline constexpr double ln2_v<double>         = 0.6931471805599453;
	template <> inline constexpr double ln10_v<double>        = 2.302585092994046;
	template <> inline constexpr double sqrt2_v<double>       = 1.4142135623730951;
	template <> inline constexpr double sqrt3_v<double>       = 1.7320508075688772;
	template <> inline constexpr double inv_sqrt2_v<double>   = 0.7071067811865475;
	template <> inline constexpr double inv_sqrt3_v<double>   = 0.5773502691896257;
	template <> inline constexpr double egamma_v<double>      = 0.5772156649015329;
	template <> inline constexpr double phi_v<double>         = 1.618033988749895;
	template <> inline constexpr double G_v<double>           = 0.000000000066742;
	template <> inline constexpr double Np_v<double>          = 8.685889638065036;
	template <> inline constexpr double inv_Np_v<double>      = 0.115129254649702;
	template <> inline constexpr double rad_v<double>         = 57.295779513082321;
	template <> inline constexpr double inv_rad_v<double>     = 0.0174532925199433;

	template <> inline constexpr float delta_v<float>       = static_cast<float>(delta_v<double>);
	template <> inline constexpr float macheps16_v<float>   = static_cast<float>(macheps16_v<double>);
	template <> inline constexpr float macheps32_v<float>   = static_cast<float>(macheps32_v<double>);
	template <> inline constexpr float macheps64_v<float>   = static_cast<float>(macheps64_v<double>);
	template <> inline constexpr float e_v<float>           = static_cast<float>(e_v<double>);
	template <> inline constexpr float log2e_v<float>       = static_cast<float>(log2e_v<double>);
	template <> inline constexpr float log10e_v<float>      = static_cast<float>(log10e_v<double>);
	template <> inline constexpr float log10_2_v<float>     = static_cast<float>(log10_2_v<double>);
	template <> inline constexpr float pi_v<float>          = static_cast<float>(pi_v<double>);
	template <> inline constexpr float pi2_v<float>         = static_cast<float>(pi2_v<double>);
	template <> inline constexpr float pi4_v<float>         = static_cast<float>(pi4_v<double>);
	template <> inline constexpr float inv_pi_v<float>      = static_cast<float>(inv_pi_v<double>);
	template <> inline constexpr float inv_pi2_v<float>     = static_cast<float>(inv_pi2_v<double>);
	template <> inline constexpr float inv_sqrtpi_v<float>  = static_cast<float>(inv_sqrtpi_v<double>);
	template <> inline constexpr float inv_sqrtpi2_v<float> = static_cast<float>(inv_sqrtpi2_v<double>);
	template <> inline constexpr float ln2_v<float>         = static_cast<float>(ln2_v<double>);
	template <> inline constexpr float ln10_v<float>        = static_cast<float>(ln10_v<double>);
	template <> inline constexpr float sqrt2_v<float>       = static_cast<float>(sqrt2_v<double>);
	template <> inline constexpr float sqrt3_v<float>       = static_cast<float>(sqrt3_v<double>);
	template <> inline constexpr float inv_sqrt2_v<float>   = static_cast<float>(inv_sqrt2_v<double>);
	template <> inline constexpr float inv_sqrt3_v<float>   = static_cast<float>(inv_sqrt3_v<double>);
	template <> inline constexpr float egamma_v<float>      = static_cast<float>(egamma_v<double>);
	template <> inline constexpr float phi_v<float>         = static_cast<float>(phi_v<double>);
	template <> inline constexpr float G_v<float>           = static_cast<float>(G_v<double>);
	template <> inline constexpr float Np_v<float>          = static_cast<float>(Np_v<double>);
	template <> inline constexpr float inv_Np_v<float>      = static_cast<float>(inv_Np_v<double>);
	template <> inline constexpr float rad_v<float>         = static_cast<float>(rad_v<double>);
	template <> inline constexpr float inv_rad_v<float>     = static_cast<float>(inv_rad_v<double>);

	inline constexpr tfloat delta       = delta_v<tfloat>;        // 0.00001 precision threshold
	inline constexpr tfloat macheps16   = macheps16_v<tfloat>;    // Machine epsilon 16bit 9.77e-4
	inline constexpr tfloat macheps32   = macheps32_v<tfloat>;    // Machine epsilon 32bit 1.19e-7
	inline constexpr tfloat macheps64   = macheps64_v<tfloat>;    // Machine epsilon 64bit 2.22e-16
	inline constexpr tfloat e           = e_v<tfloat>;            // Euler's number
	inline constexpr tfloat log2e       = log2e_v<tfloat>;        // log2(e)
	inline constexpr tfloat log10e      = log10e_v<tfloat>;       // log10(e)
	inline constexpr tfloat log10_2     = log10_2_v<tfloat>;      // log10(2)
	inline constexpr tfloat pi          = pi_v<tfloat>;           // pi
	inline constexpr tfloat pi2         = pi2_v<tfloat>;          // pi/2
	inline constexpr tfloat pi4         = pi4_v<tfloat>;          // pi/4
	inline constexpr tfloat inv_pi      = inv_pi_v<tfloat>;       // 1/pi
	inline constexpr tfloat inv_pi2     = inv_pi2_v<tfloat>;      // 2/pi
	inline constexpr tfloat inv_sqrtpi  = inv_sqrtpi_v<tfloat>;   // 1/sqrt(pi)
	inline constexpr tfloat inv_sqrtpi2 = inv_sqrtpi2_v<tfloat>;  // 2/sqrt(pi)
	inline constexpr tfloat ln2         = ln2_v<tfloat>;          // ln(2)
	inline constexpr tfloat ln10        = ln10_v<tfloat>;         // ln(10)
	inline constexpr tfloat sqrt2       = sqrt2_v<tfloat>;        // sqrt(2)
	inline constexpr tfloat sqrt3       = sqrt3_v<tfloat>;        // sqrt(3)
	inline constexpr tfloat inv_sqrt2   = inv_sqrt2_v<tfloat>;    // 1/sqrt(2)
	inline constexpr tfloat inv_sqrt3   = inv_sqrt3_v<tfloat>;    // 1/sqrt(3)
	inline constexpr tfloat egamma      = egamma_v<tfloat>;       // γ Euler–Mascheroni constant
	inline constexpr tfloat phi         = phi_v<tfloat>;          // φ=(1+sqrt(5))/2 (golden ratio)
	inline constexpr tfloat G           = G_v<tfloat>;            // Gravitational constant
	inline constexpr tfloat Np          = Np_v<tfloat>;           // Neper 20*log10(e)
	inline constexpr tfloat inv_Np      = inv_Np_v<tfloat>;       // 1/Np
	inline constexpr tfloat rad         = rad_v<tfloat>;          // 180/pi
	inline constexpr tfloat inv_rad     = inv_rad_v<tfloat>;      // 1/rad
}  // namespace iplug::math::constants
