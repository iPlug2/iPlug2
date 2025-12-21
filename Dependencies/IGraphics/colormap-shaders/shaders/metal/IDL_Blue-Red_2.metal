#include <metal_stdlib>
using namespace metal;

namespace colormap {
namespace IDL {
namespace Blue_Red_2 {

float colormap_red(float x) {
	if (x < 0.75) {
		return 1012.0 * x - 389.0;
	} else {
		return -1.11322769567548E+03 * x + 1.24461193212872E+03;
	}
}

float colormap_green(float x) {
	if (x < 0.5) {
		return 1012.0 * x - 129.0;
	} else {
		return -1012.0 * x + 899.0;
	}
}

float colormap_blue(float x) {
	if (x < 0.25) {
		return 1012.0 * x + 131.0;
	} else {
		return -1012.0 * x + 643.0;
	}
}

float4 colormap(float x) {
	float r = clamp(colormap_red(x) / 255.0, 0.0, 1.0);
	float g = clamp(colormap_green(x) / 255.0, 0.0, 1.0);
	float b = clamp(colormap_blue(x) / 255.0, 0.0, 1.0);
	return float4(r, g, b, 1.0);
}

} // namespace Blue_Red_2
} // namespace IDL
} // namespace colormap
