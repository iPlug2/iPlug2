float colormap_red(float x) {
    if (x < 1.0 / 3.0) {
        return 4.0 * x - 2.992156863;
    } else if (x < 2.0 / 3.0) {
        return 4.0 * x - 2.9882352941;
    } else if (x < 2.9843137255 / 3.0) {
        return 4.0 * x - 2.9843137255;
    } else {
        return x;
    }
}

float colormap_green(float x) {
    return 1.602642681354730 * x - 5.948580022657070e-1;
}

float colormap_blue(float x) {
    return 1.356416928785610 * x + 3.345982835050930e-3;
}

vec4 colormap(float x) {
    float r = clamp(colormap_red(x), 0.0, 1.0);
    float g = clamp(colormap_green(x), 0.0, 1.0);
    float b = clamp(colormap_blue(x), 0.0, 1.0);
    return vec4(r, g, b, 1.0);
}
