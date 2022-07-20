#include <metal_stdlib>
using namespace metal;

namespace colormap {
namespace MATLAB {
namespace spring {

float4 colormap(float x) {
    return float4(1.0, clamp(x, 0.0, 1.0), clamp(1.0 - x, 0.0, 1.0), 1.0);
}

} // namespace spring
} // namespace MATLAB
} // namespace colormap
