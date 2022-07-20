#include <metal_stdlib>
using namespace metal;

namespace colormap {
namespace MATLAB {
namespace autumn {

float4 colormap(float x) {
    float g = clamp(x, 0.0, 1.0);
    return float4(1.0, g, 0.0, 1.0);
}

} // namespace autumn
} // namespace MATLAB
} // namespace colormap
