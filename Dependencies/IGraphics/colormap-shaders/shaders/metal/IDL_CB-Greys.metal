#include <metal_stdlib>
using namespace metal;

namespace colormap {
namespace IDL {
namespace CB_Greys {

float colormap_f1(float x) {
	if (x < 0.3849871446504941) {
		return (-1.97035589869658E+02 * x - 1.04694505989261E+02) * x + 2.54887830314633E+02;
	} else if (x < 0.7524552013985151) {
		return (8.71964614639801E+01 * x - 3.79941007690502E+02) * x + 3.18726712728548E+02;
	} else {
		return (2.28085532626505E+02 * x - 7.25770100421835E+02) * x + 4.99177793972139E+02;
	}
}

float4 colormap(float x) {
	float v = clamp(colormap_f1(x) / 255.0, 0.0, 1.0);
	return float4(v, v, v, 1.0);
}

} // namespace CB_Greys
} // namespace IDL
} // namespace colormap
