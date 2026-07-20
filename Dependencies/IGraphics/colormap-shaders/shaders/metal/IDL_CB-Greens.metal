#include <metal_stdlib>
using namespace metal;

namespace colormap {
namespace IDL {
namespace CB_Greens {

float colormap_red(float x) {
	if (x < 0.6193682068820651) {
		return ((1.41021531432983E+02 * x - 3.78122271460656E+02) * x - 1.08403692154170E+02) * x + 2.45743977533647E+02;
	} else {
		return ((-8.63146749682724E+02 * x + 1.76195389457266E+03) * x - 1.43807716183136E+03) * x + 4.86922446232568E+02;
	}
}

float colormap_green(float x) {
	return (-1.37013460576160E+02 * x - 4.54698187198101E+01) * x + 2.52098684286706E+02;
}

float colormap_blue(float x) {
	if (x < 0.5062477983469252) {
		return ((3.95067226937040E+02 * x - 4.52381961582927E+02) * x - 1.25304923569201E+02) * x + 2.43770002412197E+02;
	} else {
		return ((2.98249378459208E+02 * x - 6.14859580726999E+02) * x + 2.22299590241459E+02) * x + 1.21998454489668E+02;
	}
}

float4 colormap(float x) {
	float r = clamp(colormap_red(x) / 255.0, 0.0, 1.0);
	float g = clamp(colormap_green(x) / 255.0, 0.0, 1.0);
	float b = clamp(colormap_blue(x) / 255.0, 0.0, 1.0);
	return float4(r, g, b, 1.0);
}

} // namespace CB_Greens
} // namespace IDL
} // namespace colormap
