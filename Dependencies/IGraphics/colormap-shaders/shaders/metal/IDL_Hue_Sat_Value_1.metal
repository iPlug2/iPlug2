#include <metal_stdlib>
using namespace metal;

namespace colormap {
namespace IDL {
namespace Hue_Sat_Value_1 {

float colormap_low(float x) {
	return (3.31549320112257E-06 * x + 9.90093135228017E-01) * x - 2.85569629002368E-01;
}

float colormap_r2(float x) {
	float t = x - 172.2021990097892;
	return (-2.39029325463818E-02 * t + 2.98715296752437E+00) * t + 170.308961782;
}

float colormap_g1(float x) {
	float t = x - 86.01791713538523;
	return (-2.29102048531908E-02 * t + 4.93089581616270E+00) * t + 84.9047112396;
}

float colormap_g2(float x) {
	float t = x - 215.6804047700857;
	return ((-2.84214232291614E-04 * t + 3.97502254733824E-02) * t - 1.21659773743153E+00) * t + 255.0;
}

float colormap_b2(float x) {
	float t = x - 129.1625547389263;
	return (2.47358957372179E-02 * t - 3.03236899995258E+00) * t + 255.0;
}

float colormap_red(float x) {
	if (x < 43.15291462916737) {
		return 255.0;
	} else if (x < 86.01791713538523) {
		return (2.31649880284905E-02 * x - 6.92920742890475E+00) * x + 5.09541054138852E+02; // R1
	} else if (x < 172.2021990097892) {
		return colormap_low(x);
	} else if (x < 215.6804047700857) {
		return colormap_r2(x);
	} else {
		return 255.0;
	}
}

float colormap_green(float x) {
	if (x < 86.01791713538523) {
		return colormap_low(x);
	} else if (x < 129.1625547389263) {
		return colormap_g1(x);
	} else if (x < 215.6804047700857) {
		return 255.0;
	} else {
		return colormap_g2(x);
	}
}

float colormap_blue(float x) {
	if (x < 43.15291462916737) {
		return (-2.29056417125175E-02 * x + 6.89833449327894E+00) * x - 2.89480825884616E-02; // B1
	} else if (x < 129.1625547389263) {
		return 255.0;
	} else if (x < 172.2021990097892) {
		return colormap_b2(x);
	} else {
		return colormap_low(x);
	}
}

// B1 - 255 = 0
// => [x=43.15291462916737,x=258.0102040408121]

// R1 - low = 0
// => [x=86.01791713538523,x=255.8961027639475]

// G1 - 255 = 0
// => [x=129.1625547389263,x=258.1003299995292]

// B2 - low = 0
// => [x=172.2021990097892,x=248.7957319298701]

// R2 - 255 = 0
// => [x=215.6804047700857,x=253.6941391396688]

// low(86.01791713538523) = 84.9047112396
// low(172.2021990097892) = 170.308961782

float4 colormap(float x) {
	float t = x * 255.0;
	float r = clamp(colormap_red(t) / 255.0, 0.0, 1.0);
	float g = clamp(colormap_green(t) / 255.0, 0.0, 1.0);
	float b = clamp(colormap_blue(t) / 255.0, 0.0, 1.0);
	return float4(r, g, b, 1.0);
}

} // namespace Hue_Sat_Value_1
} // namespace IDL
} // namespace colormap
