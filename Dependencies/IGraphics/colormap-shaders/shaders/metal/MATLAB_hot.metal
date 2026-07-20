#include <metal_stdlib>
using namespace metal;

namespace colormap {
namespace MATLAB {
namespace hot {

float4 colormap(float x) {
    float r = clamp(8.0 / 3.0 * x, 0.0, 1.0);
    float g = clamp(8.0 / 3.0 * x - 1.0, 0.0, 1.0);
    float b = clamp(4.0 * x - 3.0, 0.0, 1.0);
    return float4(r, g, b, 1.0);
}

} // namespace hot
} // namespace MATLAB
} // namespace colormap
