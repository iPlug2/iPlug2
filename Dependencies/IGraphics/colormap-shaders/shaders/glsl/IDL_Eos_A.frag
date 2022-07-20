float colormap_h(float x) {
	if (x < 0.1151580585723306) {
		return (2.25507158009032E+00 * x - 1.17973110308697E+00) * x + 7.72551618145170E-01; // H1
	} else if (x < (9.89643667779019E-01 - 6.61604251019618E-01) / (2.80520737708568E+00 - 1.40111938331467E+00)) {
		return -2.80520737708568E+00 * x + 9.89643667779019E-01; // H2
	} else if (x < (6.61604251019618E-01 - 4.13849520734156E-01) / (1.40111938331467E+00 - 7.00489176507247E-01)) {
		return -1.40111938331467E+00 * x + 6.61604251019618E-01; // H3
	} else if (x < (4.13849520734156E-01 - 2.48319927251200E-01) / (7.00489176507247E-01 - 3.49965224045823E-01)) {
		return -7.00489176507247E-01 * x + 4.13849520734156E-01; // H4
	} else {
		return -3.49965224045823E-01 * x + 2.48319927251200E-01; // H5
	}
}

float colormap_v(float x) {
	float v = 1.0;
	if (x < 0.5) {
		v = clamp(2.10566088679245E+00 * x + 7.56360684411500E-01, 0.0, 1.0);
	} else {
		v = clamp(-1.70132918347782E+00 * x + 2.20637371757606E+00, 0.0, 1.0);
	}
	float period = 4.0 / 105.0;
	float len = 3.0 / 252.0;
	float t = mod(x + 7.0 / 252.0, period);
	if (0.0 <= t && t < len) {
		if (x < 0.12) {
			v = (1.87862631683169E+00 * x + 6.81498517051705E-01);
		} else if (x < 0.73) {
			v -= 26.0 / 252.0; 
		} else {
			v = -1.53215278202992E+00 * x + 1.98649818445446E+00;
		}
	}
	return v;
}

// H1 - H2 = 0
// => [x=-0.8359672286003642,x=0.1151580585723306]

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
	float s = 1.0;
	float v = colormap_v(clamp(x, 0.0, 1.0));
	return colormap_hsv2rgb(h, s, v);
}
