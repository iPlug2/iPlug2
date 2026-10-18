#include <metal_stdlib>
using namespace metal;

namespace colormap {
namespace IDL {
namespace Waves {

float colormap_f(float x, float phase) {
	const float pi = 3.141592653589793238462643383279502884197169399;
	const float a = 126.9634465941118;
	const float b = 1.011727672706345;
	const float c = 0.0038512319231245;
	const float d = 127.5277540583575;
	return a * sin(2.0 * pi / b * x + 2.0 * pi * (c + phase)) + d;
}

float colormap_red(float x) {
	return colormap_f(x, 0.5);
}

float colormap_green(float x) {
	const float pi = 3.141592653589793238462643383279502884197169399;
	const float a = 63.19460736097507;
	const float b = 0.06323746667143024;
	const float c = 0.06208443629833329;
	const float d = 96.56305326777574;
	return a * sin(2.0 * pi / b * x + 2.0 * pi * c) + d;
}

float colormap_blue(float x) {
	return colormap_f(x, 0.0);
}

float4 colormap(float x) {
	float r = clamp(colormap_red(x) / 255.0, 0.0, 1.0);
	float g = clamp(colormap_green(x) / 255.0, 0.0, 1.0);
	float b = clamp(colormap_blue(x) / 255.0, 0.0, 1.0);
	return float4(r, g, b, 1.0);
}

} // namespace Waves
} // namespace IDL
} // namespace colormap
