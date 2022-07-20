float colormap_low(float x) {
	return (-1.91005335917480E-03 * x + 1.49751468348116E+00) * x - 6.97037614414503E+00; // lower
}

float colormap_up(float x) {
	return (1.88420526249161E-03 * x - 5.03556849093925E-01) * x + 2.55777688663313E+02; // upper
}

float colormap_red(float x) {
	if (x < 43.76015739302458) { // first root of `lower = B1`
		return colormap_up(x);
	} else if (x < 86.85552651930304) { // first root of `lower = R1`
		return (3.42882679808412E-02 * x - 7.47424573913507E+00) * x + 4.99200716753466E+02; // R1
	} else if (x < 174.6136813850324) { // first root of `low = B2`
		return colormap_low(x);
	} else {
		return ((1.12237347384081E-04 * x - 7.83534162528667E-02) * x + 1.86033275155350E+01) * x - 1.25879271751642E+03;
	}
}

float colormap_green(float x) {
	if (x < 86.85552651930304) {
		return colormap_low(x);
	} else if (x < 130.6514942376722) {
		return (-2.86318899478317E-02 * x + 8.83599571161434E+00) * x - 4.43544771805581E+02; // G1
	} else {
		return colormap_up(x);
	}
}

float colormap_blue(float x) {
	if (x < 43.76015739302458) {
		return (-3.50205069618621E-02 * x + 7.15746326474339E+00) * x - 8.79902788903102E+00; // B1
	} else if (x < 130.6514942376722) { // first root of `upper = G1`
		return colormap_up(x);
	} else if (x < 174.6136813850324) { // first root of `low = B2`
		return (1.99506804131033E-02 * x - 6.64847464240324E+00) * x + 7.48898397192062E+02; // B2
	} else {
		return colormap_low(x);
	}
}

vec4 colormap(float x) {
	float t = x * 255.0;
	float r = clamp(colormap_red(t) / 255.0, 0.0, 1.0);
	float g = clamp(colormap_green(t) / 255.0, 0.0, 1.0);
	float b = clamp(colormap_blue(t) / 255.0, 0.0, 1.0);
	return vec4(r, g, b, 1.0);
}
