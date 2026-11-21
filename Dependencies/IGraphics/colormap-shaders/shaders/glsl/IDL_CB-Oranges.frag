float colormap_red(float x) {
	if (x < 0.4798405468463898) {
		return 255.0;
	} else if (x < 0.7629524491886985) {
		return (-3.67617638383468E+02 * x + 3.17332748024787E+02) * x + 1.85373720793787E+02;
	} else {
		return (3.68357233392831E+02 * x - 1.00617951362078E+03) * x + 7.66695019519326E+02;
	}
}

float colormap_green(float x) {
	if (x < 0.748539247687408) {
		return (((-8.92644295264035E+01 * x + 3.93421870424412E+02) * x - 4.73834129104390E+02) * x - 5.60962544745416E+01) * x + 2.43354168263028E+02;
	} else {
		return (1.06683260838348E+02 * x - 3.18020138166420E+02) * x + 2.51126712492908E+02;
	}
}

float colormap_blue(float x) {
	if (x < 0.76) {
		return ((7.32034492098544E+02 * x - 7.55283914444663E+02) * x - 1.53168890861198E+02) * x + 2.33567667053916E+02;
	} else {
		return 1.23702752385982E+01 * x - 8.09423081765692E+00;
	}
}

vec4 colormap(float x) {
	float r = clamp(colormap_red(x) / 255.0, 0.0, 1.0);
	float g = clamp(colormap_green(x) / 255.0, 0.0, 1.0);
	float b = clamp(colormap_blue(x) / 255.0, 0.0, 1.0);
	return vec4(r, g, b, 1.0);
}
