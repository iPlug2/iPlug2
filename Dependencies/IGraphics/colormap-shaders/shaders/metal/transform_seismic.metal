#include <metal_stdlib>
using namespace metal;

namespace colormap {
namespace transform {
namespace seismic {

float colormap_f(float x) {
    return ((-2010.0 * x + 2502.5950459) * x - 481.763180924) / 255.0;
}

float colormap_red(float x) {
    if (x < 0.0) {
        return 3.0 / 255.0;
    } else if (x < 0.238) {
        return ((-1810.0 * x + 414.49) * x + 3.87702) / 255.0;
    } else if (x < 51611.0 / 108060.0) {
        return (344441250.0 / 323659.0 * x - 23422005.0 / 92474.0) / 255.0;
    } else if (x < 25851.0 / 34402.0) {
        return 1.0;
    } else if (x <= 1.0) {
        return (-688.04 * x + 772.02) / 255.0;
    } else {
        return 83.0 / 255.0;
    }
}

float colormap_green(float x) {
    if (x < 0.0) {
        return 0.0;
    } else if (x < 0.238) {
        return 0.0;
    } else if (x < 51611.0 / 108060.0) {
        return colormap_f(x);
    } else if (x < 0.739376978894039) {
        float xx = x - 51611.0 / 108060.0;
        return ((-914.74 * xx - 734.72) * xx + 255.) / 255.0;
    } else {
        return 0.0;
    }
}

float colormap_blue(float x) {
    if (x < 0.0) {
        return 19.0 / 255.0;
    } else if (x < 0.238) {
        float xx = x - 0.238;
        return (((1624.6 * xx + 1191.4) * xx + 1180.2) * xx + 255.0) / 255.0;
    } else if (x < 51611.0 / 108060.0) {
        return 1.0;
    } else if (x < 174.5 / 256.0) {
        return (-951.67322673866 * x + 709.532730938451) / 255.0;
    } else if (x < 0.745745353439206) {
        return (-705.250074130877 * x + 559.620050530617) / 255.0;
    } else if (x <= 1.0) {
        return ((-399.29 * x + 655.71) * x - 233.25) / 255.0;
    } else {
        return 23.0 / 255.0;
    }
}

float4 colormap(float x) {
    return float4(colormap_red(x), colormap_green(x), colormap_blue(x), 1.0);
}

} // namespace seismic
} // namespace transform
} // namespace colormap
