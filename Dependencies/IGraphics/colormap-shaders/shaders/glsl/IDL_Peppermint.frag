float colormap_red(float x) {
	return float(int(mod(x * 256.0 / 4.0, 4.0))) * 80.0;
}

float colormap_green(float x) {
	return float(int(x * 256.0 / 16.0)) * 16.0;
}

float colormap_blue(float x) {
	return float(int(mod(x * 256.0, 4.0))) * 80.0;
}

vec4 colormap(float x) {
	float r = clamp(colormap_red(x) / 255.0, 0.0, 1.0);
	float g = clamp(colormap_green(x) / 255.0, 0.0, 1.0);
	float b = clamp(colormap_blue(x) / 255.0, 0.0, 1.0);
	return vec4(r, g, b, 1.0);
}
