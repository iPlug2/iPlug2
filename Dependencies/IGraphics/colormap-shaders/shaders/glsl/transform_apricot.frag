float colormap_pi = 3.141592653589793;

float colormap_f(float x, float c) {
    return abs((((-5.563e-5 * x + 3.331e-16) * x + 3.045e-1) * x + 4.396e-12) * x + c);
}

float colormap_f2(float x) {
    return 262.0 * x + 12.0 * x * sin(((x - 8.0) * x + 66.0 * colormap_pi) * x);
}

float colormap_red(float x) {
    if (x < 0.0) {
        return 0.0;
    } else if (x < 1.0) {
        float r = colormap_f2(x);
        if (r > 255.0) {
            r = 510.0 - r;
        }
        return r / 255.0;
    } else {
        return 1.0;
    }
}


float colormap_green(float x) {
    if (x < 0.0) {
        return 0.0;
    } else if (x < 1.0) {
        return (109.0 * x + 25.0 * sin(9.92 * colormap_pi * x) * x) / 255.0;
    } else {
        return 102.0 / 255.0;
    }
}

float colormap_blue(float x) {
    float b = 0.0;
    x = x * 256.0;

    if (x < 0.0) {
        b = 241.0 / 255.0;
    } else if (x < 66.82) {
        b = colormap_f(x - 32.0, -27.0);
        if (x > 32.0 && b > 225.0) {
            b = b - 10.0;
        }
    } else if (x < 126.67) {
        b = colormap_f(x - 97.0, 30.0);
    } else if (x < 195.83) {
        b = colormap_f(x - 161.0, -27.0);
        if (x > 161.0 && b > 225.0) {
            b -= 10.0;
        }
    } else if (x < 256.0) {
        b = colormap_f(x - 226.0, 30.0);
    } else {
        b = 251.0;
    }
    if (b > 255.0) {
        b = 255.0;
    }

    return b / 255.0;
}

vec4 colormap(float x) {
    return vec4(colormap_red(x), colormap_green(x), colormap_blue(x), 1.0);
}
