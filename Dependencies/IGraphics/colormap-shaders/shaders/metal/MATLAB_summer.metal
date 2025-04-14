#include <metal_stdlib>
using namespace metal;

namespace colormap {
namespace MATLAB {
namespace summer {

float4 colormap(float x) {
    return float4(clamp(x, 0.0, 1.0), clamp(0.5 * x + 0.5, 0.0, 1.0), 0.4, 1.0);
}

} // namespace summer
} // namespace MATLAB
} // namespace colormap
