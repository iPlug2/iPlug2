#include <metal_stdlib>
using namespace metal;

namespace colormap {
namespace IDL {
namespace CB_BuPu {

float colormap_red(float x) {
	if (x < 0.5) {
		return ((9.36778020337806E+02 * x - 6.50118321723071E+02) * x - 1.17443717298418E+02) * x + 2.45640353186226E+02;
	} else if (x < 0.8774830563107102) {
		return (-1.07698286837105E+02 * x + 1.18941048271099E+02) * x + 1.07226580391914E+02;
	} else {
		return (-9.74524977247347E+01 * x - 2.28433367883516E+02) * x + 4.04152727778340E+02;
	}
}

float colormap_green(float x) {
	if (x < 0.8733318961770479) {
		return ((5.48640317293175E+01 * x - 2.50041384768192E+02) * x - 9.10862643329019E+01) * x + 2.50330566129102E+02;
	} else {
		return (1.99199516170089E+02 * x - 4.99460567863480E+02) * x + 3.00881779984708E+02;
	}
}

float colormap_blue(float x) {
	return ((((-5.85479883993044E+02 * x + 1.17558327595144E+03) * x - 8.53086782991886E+02) * x + 1.82921150609850E+02) * x - 9.97610091178212E+01) * x + 2.53898307388663E+02;
}

float4 colormap(float x) {
	float r = clamp(colormap_red(x) / 255.0, 0.0, 1.0);
	float g = clamp(colormap_green(x) / 255.0, 0.0, 1.0);
	float b = clamp(colormap_blue(x) / 255.0, 0.0, 1.0);
	return float4(r, g, b, 1.0);
}

} // namespace CB_BuPu
} // namespace IDL
} // namespace colormap
