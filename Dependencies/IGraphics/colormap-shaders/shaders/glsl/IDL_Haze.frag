float colormap_red(float x) {
    if (x < 0.0) {
        return 167.0;
    } else if (x < (2.54491177159840E+02 + 2.49117061281287E+02) / (1.94999353031535E+00 + 1.94987400471999E+00)) {
        return -1.94987400471999E+00 * x + 2.54491177159840E+02;
    } else if (x <= 255.0) {
        return 1.94999353031535E+00 * x - 2.49117061281287E+02;
    } else {
        return 251.0;
    }
}

float colormap_green(float x) {
    if (x < 0.0) {
        return 112.0;
    } else if (x < (2.13852573128775E+02 + 1.42633630462899E+02) / (1.31530121382008E+00 + 1.39181683887691E+00)) {
        return -1.39181683887691E+00 * x + 2.13852573128775E+02;
    } else if (x <= 255.0) {
        return 1.31530121382008E+00 * x - 1.42633630462899E+02;
    } else {
        return 195.0;
    }
}

float colormap_blue(float x) {
    if (x < 0.0) {
        return 255.0;
    } else if (x <= 255.0) {
        return -9.84241021836929E-01 * x + 2.52502692064968E+02;
    } else {
        return 0.0;
    }
}

vec4 colormap(float x) {
    float t = x * 255.0;
    float r = colormap_red(t) / 255.0;
    float g = colormap_green(t) / 255.0;
    float b = colormap_blue(t) / 255.0;
    return vec4(r, g, b, 1.0);
}
