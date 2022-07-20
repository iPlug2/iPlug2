#include <metal_stdlib>
using namespace metal;

namespace colormap {
namespace IDL {
namespace Black_White_Linear {

float4 colormap(float x) {
	float d = clamp(x, 0.0, 1.0);
	return float4(d, d, d, 1.0);
}

} // namespace Black_White_Linear
} // namespace IDL
} // namespace colormap
