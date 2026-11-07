float colormap_f(float x) {
    return ((-9.93427e0 * x + 1.56301e1) * x + 2.44663e2 * x) / 255.0;
}

float colormap_blue(float x) {
    if (x < 0.0) {
        return 11.0 / 255.0;
    } else if (x < 0.16531216481302) {
        return (((-1635.0 * x) + 1789.0) * x + 3.938) / 255.0;
    } else if (x < 0.50663669203696) {
        return 1.0;
    } else if (x < 0.67502056695956) {
        return ((((1.28932e3 * x) - 7.74147e2) * x - 9.47634e2) * x + 7.65071e2) / 255.0;
    } else if (x < 1.0) {
        return colormap_f(x);
    } else {
        return 251.0 / 255.0;
    }
}

float colormap_green(float x) {
    if (x < 0.0) {
        return 0.0;
    } else if (x < 0.33807590140751) {
        return colormap_f(x);
    } else if (x < 0.50663669203696) {
        return (((-5.83014e2 * x - 8.38523e2) * x + 2.03823e3) * x - 4.86592e2) / 255.0;
    } else if (x < 0.84702285244773) {
        return 1.0;
    } else if (x < 1.0) {
        return (((-5.03306e2 * x + 2.95545e3) * x - 4.19210e3) * x + 1.99128e3) / 255.0;
    } else {
        return 251.0 / 255.0;
    }
}

float colormap_red(float x) {
    if (x < 0.16531216481302) {
        return 1.0;
    } else if (x < 0.33807590140751) {
        return (((-5.15164e3 * x + 5.30564e3) * x - 2.65098e3) * x + 5.70771e2) / 255.0;
    } else if (x < 0.67502056695956) {
        return colormap_f(x);
    } else if (x < 0.84702285244773) {
        return (((3.34136e3 * x - 9.01976e3) * x + 8.39740e3) * x - 2.41682e3) / 255.0;
    } else {
        return 1.0;
    }
}

vec4 colormap(float x) {
    return vec4(colormap_red(x), colormap_green(x), colormap_blue(x), 1.0);
}
