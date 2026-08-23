#include <metal_stdlib>
using namespace metal;

namespace colormap {
namespace IDL {
namespace CB_YIOrBr {

float colormap_red(float x) {
	return ((((1.30858855846896E+03 * x - 2.84649723684787E+03) * x + 1.76048857883363E+03) * x - 3.99775093706324E+02) * x + 2.69759225316811E+01) * x + 2.54587325383574E+02;
}

float colormap_green(float x) {
	return ((((-8.85605750526301E+02 * x + 2.20590941129997E+03) * x - 1.50123293069936E+03) * x + 2.38490009587258E+01) * x - 6.03460495073813E+01) * x + 2.54768707485247E+02;
}

float colormap_blue(float x) {
	if (x < 0.2363454401493073) {
		return (-3.68734834041388E+01 * x - 3.28163398692792E+02) * x + 2.27342862588147E+02;
	} else if (x < 0.7571054399013519) {
		return ((((1.60988309475108E+04 * x - 4.18782706486673E+04) * x + 4.14508040221340E+04) * x - 1.88926043556059E+04) * x + 3.50108270140290E+03) * x - 5.28541997751406E+01;
	} else {
		return 1.68513761929930E+01 * x - 1.06424668227935E+01;
	}
}

float4 colormap(float x) {
	float r = clamp(colormap_red(x) / 255.0, 0.0, 1.0);
	float g = clamp(colormap_green(x) / 255.0, 0.0, 1.0);
	float b = clamp(colormap_blue(x) / 255.0, 0.0, 1.0);
	return float4(r, g, b, 1.0);
}

} // namespace CB_YIOrBr
} // namespace IDL
} // namespace colormap
