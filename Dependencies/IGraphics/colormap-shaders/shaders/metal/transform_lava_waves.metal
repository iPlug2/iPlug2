#include <metal_stdlib>
using namespace metal;

namespace colormap {
namespace transform {
namespace lava_waves {

float colormap_red(float x) {
    if (x < 0.0) {
        return 124.0 / 255.0;
    } else if (x <= 1.0) {
        return (128.0 * sin(6.25 * (x + 0.5)) + 128.0) / 255.0;
    } else {
        return 134.0 / 255.0;
    }
}


float colormap_green(float x) {
    if (x < 0.0) {
        return 121.0 / 255.0;
    } else if (x <= 1.0) {
        return (63.0 * sin(x * 99.72) + 97.0) / 255.0;
    } else {
        return 52.0 / 255.0;
    }
}

float colormap_blue(float x) {
    if (x < 0.0) {
        return 131.0 / 255.0;
    } else if (x <= 1.0) {
        return (128.0 * sin(6.23 * x) + 128.0) / 255.0;
    } else {
        return 121.0 / 255.0;
    }
}

float4 colormap(float x) {
    return float4(colormap_red(x), colormap_green(x), colormap_blue(x), 1.0);
}

} // namespace lava_waves
} // namespace transform
} // namespace colormap
