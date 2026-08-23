#include <metal_stdlib>
using namespace metal;

namespace colormap {
namespace IDL {
namespace CB_Dark2 {

float colormap_red(float x) {
	if (x < 0.1420141309011431) {
		return 1.34039497392129E+03 * x + 2.69843527738265E+01; // R1
	} else if (x < 0.2832372296870317) {
		return -7.10676962676977E+02 * x + 3.18265551265554E+02; // R2
	} else if (x < 0.4289458478001075) {
		return 7.81992413466075E+02 * x - 1.04513987671874E+02; // R3
	} else if (x < 0.5708308530153928) {
		return -9.08417760617793E+02 * x + 6.20580437580458E+02; // R4
	} else if (x < 0.7161145088911144) {
		return 8.78642484589930E+02 * x - 3.99528686581363E+02; // R5
	} else {
		return -4.46088623062543E+02 * x + 5.49130479987969E+02; // R6
	}
}

float colormap_green(float x) {
	if (x < 0.1423373623518399) {
		return -4.42075062917170E+02 * x + 1.57882591093117E+02;
	} else if (x < 0.2834256193936661) {
		return 1.20312044817928E+02 * x + 7.78338935574229E+01;
	} else if (x < 0.4289129211292926) {
		return -4.87706495969635E+02 * x + 2.50161925082971E+02;
	} else if (x < 0.5707509745277107) {
		return 8.81285199485221E+02 * x - 3.37016302016313E+02;
	} else if (x < 0.7167318372796874) {
		return 3.38425794215443E+01 * x + 1.46662399241347E+02;
	} else if (x < 0.8574321486576632) {
		return -3.74788931788816E+02 * x + 4.39541613041524E+02;
	} else {
		return -1.11864607464601E+02 * x + 2.14101844701848E+02;
	}
}

float colormap_blue(float x) {
	if (x < 0.1416506418642016) {
		return -8.25590327169275E+02 * x + 1.19073968705548E+02;
	} else if (x < 0.283798849411819) {
		return 1.24433848133849E+03 * x - 1.74132775632779E+02;
	} else if (x < 0.4285299211147395) {
		return -2.82844476054998E+02 * x + 2.59279990516832E+02;
	} else if (x < 0.5700066234111405) {
		return -7.58992535392565E+02 * x + 4.63323680823696E+02;
	} else if (x < 0.7144281197454347) {
		return -1.96085154061629E+02 * x + 1.42462745098042E+02;
	} else if (x < 0.8576876140758521) {
		return 1.84489769121314E+02 * x - 1.29430681693811E+02;
	} else {
		return 4.99467953667967E+02 * x - 3.99583569283573E+02;
	}
}

float4 colormap(float x) {
	float r = clamp(colormap_red(x) / 255.0, 0.0, 1.0);
	float g = clamp(colormap_green(x) / 255.0, 0.0, 1.0);
	float b = clamp(colormap_blue(x) / 255.0, 0.0, 1.0);
	return float4(r, g, b, 1.0);
}

} // namespace CB_Dark2
} // namespace IDL
} // namespace colormap
