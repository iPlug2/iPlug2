vec4 colormap(float x) {
    return vec4(0.0, clamp(x, 0.0, 1.0), clamp(-0.5 * x + 1.0, 0.0, 1.0), 1.0);
}
