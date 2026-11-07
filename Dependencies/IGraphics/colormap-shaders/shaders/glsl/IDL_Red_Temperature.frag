float colormap_red(float x) {
    return 1.448953446096850 * x - 5.02253539008443e-1;
}

float colormap_green(float x) {
    return 1.889376646180860 * x - 2.272028094820020e2;
}

float colormap_blue(float x) {
    return 3.92613636363636 * x - 7.46528409090909e+2;
}

vec4 colormap(float x) {
    float t = x * 255.0;
    float r = clamp(colormap_red(t) / 255.0, 0.0, 1.0);
    float g = clamp(colormap_green(t) / 255.0, 0.0, 1.0);
    float b = clamp(colormap_blue(t) / 255.0, 0.0, 1.0);
    return vec4(r, g, b, 1.0);
}
