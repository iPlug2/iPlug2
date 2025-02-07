#include <metal_stdlib>
using namespace metal;

namespace colormap {
namespace kbinani {
namespace altitude {

float colormap_func(float x, float a, float c, float d, float e, float f, float g) {
	x = 193.0 * clamp(x, 0.0, 1.0);
    return a * exp(-x * x / (2.0 * c * c)) + ((d * x + e) * x + f) * x + g;
}

float4 colormap(float x) {
    float r = colormap_func(x, 48.6399, 13.3443, 0.00000732641, -0.00154886, -0.211758, 83.3109);
    float g = colormap_func(x, 92.7934, 9.66818, 0.00000334955, -0.000491041, -0.189276, 56.8844);
    float b = colormap_func(x, 43.4277, 8.92338, 0.00000387675, -0.00112176, 0.0373863, 15.9435);
    return float4(r / 255.0, g / 255.0, b / 255.0, 1.0);
}

} // namespace altitude
} // namespace kbinani
} // namespace colormap
