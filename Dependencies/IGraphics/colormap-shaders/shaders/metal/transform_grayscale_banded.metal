#include <metal_stdlib>
using namespace metal;

namespace colormap {
namespace transform {
namespace grayscale_banded {

float4 colormap(float x) {
    float v = cos(133.0 * x) * 28.0 + 230.0 * x + 27.0;
    if (v > 255.0) {
        v = 510.0 - v;
    }
    v = v / 255.0;
    return float4(v, v, v, 1.0);
}

} // namespace grayscale_banded
} // namespace transform
} // namespace colormap
