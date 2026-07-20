float colormap_red(float x) {
    if (x < 129.0) {
        return -1.95881595881596E+00 * x + 4.39831831831832E+02;
    } else {
        return 5.70897317298797E+00 * x - 1.11405615171138E+03;
    }
}

float colormap_green(float x) {
    if (x < 129.0) {
        return 0.0;
    } else if (x < 200.0) {
        return 5.72337662337662E+00 * x - 6.06801082251082E+02;
    } else {
        return -5.58823529411765E+00 * x + 1.59313235294118E+03;
    }
}

float colormap_blue(float x) {
    if (x < 120.0) {
        return 1.95784725990233E+00 * x + 6.90962481913547E+01;
    } else {
        return -5.71881606765328E+00 * x + 1.11517336152220E+03;
    }
}

vec4 colormap(float x) {
    float t = x * 255.0;
    float r = clamp(colormap_red(t) / 255.0, 0.0, 1.0);
    float g = clamp(colormap_green(t) / 255.0, 0.0, 1.0);
    float b = clamp(colormap_blue(t) / 255.0, 0.0, 1.0);
    return vec4(r, g, b, 1.0);
}
