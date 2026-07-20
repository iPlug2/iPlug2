#include <metal_stdlib>
using namespace metal;

namespace colormap {
namespace IDL {
namespace Ocean {

float colormap_red(float x) {
	if (x < 0.84121424085) {
		const float pi = 3.141592653589793238462643383279502884197169399;
		const float a = 92.39421034238549;
		const float b = 88.02925388837211;
		const float c = 0.5467741159150409;
		const float d = 0.03040219113949284;
		return a * sin(2.0 * pi / c * (x - d)) + b;
	} else {
		return 105.0;
	}
}

float colormap_green(float x) {
	if (x < 0.84121424085) {
		const float pi = 3.141592653589793238462643383279502884197169399;
		const float a = 92.44399971120093;
		const float b = 22.7616696017667;
		const float c = 0.3971750420482239;
		const float d = 0.1428144080827581;
		const float e = 203.7220396611977;
		const float f = 49.51517183258432;
		float v = (a * x + b) * sin(2.0 * pi / c * (x + d)) + (e * x + f);
		if (v > 255.0) {
			return 255.0 - (v - 255.0);
		} else {
			return v;
		}
	} else {
		return 246.0;
	}
}

float colormap_blue(float x) {
	if (x < 0.84121424085) {
		const float pi = 3.141592653589793238462643383279502884197169399;
		const float a = 251.0868719483008;
		const float b = 0.5472498585835275;
		const float c = 0.02985857858149428;
		const float d = 225.9495771701237;
		float v = a * sin(2.0 * pi / b * (x - c)) + d;
		if (v > 255.0) {
			return 255.0 - (v - 255.0);
		} else if (v < 0.0) {
			return -v;
		} else {
			return v;
		}
	} else {
		return 234.0;
	}
}

// R1 - 105 = 0
// => 0.8344881408181015

// B1 - 234 = 0
// => 0.847940340889657

float4 colormap(float x) {
    float r = clamp(colormap_red(x) / 255.0, 0.0, 1.0);
    float g = clamp(colormap_green(x) / 255.0, 0.0, 1.0);
    float b = clamp(colormap_blue(x) / 255.0, 0.0, 1.0);
    return float4(r, g, b, 1.0);
}

} // namespace Ocean
} // namespace IDL
} // namespace colormap
