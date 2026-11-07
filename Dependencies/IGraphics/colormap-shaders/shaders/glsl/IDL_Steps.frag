float colormap_erf(float x) {
    // erf approximation formula
    const float pi = 3.141592653589793238462643383279502884197169399;
    const float a = -8.0 * (pi - 3.0) / (3.0 * pi * (pi - 4.0));
    float v = 1.0 - exp(-x * x * (4.0 / pi + a * x * x) / (1.0 + a * x * x));
    return sign(x) * sqrt(v);
}

float colormap_red(float x) {
    if (x <= 95.5) {
        return 8.14475806451613E+00 * x - 5.23967741935484E+02;
    } else {
        return colormap_erf((x - 145.0) * 0.028) * 131.0 + 125.0;
    }
}

float colormap_green(float x) {
    if (x < (3.14410256410256E+02 + 2.14285714285714E-01) / (4.25000000000000E+01 + 9.81196581196581E+00)) {
        return 4.25000000000000E+01 * x - 2.14285714285714E-01;
    } else if (x < 192.0) { // actual root: 193.529143410603
        return -9.81196581196581E+00 * x + 3.14410256410256E+02;
    } else {
        return ((5.35129859215999E-04 * x - 2.98599683017528E-01) * x + 5.69466901216655E+01) * x - 3.71604038989543E+03;
    }
}

float colormap_blue(float x) {
    if (x < 63.0) {
        return 8.22620967741936E+00 * x - 2.63729032258065E+02;
    } else if (x <= 95.5) {
        return 4.97690615835777E+00 * x - 3.16414039589443E+02;
    } else {
        return (((-7.88871743679920E-05 * x + 7.21525684930384E-02) * x - 2.45956037640571E+01) * x + 3.70824713134765E+03) * x - 2.08852518066406E+05;
    }
}

vec4 colormap(float x) {
    float t = x * 255.0 - 0.5;
    float r = clamp(colormap_red(t) / 255.0, 0.0, 1.0);
    float g = clamp(colormap_green(t) / 255.0, 0.0, 1.0);
    float b = clamp(colormap_blue(t) / 255.0, 0.0, 1.0);
    return vec4(r, g, b, 1.0);
}
