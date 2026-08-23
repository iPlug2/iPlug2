float colormap_red(float x) {
    if (x < 0.0) {
        return 0.0;
    } else if (x <= 1.0) {
        float xx = 270.9 * x + 0.7703;
        if (xx > 255.0) {
            return (510.0 - xx) / 266.0;
        } else {
            return xx / 255.0;
        }
    } else {
        return 239.0 / 255.0;
    }
}

float colormap_green(float x) {
    if (x < 0.0) {
        return 124.0 / 255.0;
    } else if (x <= 1.0) {
        float xx = 180.0 * sin(x * 3.97 + 9.46) + 131.0;
        if (xx < 0.0) {
            return abs(xx) / 255.0;
        } else if (xx > 255.0) {
            return (510.0 - xx) / 255.0;
        } else {
            return xx / 255.0;
        }
    } else {
        return 242.0 / 255.0;
    }
}

float colormap_blue(float x) {
    if (x < 0.0) {
        return 78.0 / 255.0;
    } else if (x <= 1.0e0) {
        return (95.0 * sin((x - 0.041) * 7.46) + 106.9) / 255.0;
    } else {
        return 179.0 / 255.0;
    }
}

vec4 colormap(float x) {
    return vec4(colormap_red(x), colormap_green(x), colormap_blue(x), 1.0);
}
