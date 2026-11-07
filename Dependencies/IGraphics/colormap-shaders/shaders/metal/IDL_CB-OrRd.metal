#include <metal_stdlib>
using namespace metal;

namespace colormap {
namespace IDL {
namespace CB_OrRd {

float colormap_erf(float x) {
	// erf approximation formula
	const float pi = 3.141592653589793238462643383279502884197169399;
	const float a = -8.0 * (pi - 3.0) / (3.0 * pi * (pi - 4.0));
	float v = 1.0 - exp(-x * x * (4.0 / pi + a * x * x) / (1.0 + a * x * x));
	return sign(x) * sqrt(v);
}

float colormap_red(float x) {
	return 127.0548665301906 * (1.0 - colormap_erf(2.926357498911938 * (x - 1.0)));
}

float colormap_green(float x) {
	if (x < 0.3619403852314316) {
		return (-1.54198428391755E+02 * x - 1.02444772146395E+02) * x + 2.46537152765234E+02;
	} else {
		return (-9.36243338922068E+01 * x - 2.52981049073614E+02) * x + 2.93087053416795E+02;
	}
}

float colormap_blue(float x) {
	if (x < 0.257177186368595) {
		return -3.12872659968267E+02 * x + 2.36904283447911E+02;
	} else if (x < 0.3782584246092673) {
		return -2.10348217636022E+02 * x + 2.10537335834895E+02;
	} else if (x < 0.4977365973504272) {
		return -3.49969458128060E+02 * x + 2.63350246305405E+02;
	} else if (x < 0.6243924786616242) {
		return -1.32763025210105E+02 * x + 1.55238655462198E+02;
	} else if (x < 0.7554357868699768) {
		return -3.23593609804140E+02 * x + 2.74391837181314E+02;
	} else {
		return -2.41701581027554E+02 * x + 2.12527667984095E+02;
	}
}

float4 colormap(float x) {
	float r = clamp(colormap_red(x) / 255.0, 0.0, 1.0);
	float g = clamp(colormap_green(x) / 255.0, 0.0, 1.0);
	float b = clamp(colormap_blue(x) / 255.0, 0.0, 1.0);
	return float4(r, g, b, 1.0);
}

} // namespace CB_OrRd
} // namespace IDL
} // namespace colormap
