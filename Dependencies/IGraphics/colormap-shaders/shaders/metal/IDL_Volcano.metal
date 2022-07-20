#include <metal_stdlib>
using namespace metal;

namespace colormap {
namespace IDL {
namespace Volcano {

float colormap_r1(float x) {
	const float pi = 3.141592653589793238462643383279502884197169399;
	const float a = 216.6901575438631;
	const float b = 1.073972444219095;
	const float c = 0.6275803332110022;
	const float d = 221.6241814852619;
	float v = a * sin(2.0 * pi / b * x + 2.0 * pi * c) + d;
	if (v < 0.0) {
		return -v;
	} else {
		return v;
	}
}

float colormap_r2(float x) {
	const float pi = 3.141592653589793238462643383279502884197169399;
	const float a = 202.3454274460618;
	const float b = 1.058678309228987;
	const float c = 0.4891299991060677;
	const float d = -72.38173481234448;
	return a * sin(2.0 * pi / b * x + 2.0 * pi * c) + d;
}

float colormap_red(float x) {
	if (x < 0.4264009656413063) {
		return colormap_r1(x);
	} else if (x < 0.6024851624202665) {
		const float a = (0.0 - 255.0) / (0.6024851624202665 - 0.4264009656413063);
		const float b = -0.6024851624202665 * a;
		return a * x + b;
	} else {
		return colormap_r2(x);
	}
}

float colormap_green(float x) {
	const float pi = 3.141592653589793238462643383279502884197169399;
	const float a = 126.3856859482602;
	const float b = 0.6744554815524477;
	const float c = 0.01070628027163306;
	const float d = 26.95058522613648;
	float v = a * sin(2.0 * pi / b * x + 2.0 * pi * c) + d;
	if (v < 0.0) {
		return -v;
	} else {
		return v;
	}
}

float colormap_blue(float x) {
	const float pi = 3.141592653589793238462643383279502884197169399;
	const float a = 126.9540413031656;
	const float b = 0.2891013907955124;
	const float c = 0.5136633102640619;
	const float d = 126.5159759632338;
	return a * sin(2.0 * pi / b * x + 2.0 * pi * c) + d;
}

// R1 - 255 = 0
// => 0.4264009656413063

// R2 = 0
// => 0.6024851624202665

float4 colormap(float x) {
	float r = clamp(colormap_red(x) / 255.0, 0.0, 1.0);
	float g = clamp(colormap_green(x) / 255.0, 0.0, 1.0);
	float b = clamp(colormap_blue(x) / 255.0, 0.0, 1.0);
	return float4(r, g, b, 1.0);
}

} // namespace Volcano
} // namespace IDL
} // namespace colormap
