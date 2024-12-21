float colormap_red(float x) {
    return 80.0 / 63.0 * x + 5.0 / 252.0;
}

float colormap_green(float x) {
    return 0.7936 * x - 0.0124;
}

float colormap_blue(float x) {
    return 796.0 / 1575.0 * x + 199.0 / 25200.0;
}

vec4 colormap(float x) {
    float r = clamp(colormap_red(x), 0.0, 1.0);
    float g = clamp(colormap_green(x), 0.0, 1.0);
    float b = clamp(colormap_blue(x), 0.0, 1.0);
    return vec4(r, g, b, 1.0);
}
