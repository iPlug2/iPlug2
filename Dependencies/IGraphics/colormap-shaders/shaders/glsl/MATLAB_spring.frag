vec4 colormap(float x) {
    return vec4(1.0, clamp(x, 0.0, 1.0), clamp(1.0 - x, 0.0, 1.0), 1.0);
}
