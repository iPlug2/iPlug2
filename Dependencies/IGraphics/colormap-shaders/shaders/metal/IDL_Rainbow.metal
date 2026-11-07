#include <metal_stdlib>
using namespace metal;

namespace colormap {
namespace IDL {
namespace Rainbow {

float colormap_red(float x) {
    if (x < 100.0) {
        return (-9.55123422981038E-02 * x + 5.86981763554179E+00) * x - 3.13964093701986E+00;
    } else {
        return 5.25591836734694E+00 * x - 8.32322857142857E+02;
    }
}

float colormap_green(float x) {
    if (x < 150.0) {
        return 5.24448979591837E+00 * x - 3.20842448979592E+02;
    } else {
        return -5.25673469387755E+00 * x + 1.34195877551020E+03;
    }
}

float colormap_blue(float x) {
    if (x < 80.0) {
        return 4.59774436090226E+00 * x - 2.26315789473684E+00;
    } else {
        return -5.25112244897959E+00 * x + 8.30385102040816E+02;
    }
}

float4 colormap(float x) {
    float t = x * 255.0;
    float r = clamp(colormap_red(t) / 255.0, 0.0, 1.0);
    float g = clamp(colormap_green(t) / 255.0, 0.0, 1.0);
    float b = clamp(colormap_blue(t) / 255.0, 0.0, 1.0);
    return float4(r, g, b, 1.0);
}

} // namespace Rainbow
} // namespace IDL
} // namespace colormap
