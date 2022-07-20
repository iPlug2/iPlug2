#include <metal_stdlib>
using namespace metal;

namespace colormap {
namespace IDL {
namespace Green_Red_Blue_White {

float colormap_red(float x) {
    if (x < 0.2648221343873518) {
        return 1518.00 * x - 162.00;
    } else if (x < 0.2806324110671937) {
        return 759.00 * x + 39.00;
    } else if (x < 0.2964426877470356) {
        return 252.0;
    } else if (x < 0.3122529644268774) {
        return -253.00 * x + 327.00;
    } else if (x < 0.3280632411067194) {
        return 248.0;
    } else if (x < 0.3596837944664031) {
        return -253.00 * x + 331.00;
    } else if (x < 0.3636363636363636) {
        return 240.0;
    } else if (x < 0.3794466403162055) {
        return -253.00 * x + 332.00;
    } else if (x < 0.391304347826087) {
        return 236.0;
    } else if (x < 0.4229249011857708) {
        return -253.00 * x + 335.00;
    } else if (x < 0.4387351778656127) {
        return 228.0;
    } else if (x < 0.4861660079051384) {
        return -253.00 * x + 339.00;
    } else if (x < 0.5019762845849802) {
        return 216.0;
    } else if (x < 0.549407114624506) {
        return -253.00 * x + 343.00;
    } else if (x < 0.5652173913043478) {
        return 204.0;
    } else if (x < 0.5968379446640316) {
        return -253.00 * x + 347.00;
    } else if (x < 0.6126482213438735) {
        return 196.0;
    } else if (x < 0.6600790513833992) {
        return -253.00 * x + 351.00;
    } else if (x < 0.6758893280632411) {
        return 184.0;
    } else if (x < 0.7075098814229249) {
        return -253.00 * x + 355.00;
    } else if (x < 0.7233201581027668) {
        return 176.0;
    } else if (x < 0.7707509881422925) {
        return -253.00 * x + 359.00;
    } else if (x < 0.7865612648221344) {
        return 164.0;
    } else if (x < 0.83399209486166) {
        return -253.00 * x + 363.00;
    } else if (x < 0.849802371541502) {
        return 152.0;
    } else if (x < 0.8662737248407505) {
        return -253.00 * x + 367.00;
    } else {
        return 8.24946218487293E+02 * x - 5.66796485866989E+02;
    }
}

float colormap_green(float x) {
    if (x < 0.04321209459549381) {
        return 9.10799999999998E+02 * x + 6.80363636363637E+01;
    } else if (x < 0.1067193675889328) {
        return 2277.00 * x + 9.00;
    } else if (x < 0.1225296442687747) {
        return -759.00 * x + 333.00;
    } else if (x < 0.6113554850777934) {
        return -1518.00 * x + 426.00;
    } else if (x < 0.9924501603814814) {
        return 1.97884558823513E+03 * x - 1.71181573083763E+03;
    } else {
        return 253.00 * x + 1.00;
    }
}

float colormap_blue(float x) {
    return 5.23682489688790E+02 * x - 1.55016347956506E+02;
}

float4 colormap(float x) {
    float r = clamp(colormap_red(x) / 255.0, 0.0, 1.0);
    float g = clamp(colormap_green(x) / 255.0, 0.0, 1.0);
    float b = clamp(colormap_blue(x) / 255.0, 0.0, 1.0);
    return float4(r, g, b, 1.0);
}

} // namespace Green_Red_Blue_White
} // namespace IDL
} // namespace colormap
