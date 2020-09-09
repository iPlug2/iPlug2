/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for more info.

 ==============================================================================
*/

#pragma once


namespace iplug::math
{
	struct ultimate_question_of_life
	{
		template <class T> static inline constexpr T answer = 42;
	};

	template <class T> inline constexpr T e_v           = _Invalid<T> {};
	template <class T> inline constexpr T log2e_v       = _Invalid<T> {};
	template <class T> inline constexpr T log10e_v      = _Invalid<T> {};
	template <class T> inline constexpr T log10_2_v     = _Invalid<T> {};
	template <class T> inline constexpr T pi_v          = _Invalid<T> {};
	template <class T> inline constexpr T pi_t2_v       = _Invalid<T> {};
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
	template <class T> inline constexpr T ampdbe_v      = _Invalid<T> {};
	template <class T> inline constexpr T inv_ampdbe_v  = _Invalid<T> {};
	template <class T> inline constexpr T rad_v         = _Invalid<T> {};
	template <class T> inline constexpr T inv_rad_v     = _Invalid<T> {};

	// VC++ intellisense C++17 bug showing 'explicit specialization must precede first use'
	// Code compiles just fine and tested on compiler explorer.

	template <> inline constexpr double e_v<double>           = 2.718281828459045;
	template <> inline constexpr double log2e_v<double>       = 1.4426950408889634;
	template <> inline constexpr double log10e_v<double>      = 0.4342944819032518;
	template <> inline constexpr double log10_2_v<double>     = 0.3010299956639812;
	template <> inline constexpr double pi_v<double>          = 3.141592653589793;
	template <> inline constexpr double pi_t2_v<double>       = 6.283185307179585;
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
	template <> inline constexpr double ampdbe_v<double>      = 8.685889638065036;
	template <> inline constexpr double inv_ampdbe_v<double>  = 0.115129254649702;
	template <> inline constexpr double rad_v<double>         = 57.295779513082321;
	template <> inline constexpr double inv_rad_v<double>     = 0.0174532925199433;

	template <> inline constexpr float e_v<float>           = static_cast<float>(e_v<double>);
	template <> inline constexpr float log2e_v<float>       = static_cast<float>(log2e_v<double>);
	template <> inline constexpr float log10e_v<float>      = static_cast<float>(log10e_v<double>);
	template <> inline constexpr float log10_2_v<float>     = static_cast<float>(log10_2_v<double>);
	template <> inline constexpr float pi_v<float>          = static_cast<float>(pi_v<double>);
	template <> inline constexpr float pi_t2_v<float>       = static_cast<float>(pi_t2_v<double>);
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
	template <> inline constexpr float ampdbe_v<float>      = static_cast<float>(ampdbe_v<double>);
	template <> inline constexpr float inv_ampdbe_v<float>  = static_cast<float>(inv_ampdbe_v<double>);
	template <> inline constexpr float rad_v<float>         = static_cast<float>(rad_v<double>);
	template <> inline constexpr float inv_rad_v<float>     = static_cast<float>(inv_rad_v<double>);

	inline constexpr tfloat e           = e_v<tfloat>;            // e
	inline constexpr tfloat log2e       = log2e_v<tfloat>;        // log2(e)
	inline constexpr tfloat log10e      = log10e_v<tfloat>;       // log10(e)
	inline constexpr tfloat log10_2     = log10_2_v<tfloat>;      // log10(2)
	inline constexpr tfloat pi          = pi_v<tfloat>;           // pi
	inline constexpr tfloat pi_t2       = pi_t2_v<tfloat>;        // pi*2
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
	inline constexpr tfloat egamma      = egamma_v<tfloat>;       // eulers gamma
	inline constexpr tfloat phi         = phi_v<tfloat>;          // Φ (golden ratio)
	inline constexpr tfloat G           = G_v<tfloat>;            // gravitational constant
	inline constexpr tfloat ampdbe      = ampdbe_v<tfloat>;       // 20*log10(e)
	inline constexpr tfloat inv_ampdbe  = inv_ampdbe_v<tfloat>;   // 1/(20*log10(e))
	inline constexpr tfloat rad         = rad_v<tfloat>;          // 180/pi
	inline constexpr tfloat inv_rad     = inv_rad_v<tfloat>;      // pi/180

	// Degrees to radians
	template <class T = tfloat> inline constexpr auto DegToRad(T Degrees)
	{
		return Degrees * inv_rad_v<T>;
	}

	// Radians to degrees
	template <class T = tfloat> inline constexpr auto RadToDeg(T Radians)
	{
		return Radians * rad_v<T>;
	}

	/** @brief Calculates gain from a given dB value
		 * @param dB Value in dB
		 * @return Gain calculated as an approximation of
		 * \f$ 10^{\frac{x}{20}} \f$
		 * @see #IAMP_DB
		 */
	template <class T = tfloat> inline constexpr auto DBToAmp(T dB)
	{
		// return std::exp(amp_db<T> * dB);
		if constexpr (std::is_same_v<T, float>)
			return std::expf(inv_ampdbe_v<T> * dB);
		else
			return std::exp(inv_ampdbe_v<T> * dB);
	}

	/** @return dB calculated as an approximation of
		 * \f$ 20*log_{10}(x) \f$
		 * @see #AMP_DB */
	template <class T = tfloat> inline constexpr auto AmpToDB(T Amplitude)
	{
		// return inv_amp_db<T> * std::log(Amplitude);
		if constexpr (std::is_same_v<T, float>)
			return std::log10f(Amplitude) * 20.0f;
		else
			return std::log10(Amplitude) * 20.0f;
	}

	// AmpToDB using fast approximation
	inline const auto AmpToDBf(const float Amplitude) noexcept
	{
		int E;
		float F = std::frexpf(std::fabsf(Amplitude), &E);
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


	inline const float fsqrt(float value)
	{
		static const auto fsqrt_lut = [] {
			PLATFORM_CACHE_ALIGN
			static uint32 array[0x10000];
			union
			{
				float f;
				uint32 i;
			} s;
			for (int i = 0; i <= 0x7fff; ++i)
			{
				s.i               = (i << 8) | (0x7f << 23);
				s.f               = static_cast<float>(std::sqrt(s.f));
				array[i + 0x8000] = (s.i & 0x7fffff);
				s.i               = (i << 8) | (0x80 << 23);
				s.f               = static_cast<float>(std::sqrt(s.f));
				array[i]          = (s.i & 0x7fffff);
			}
			return array;
		}();

		uint32 fbits = *(uint32*) &value;
		if (fbits == 0)
			return 0.0;

		*(uint32*) &value = fsqrt_lut[(fbits >> 8) & 0xffff] | ((fbits - 0x3f800000) >> 1) + 0x3f800000 & 0x7f800000;
		return value;
	}

}  // namespace iplug::math
