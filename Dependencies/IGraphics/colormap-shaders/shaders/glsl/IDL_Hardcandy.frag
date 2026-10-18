float colormap_red(float x) {
	const float pi = 3.141592653589793238462643383279502884197169399;
	float v = sin(2.0 * pi * (1.0 / (2.0 * 0.143365330321852)) * x + 1.55) * (322.0 * x) + 191.0 * x;
	if (v < 0.0) {
		v = -v;
	} else if (v > 255.0) {
		v = 255.0 - (v - 255.0);
	}
	return v;
}

float colormap_green(float x) {
	const float pi = 3.141592653589793238462643383279502884197169399;
	float v = sin(2.0 * pi / 0.675 * (x + 0.015)) * 190.0 + 25.0;
	if (v < 0.0) {
		return -v;
	} else {
		return v;
	}
}

float colormap_blue(float x) {
	const float pi = 3.141592653589793238462643383279502884197169399;
	return sin(2.0 * pi * (x * -3.45 - 0.02)) * 127.5 + 127.5;
}

vec4 colormap(float x) {
    float r = clamp(colormap_red(x) / 255.0, 0.0, 1.0);
    float g = clamp(colormap_green(x) / 255.0, 0.0, 1.0);
    float b = clamp(colormap_blue(x) / 255.0, 0.0, 1.0);
    return vec4(r, g, b, 1.0);
}
