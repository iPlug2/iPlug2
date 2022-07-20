#include <metal_stdlib>
using namespace metal;

namespace colormap {
namespace transform {
namespace supernova {

float colormap_f1(float x) {
    return (0.3647 * x + 164.02) * x + 154.21;
}

float colormap_f2(float x) {
    return (126.68 * x + 114.35) * x + 0.1551;
}

float colormap_red(float x) {
    if (x < 0.0) {
        return 0.0;
    } else if (x < 0.136721748106749) {
        return colormap_f2(x) / 255.0;
    } else if (x < 0.23422409711017) {
        return (1789.6 * x - 226.52) / 255.0;
    } else if (x < 0.498842730309711) {
        return colormap_f1(x) / 255.0;
    } else if (x < 0.549121259378134) {
        return (-654.951781800243 * x + 562.838873112072) / 255.0;
    } else if (x < 1.0) {
        return ((3.6897 * x + 11.125) * x + 223.15) / 255.0;
    } else {
        return 237.0 / 255.0;
    }
}

float colormap_green(float x) {
    if (x < 0.0) {
        return 154.0 / 255.0;
    } else if (x < 3.888853260731947e-2) {
        return colormap_f1(x) / 255.0;
    } else if (x < 0.136721748106749e0) {
        return (-1455.86353067466 * x + 217.205447330541) / 255.0;
    } else if (x < 0.330799131955394) {
        return colormap_f2(x) / 255.0;
    } else if (x < 0.498842730309711) {
        return (1096.6 * x - 310.91) / 255.0;
    } else if (x < 0.549121259378134) {
        return colormap_f1(x) / 255.0;
    } else {
        return 244.0 / 255.0;
    }
}

float colormap_blue(float x) {
    if (x < 0.0) {
        return 93.0 / 255.0;
    } else if (x < 3.888853260731947e-2) {
        return (1734.6 * x + 93.133) / 255.0;
    } else if (x < 0.234224097110170) {
        return colormap_f1(x) / 255.0;
    } else if (x < 0.330799131955394) {
        return (-1457.96598791534 * x + 534.138211325166) / 255.0;
    } else if (x < 0.549121259378134) {
        return colormap_f2(x) / 255.0;
    } else if (x < 1.0) {
        return ((3.8931 * x + 176.32) * x + 3.1505) / 255.0;
    } else {
        return 183.0 / 255.0;
    }
}

float4 colormap(float x) {
    return float4(colormap_red(x), colormap_green(x), colormap_blue(x), 1.0);
}

} // namespace supernova
} // namespace transform
} // namespace colormap
