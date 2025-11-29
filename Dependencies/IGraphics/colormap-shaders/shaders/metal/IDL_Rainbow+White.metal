#include <metal_stdlib>
using namespace metal;

namespace colormap {
namespace IDL {
namespace Rainbow_White {

float4 colormap_hsv2rgb(float h, float s, float v) {
	float r = v;
	float g = v;
	float b = v;
	if (s > 0.0) {
		h *= 6.0;
		int i = int(h);
		float f = h - float(i);
		if (i == 1) {
			r *= 1.0 - s * f;
			b *= 1.0 - s;
		} else if (i == 2) {
			r *= 1.0 - s;
			b *= 1.0 - s * (1.0 - f);
		} else if (i == 3) {
			r *= 1.0 - s;
			g *= 1.0 - s * f;
		} else if (i == 4) {
			r *= 1.0 - s * (1.0 - f);
			g *= 1.0 - s;
		} else if (i == 5) {
			g *= 1.0 - s;
			b *= 1.0 - s * f;
		} else {
			g *= 1.0 - s * (1.0 - f);
			b *= 1.0 - s;
		}
	}
	return float4(r, g, b, 1.0);
}

float4 colormap(float x) {
	if (x < 0.0) {
		return float4(0.0, 0.0, 0.0, 1.0);
	} else if (1.0 < x) {
		return float4(1.0, 1.0, 1.0, 1.0);
	} else {
		float h = clamp(-9.42274071356572E-01 * x + 8.74326827903982E-01, 0.0, 1.0);
		float s = 1.0;
		float v = clamp(4.90125513855204E+00 * x + 9.18879034690780E-03, 0.0, 1.0);
		return colormap_hsv2rgb(h, s, v);
	}
}

} // namespace Rainbow_White
} // namespace IDL
} // namespace colormap
