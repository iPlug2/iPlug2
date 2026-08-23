#include <metal_stdlib>
using namespace metal;

namespace colormap {
namespace IDL {
namespace Nature {

float colormap_red(float x) {
	if (x < 0.8) {
		const float pi = 3.141592653589793238462643383279502884197169399;
		float v = sin(2.0 * pi * (x * 2.440771851872335 + 0.03082889566781979)) * 94.0 + 86.68712190457872;
		if (v < 0.0) {
			return -v;
		} else {
			return v;
		}
	} else {
		return 82.0;
	}
}

float colormap_green(float x) {
	if (x < 0.8) {
		const float pi = 3.141592653589793238462643383279502884197169399;
		const float a = - 133.8180196373718;
		const float b = 105.5963045788319;
		const float c = 1.192621559448461;
		const float d = 4.00554233186818;
		const float e = 0.04779355732364274;
		const float f = 218.2356517672776;
		const float g = -269.6049419208264;
		float v = (a * x + b) * sin(2.0 * pi / c * (d * x + e)) + f + g * x;
		if (v > 255.0) {
			return 255.0 - (v - 255.0);
		} else {
			return v;
		}
	} else {
		return 0.0;
	}
}

float colormap_blue(float x) {
	if (x < 0.8) {
		const float pi = 3.141592653589793238462643383279502884197169399;
		const float a = 2.443749115965466;
		const float b = 0.02934035424870109;
		const float c = 253.745120022022;
		const float d = 226.671125688366;
		float v = sin(2.0 * pi * (x * a + b))*c + d;
		if (v > 255.0) {
			return 255.0 - (v - 255.0);
		} else if (v < 0.0) {
			return -v;
		} else {
			return v;
		}
	} else {
		return 214.0;
	}
}

float4 colormap(float x) {
    float r = clamp(colormap_red(x) / 255.0, 0.0, 1.0);
    float g = clamp(colormap_green(x) / 255.0, 0.0, 1.0);
    float b = clamp(colormap_blue(x) / 255.0, 0.0, 1.0);
    return float4(r, g, b, 1.0);
}

} // namespace Nature
} // namespace IDL
} // namespace colormap
