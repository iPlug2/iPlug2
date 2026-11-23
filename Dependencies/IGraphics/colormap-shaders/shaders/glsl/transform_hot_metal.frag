float colormap_blue(float x) {
    return 0.0;
}

float colormap_green(float x) {
    if (x < 0.6) {
        return 0.0;
    } else if (x <= 0.95) {
        return ((x - 0.6) * 728.57) / 255.0;
    } else {
        return 1.0;
    }
}

float colormap_red(float x) {
    if (x < 0.0) {
        return 0.0;
    } else if (x <= 0.57147) {
        return 446.22 * x / 255.0;
    } else {
       return 1.0;
    }
}

vec4 colormap(float x) {
    return vec4(colormap_red(x), colormap_green(x), colormap_blue(x), 1.0);
}
