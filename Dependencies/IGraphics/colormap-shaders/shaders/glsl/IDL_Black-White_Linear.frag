vec4 colormap(float x) {
	float d = clamp(x, 0.0, 1.0);
	return vec4(d, d, d, 1.0);
}
