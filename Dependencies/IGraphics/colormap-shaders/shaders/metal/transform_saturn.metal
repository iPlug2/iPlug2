#include <metal_stdlib>
using namespace metal;

namespace colormap {
namespace transform {
namespace saturn {

float colormap_f1(float x) {
    return -510.0 * x + 255.0;
}

float colormap_f2(float x) {
    return (-1891.7 * x + 217.46) * x + 255.0;
}

float colormap_f3(float x) {
    return 9.26643676359015e1 * sin((x - 4.83450094847127e-1) * 9.93) + 1.35940451627965e2;
}

float colormap_f4(float x) {
    return -510.0 * x + 510.0;
}

float colormap_f5(float x) {
    float xx = x - 197169.0 / 251000.0;
    return (2510.0 * xx - 538.31) * xx;
}

float colormap_red(float x) {
    if (x < 0.0) {
        return 1.0;
    } else if (x < 10873.0 / 94585.0) {
        float xx = colormap_f2(x);
        if (xx > 255.0) {
            return (510.0 - xx) / 255.0;
        } else {
            return xx / 255.0;
        }
    } else if (x < 0.5) {
        return 1.0;
    } else if (x < 146169.0 / 251000.0) {
        return colormap_f4(x) / 255.0;
    } else if (x < 197169.0 / 251000.0) {
        return colormap_f5(x) / 255.0;
    } else {
        return 0.0;
    }
}

float colormap_green(float x) {
    if (x < 10873.0 / 94585.0) {
        return 1.0;
    } else if (x < 36373.0 / 94585.0) {
        return colormap_f2(x) / 255.0;
    } else if (x < 0.5) {
        return colormap_f1(x) / 255.0;
    } else if (x < 197169.0 / 251000.0) {
        return 0.0;
    } else if (x <= 1.0) {
        return abs(colormap_f5(x)) / 255.0;
    } else {
        return 0.0;
    }
}

float colormap_blue(float x) {
    if (x < 0.0) {
        return 0.0;
    } else if (x < 36373.0 / 94585.0) {
        return colormap_f1(x) / 255.0;
    } else if (x < 146169.0 / 251000.0) {
        return colormap_f3(x) / 255.0;
    } else if (x <= 1.0) {
        return colormap_f4(x) / 255.0;
    } else {
        return 0.0;
    }
}

float4 colormap(float x) {
    return float4(colormap_red(x), colormap_green(x), colormap_blue(x), 1.0);
}

} // namespace saturn
} // namespace transform
} // namespace colormap
