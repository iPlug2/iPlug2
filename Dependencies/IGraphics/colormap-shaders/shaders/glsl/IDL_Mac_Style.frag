float colormap_h(float x) {
	return -7.44992704834645E-01 * x + 7.47986634976377E-01;
}

float colormap_s(float x) {
	return 1.0;
}

float colormap_v(float x) {
	float i = mod(mod(x * 256.0, 2.0) + 2.0, 2.0);
	if (0.0 <= i && i < 1.0) {
		return 1.0;
	} else {
		return 254.0 / 255.0;
	}
}

vec4 colormap_hsv2rgb(float h, float s, float v) {
	float r = v;
	float g = v;
	float b = v;
	if (s > 0.0) {
		h *= 6.0;
		int i = int(h);
		float f = h - float(i);
		if (i == 1) {
            r *= 1.0 - s * f;
            b *= 1.0 - s;
		} else if (i == 2) {
			r *= 1.0 - s;
			b *= 1.0 - s * (1.0 - f);
		} else if (i == 3) {
			r *= 1.0 - s;
			g *= 1.0 - s * f;
		} else if (i == 4) {
			r *= 1.0 - s * (1.0 - f);
			g *= 1.0 - s;
		} else if (i == 5) {
			g *= 1.0 - s;
			b *= 1.0 - s * f;
		} else {
			g *= 1.0 - s * (1.0 - f);
			b *= 1.0 - s;
		}
	}
	return vec4(r, g, b, 1.0);
}

vec4 colormap(float x) {
	float h = colormap_h(clamp(x, 0.0, 1.0));
	float s = colormap_s(clamp(x, 0.0, 1.0));
	float v = colormap_v(clamp(x, 0.0, 1.0));
	return colormap_hsv2rgb(h, s, v);
}
