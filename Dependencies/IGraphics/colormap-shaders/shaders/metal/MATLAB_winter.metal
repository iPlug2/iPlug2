#include <metal_stdlib>
using namespace metal;

namespace colormap {
namespace MATLAB {
namespace winter {

float4 colormap(float x) {
    return float4(0.0, clamp(x, 0.0, 1.0), clamp(-0.5 * x + 1.0, 0.0, 1.0), 1.0);
}

} // namespace winter
} // namespace MATLAB
} // namespace colormap
