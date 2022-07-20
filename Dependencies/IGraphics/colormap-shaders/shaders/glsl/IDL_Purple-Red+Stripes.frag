float colormap_h(float x) {
	if (x < 0.3310005332481092) {
		return (4.38091557495284E-01 * x - 6.45518032448820E-01) * x + 8.32595065985926E-01; // H1
	} else if (x < 0.836653386) {
		return -9.95987681712782E-01 * x + 9.96092924041492E-01; // H2
	} else if (x < 0.9144920256360881) {
		return (-3.25656183098909E+00 * x + 4.39104086393490E+00) * x - 1.23205476222211E+00; // H3
	} else {
		return (1.68724608409684E+00 * x - 3.93349028637749E+00) * x + 2.24617746415606E+00; // H4
	}
}

float colormap_s(float x) {
	if (x < 0.9124516770628384) {
		return -2.49531958245657E+00 * x + 3.07915192631601E+00; // S1
	} else {
		return 2.28056601637550E+00 * x - 1.27861289779857E+00; // S2
	}
}

float colormap_v(float x) {
	float v = clamp(7.55217853407034E-01 * x + 7.48186662435193E-01, 0.0, 1.0);
	float period = 0.039840637;
	float t = x - 0.027888446;
	float tt = t - float(int(t / period)) * period;
	if (0.0 <= tt && tt < 0.007968127) {
		v -= 0.2;
	}
	return v;
}

// H1 - H2 = 0
// => [x=-1.186402343934078,x=0.3310005332481092]

// H3 - H4 = 0
// => [x=0.7693377859773962,x=0.9144920256360881]

// S1 - 1 = 0
// => [x=0.9124516770628384]

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
