float colormap_red(float x) {
	const float pi = 3.141592653589793238462643383279502884197169399;
	const float a = 94.58052830612496;
	const float b = 0.5059881077994055;
	const float c = 0.5079410623689743;
	const float d = 86.68342149719986;
	float v = a * sin(2.0 * pi / b * x + 2.0 * pi * c) + d;
	if (v < 0.0) {
		return -v;
	} else {
		return v;
	}
}

float colormap_green(float x) {
	const float pi = 3.141592653589793238462643383279502884197169399;
	const float a = 99.19361086687104;
	const float b = 1.095358975873744;
	const float c = 0.3679001899352902;
	const float d = 0.7603616684267874;
	const float e = 219.7852186508229;
	const float f = 1.161240703555854;
	float v = (a * x + b) * sin(2.0 * pi * (x / c + d)) + e * x + f;
	if (v > 255.0) {
		return 510.0 - v;
	} else {
		return v;
	}
}

float colormap_blue(float x) {
	const float pi = 3.141592653589793238462643383279502884197169399;
	const float a = 253.8552881642787;
	const float b = 0.5059881077994055;
	const float c = 0.5079410623689743;
	const float d = 226.7149651787587;
	float v = a * sin(2.0 * pi * (x / b + c)) + d;
	if (v > 255.0) {
		return 510.0 - v;
	} else if (v < 0.0) {
		return -v;
	} else {
		return v;
	}
}

vec4 colormap(float x) {
	float r = clamp(colormap_red(x) / 255.0, 0.0, 1.0);
	float g = clamp(colormap_green(x) / 255.0, 0.0, 1.0);
	float b = clamp(colormap_blue(x) / 255.0, 0.0, 1.0);
	return vec4(r, g, b, 1.0);
}
