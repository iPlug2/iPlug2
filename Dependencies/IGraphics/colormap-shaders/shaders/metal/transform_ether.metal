#include <metal_stdlib>
using namespace metal;

namespace colormap {
namespace transform {
namespace ether {

float colormap_f(float x, float b) {
    float x2 = x * x;
    return ((-1.89814e5 * x2 + 1.50967e4) * x2 + b) / 255.0;
}

float colormap_f2(float x, float b) {
    float x2 = x * x;
    return ((1.88330e5 * x2 - 1.50839e4) * x2 + b) / 255.0;
}

float colormap_blue(float x) {
    if (x < 0.0e0) {
        return 246.0 / 255.0;
    } else if (x < 0.25) {
        return colormap_f(x - 32.0 / 256.0, 65.0);
    } else if (x < 130.0 / 256.0) {
        return colormap_f2(x - 97.0 / 256.0, 190.0);
    } else if (x < 193.0 / 256.0) {
        return colormap_f(x - 161.0 / 256.0, 65.0);
    } else if (x < 1.0) {
        return colormap_f2(x - 226.0 / 256.0, 190.0);
    } else {
        return 18.0 / 255.0;
    }
}

float colormap_green(float x) {
    if (x < 0.0) {
        return 0.0;
    } else if (x < 0.20615790927912) {
        return ((((-3.81619e4 * x - 2.94574e3) * x + 2.61347e3) * x - 7.92183e1) * x) / 255.0;
    } else if (x < 0.54757171958025) {
        return (((((2.65271e5 * x - 4.14808e5) * x + 2.26118e5) * x - 5.16491e4) * x + 5.06893e3) * x - 1.80630e2) / 255.0;
    } else if (x < 0.71235558668792) {
        return ((((1.77058e5 * x - 4.62571e5) * x + 4.39628e5) * x - 1.80143e5) * x + 2.68555e4) / 255.0;
    } else if (x < 1.0) {
        float xx = ((((1.70556e5 * x - 6.20429e5) * x + 8.28331e5) * x - 4.80913e5) * x + 1.02608e5);
        if (xx > 255.0) {
            return (510.0 - xx) / 255.0;
        } else {
            return xx / 255.0;
        }
    } else {
        return 154.0 / 255.0;
    }
}

float colormap_red(float x) {
    if (x < 0.0) {
        return 2.0 / 255.0;
    } else if (x < 1.0) {
        float xx = 2.83088e2 * x + 8.17847e-1;
        if (xx > 255.0) {
            return (510.0 - xx) / 255.0;
        } else {
            return xx / 255.0;
        }
    } else {
        return 226.0 / 255.0;
    }
}

float4 colormap(float x) {
    return float4(colormap_red(x), colormap_green(x), colormap_blue(x), 1.0);
}

} // namespace ether
} // namespace transform
} // namespace colormap
