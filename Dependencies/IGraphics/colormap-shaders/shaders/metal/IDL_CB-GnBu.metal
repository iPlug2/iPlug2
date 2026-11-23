#include <metal_stdlib>
using namespace metal;

namespace colormap {
namespace IDL {
namespace CB_GnBu {

float colormap_red(float x) {
	float v = ((((-2.83671754639782E+03 * x + 6.51753994553536E+03) * x - 5.00110948171466E+03) * x + 1.30359712298773E+03) * x - 2.89958300810074E+02) * x + 2.48458039402758E+02;
	if (v < 8.0) {
		return 8.0;
	} else {
		return v;
	}
}

float colormap_green(float x) {
	return (((((-1.36304822155833E+03 * x + 4.37691418182849E+03) * x - 5.01802019417285E+03) * x + 2.39971481269598E+03) * x - 5.65401491984724E+02) * x - 1.48189675724133E+01) * x + 2.50507618187374E+02;
}

float colormap_blue(float x) {
	if (x < 0.3756393599187693) {
		return (9.62948273917718E+01 * x - 1.96136874142438E+02) * x + 2.41033490809633E+02;
	} else if (x < 0.6215448666633865) {
		return 1.21184043778803E+02 * x + 1.35422939068100E+02;
	} else if (x < 0.8830064316178203) {
		return -1.53052165744713E+02 * x + 3.05873047350666E+02;
	} else {
		return -3.49468965517114E+02 * x + 4.79310344827486E+02;
	}
}

float4 colormap(float x) {
	float r = clamp(colormap_red(x) / 255.0, 0.0, 1.0);
	float g = clamp(colormap_green(x) / 255.0, 0.0, 1.0);
	float b = clamp(colormap_blue(x) / 255.0, 0.0, 1.0);
	return float4(r, g, b, 1.0);
}

} // namespace CB_GnBu
} // namespace IDL
} // namespace colormap
