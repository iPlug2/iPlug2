float colormap_up(float x) {
	return (1.88200166286601E-03 * x - 4.65545143706978E-01) * x + 2.51008231568770E+02;
}

float colormap_low(float x) {
	return (-1.90879354636631E-03 * x - 5.05775136749144E-01) * x + 2.51839633472648E+02;
}

float colormap_r1(float x) {
	float t = x - 84.41170691108532;
	return ((-1.30664056487685E-04 * t - 2.23609578814399E-02) * t - 1.63427831229829E+00) * t + colormap_low(84.41170691108532);
}

float colormap_r2(float x) {
	float t = (x - 172.4679464259528);
	return (3.39051205856669E-02 * t + 1.53777364753859E+00) * t + colormap_low(172.4679464259528);
}

float colormap_g1(float x) {
	return (2.06966753567031E-02 * x - 3.81765550976615E+00) * x + 3.70329541512642E+02;
}

float colormap_g2(float x) {
	float t = x - 215.8140719563986;
	return (-2.93369381849802E-02 * t - 4.45609461245051E+00) * t + colormap_up(215.8140719563986);
}

float colormap_b1(float x) {
	float t = (x - 129.0039558892991);
	return (-2.69029805601284E-02 * t - 1.46365429919324E+00) * t + colormap_up(129.0039558892991);
}

float colormap_red(float x) {
	if (x < 84.41170691108532) {
		return colormap_r1(x);
	} else if (x < 172.4679464259528) {
		return colormap_low(x);
	} else if (x < 215.8140719563986) {
		return colormap_r2(x);
	} else {
		return colormap_up(x);
	}
}

float colormap_green(float x) {
	if (x < 84.41170691108532) {
		return colormap_low(x);
	} else if (x < 129.0039558892991) {
		return colormap_g1(x);
	} else if (x < 215.8140719563986) {
		return colormap_up(x);
	} else {
		return colormap_g2(x);
	}
}

float colormap_blue(float x) {
	if (x < 129.0039558892991) {
		return colormap_up(x);
	} else if (x < 172.4679464259528) {
		return colormap_b1(x);
	} else {
		return colormap_low(x);
	}
}

// G1 = low
// => [x=62.09621943267293,x=84.41170691108532]

// G1 = up
// => [x=49.16072666680554,x=129.0039558892991]

// B1 = low
// => [x=66.91982278615977,x=172.4679464259528]

// R2 = up
// => [x=86.8352194379599,x=215.8140719563986]

// low(172.4679464259528) = 107.83220272
// up(215.8140719563986) = 238.192608973

vec4 colormap(float x) {
	float t = x * 255.0;
	float r = clamp(colormap_red(t) / 255.0, 0.0, 1.0);
	float g = clamp(colormap_green(t) / 255.0, 0.0, 1.0);
	float b = clamp(colormap_blue(t) / 255.0, 0.0, 1.0);
	return vec4(r, g, b, 1.0);
}
