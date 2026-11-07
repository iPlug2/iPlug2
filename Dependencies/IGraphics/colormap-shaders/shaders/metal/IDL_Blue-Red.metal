#include <metal_stdlib>
using namespace metal;

namespace colormap {
namespace IDL {
namespace Blue_Red {

float colormap_red(float x) {
    return 4.04377880184332E+00 * x - 5.17956989247312E+02;
}

float colormap_green(float x) {
    if (x < (5.14022177419355E+02 + 1.13519230769231E+01) / (4.20313644688645E+00 + 4.04233870967742E+00)) {
        return 4.20313644688645E+00 * x - 1.13519230769231E+01;
    } else {
        return -4.04233870967742E+00 * x + 5.14022177419355E+02;
    }
}

float colormap_blue(float x) {
    if (x < 1.34071303331385E+01 / (4.25125657510228E+00 - 1.0)) { // 4.12367649967
        return x;
    } else if (x < (255.0 + 1.34071303331385E+01) / 4.25125657510228E+00) { // 63.1359518278
        return 4.25125657510228E+00 * x - 1.34071303331385E+01;
    } else if (x < (1.04455240613432E+03 - 255.0) / 4.11010047593866E+00) { // 192.100512082
        return 255.0;
    } else {
        return -4.11010047593866E+00 * x + 1.04455240613432E+03;
    }
}

float4 colormap(float x) {
    float t = x * 255.0;
    float r = clamp(colormap_red(t) / 255.0, 0.0, 1.0);
    float g = clamp(colormap_green(t) / 255.0, 0.0, 1.0);
    float b = clamp(colormap_blue(t) / 255.0, 0.0, 1.0);
    return float4(r, g, b, 1.0);
}

} // namespace Blue_Red
} // namespace IDL
} // namespace colormap
