vec4 colormap(float x) {
    float v = cos(133.0 * x) * 28.0 + 230.0 * x + 27.0;
    if (v > 255.0) {
        v = 510.0 - v;
    }
    v = v / 255.0;
    return vec4(v, v, v, 1.0);
}
