#include <metal_stdlib>
using namespace metal;

namespace colormap {
namespace IDL {
namespace Hue_Sat_Value_2 {

float colormap_low(float x) {
	return -9.89123311722871E-01 * x + 2.54113856910082E+02;
}

float colormap_r1(float x) {
	float t = x - 44.52807774916808;
	return (-2.10743035084859E-02 * t - 1.14339819510944E+00) * t + 255.0;
}

float colormap_r2(float x) {
	float t = x - 173.2142990353825;
	return (2.10464655909683E-02 * t + 3.09770350177039E+00) * t + 82.7835558104;
}

float colormap_g1(float x) {
	float t = x - 87.18599073927922;
	return (2.18814766236433E-02 * t + 1.07683877405025E+00) * t + 167.876161014;
}

float colormap_g2(float x) {
	float t = x - 216.2347301863598;
	return (-1.75617661106684E-02 * t - 5.19390917463437E+00) * t + 255.0;
}

float colormap_b2(float x) {
	float t = x - 130.3078696041572;
	return (-1.97675474706200E-02 * t - 3.16561290370380E+00) * t + 255.0;
}

float colormap_red(float x) {
	if (x < 44.52807774916808) {
		return 255.0;
	} else if (x < 87.18599073927922) {
		return colormap_r1(x);
	} else if (x < 173.2142990353825) {
		return colormap_low(x);
	} else if (x < 216.2347301863598) {
		return colormap_r2(x);
	} else {
		return 255.0;
	}
}

float colormap_green(float x) {
	if (x < 87.18599073927922) {
		return colormap_low(x);
	} else if (x < 130.3078696041572) {
		return colormap_g1(x);
	} else if (x < 216.2347301863598) {
		return 255.0;
	} else {
		return colormap_g2(x);
	}
}

float colormap_blue(float x) {
	if (x < 44.52807774916808) {
		return (2.31958376441286E-02 * x - 1.01298265446011E+00) * x + 2.54114630079813E+02; // B1
	} else if (x < 130.3078696041572) {
		return 255.0;
	} else if (x < 173.2142990353825) {
		return colormap_b2(x);
	} else {
		return colormap_low(x);
	}
}

// B1 - 255 = 0
// => [x=-0.8571972230440585,x=44.52807774916808]

// R1 - low = 0
// => [x=-5.450356335481052,x=87.18599073927922]

// G1 - 255 = 0
// => [x=-5.148233003947013,x=130.3078696041572]

// B2 - low = 0
// => [x=-22.70273917535556,x=173.2142990353825]

// R2 - 255 = 0
// => [x=-16.99015635858727,x=216.2347301863598]

// low(87.18599073927922) = 167.876161014
// low(173.2142990353825) = 82.7835558104

float4 colormap(float x) {
	float t = x * 255.0;
	float r = clamp(colormap_red(t) / 255.0, 0.0, 1.0);
	float g = clamp(colormap_green(t) / 255.0, 0.0, 1.0);
	float b = clamp(colormap_blue(t) / 255.0, 0.0, 1.0);
	return float4(r, g, b, 1.0);
}

} // namespace Hue_Sat_Value_2
} // namespace IDL
} // namespace colormap
