float colormap_red(float x) {
	if (x < 0.8724578971287745) {
		return ((((-2.98580898761749E+03 * x + 6.75014845489710E+03) * x - 4.96941610635258E+03) * x + 1.20190439358912E+03) * x - 2.94374708396149E+02) * x + 2.48449410219242E+02;
	} else {
		return 8.0;
	}
}

float colormap_green(float x) {
	if (x < 0.3725897611307026) {
		return -1.30453729372935E+02 * x + 2.51073069306930E+02;
	} else {
		return (-4.97095598364922E+01 * x - 1.77638812495581E+02) * x + 2.75554584848896E+02;
	}
}

float colormap_blue(float x) {
	if (x < 0.8782350698420436) {
		return (((-1.66242968759033E+02 * x + 2.50865766027010E+02) * x - 1.82046165445353E+02) * x - 3.29698266187334E+01) * x + 2.53927912915449E+02;
	} else {
		return -3.85153281423831E+02 * x + 4.93849833147981E+02;
	}
}

vec4 colormap(float x) {
	float r = clamp(colormap_red(x) / 255.0, 0.0, 1.0);
	float g = clamp(colormap_green(x) / 255.0, 0.0, 1.0);
	float b = clamp(colormap_blue(x) / 255.0, 0.0, 1.0);
	return vec4(r, g, b, 1.0);
}
