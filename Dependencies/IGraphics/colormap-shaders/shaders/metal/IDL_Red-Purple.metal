#include <metal_stdlib>
using namespace metal;

namespace colormap {
namespace IDL {
namespace Red_Purple {

float colormap_red(float x) {
    if (x < 25.97288868211422) {
        return 3.07931034482759E+00 * x - 1.62758620689655E+00;
    } else if(x < 154.7883608200706) {
        return (-0.002335409922053 * x + 1.770196213987500) * x + 33.949335775363600;
    } else {
        return 252.0;
    }
}

float colormap_green(float x) {
    return ((7.125813968310300E-05 * x - 2.223039020276470E-02) * x + 2.367815929630070E+00) * x - 7.739188304766140E+01;
}

float colormap_blue(float x) {
    if (x < (2.51577880184332E+01 - 5.67741935483871E-01) / (9.88497695852535E-01 - 1.70189098998888E-01)) { // 30.0498444933
        return 1.70189098998888E-01 * x - 5.67741935483871E-01;
    } else if(x < 150.2124460352976) {
        return 9.88497695852535E-01 * x - 2.51577880184332E+01;
    } else {
        return (-3.85393764961783E-03 * x + 2.82261880442729E+00) * x - 2.13706208872841E+02;
    }
}

float4 colormap(float x) {
    float t = x * 255.0;
    float r = clamp(colormap_red(t) / 255.0, 0.0, 1.0);
    float g = clamp(colormap_green(t) / 255.0, 0.0, 1.0);
    float b = clamp(colormap_blue(t) / 255.0, 0.0, 1.0);
    return float4(r, g, b, 1.0);
}

} // namespace Red_Purple
} // namespace IDL
} // namespace colormap
