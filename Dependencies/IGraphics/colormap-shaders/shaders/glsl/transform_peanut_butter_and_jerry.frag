float colormap_red(float x) {
    if (x < 0.0) {
        return 1.0 / 255.0;
    } else if (x <= 1.0) {
        float xx = 407.92 * x + 1.3181;
        if (xx > 255.0) {
            return (510.0 - xx) / 255.0;
        } else {
            return xx / 255.0;
        }
    } else {
        return 100.0 / 255.0;
    }
}

float colormap_green(float x) {
    if (x < 0.0) {
        return 0.0;
    } else if (x <= 1.0) {
        return (128.7 * x + 0.2089) / 255.0;
    } else {
        return 128.0 / 255.0;
    }
}

float colormap_blue(float x) {
    if (x < 0.0) {
        return 73.0 / 255.0;
    } else if (x <= 1.0) {
        return (63.0 * sin(x * 6.21 - 0.3) + 92.0) / 255.0;
    } else {
        return 69.0 / 255.0;
    }
}

vec4 colormap(float x) {
    return vec4(colormap_red(x), colormap_green(x), colormap_blue(x), 1.0);
}
