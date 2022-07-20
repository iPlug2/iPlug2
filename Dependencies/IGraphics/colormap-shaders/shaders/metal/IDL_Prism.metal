#include <metal_stdlib>
using namespace metal;

namespace colormap {
namespace IDL {
namespace Prism {

float colormap_red(float x) {
    if (x < (5.35651507139080E+02 + 4.75703324808184E-01) / (3.80568385693018E+00 + 4.18112109994712E+00)) {
        return 3.80568385693018E+00 * x - 4.75703324808184E-01;
    } else {
        return -4.18112109994712E+00 * x + 5.35651507139080E+02;
    }
}

float colormap_green(float x) {
    if (x < (7.72815970386039E+02 + 2.57000000000000E+02) / (4.00000000000000E+00 + 4.04283447911158E+00)) {
        return 4.00000000000000E+00 * x - 2.57000000000000E+02;
    } else {
        return -4.04283447911158E+00 * x + 7.72815970386039E+02;
    }
}

float colormap_blue(float x) {
    if (x < (1.03175883256528E+03 + 4.87540173259576E+02) / (3.86517065024528E+00 + 4.04377880184332E+00)) {
        return 3.86517065024528E+00 * x - 4.87540173259576E+02;
    } else {
        return -4.04377880184332E+00 * x + 1.03175883256528E+03;
    }
}

float4 colormap(float x) {
    float t = x * 255.0;
    float r = clamp(colormap_red(t) / 255.0, 0.0, 1.0);
    float g = clamp(colormap_green(t) / 255.0, 0.0, 1.0);
    float b = clamp(colormap_blue(t) / 255.0, 0.0, 1.0);
    return float4(r, g, b, 1.0);
}

} // namespace Prism
} // namespace IDL
} // namespace colormap
