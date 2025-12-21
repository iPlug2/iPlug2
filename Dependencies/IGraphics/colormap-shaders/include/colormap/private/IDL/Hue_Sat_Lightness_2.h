/**
 * This file was automatically created with "create_c++_header.sh".
 * Do not edit manually.
 */
#pragma once
#include "../../colormap.h"

namespace colormap
{
namespace IDL
{

class HueSatLightness2 : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_Hue_Sat_Lightness_2.frag"
		#undef float
	};

public:
	Color getColor(double x) const override
	{
		Wrapper w;
		vec4 c = w.colormap(x);
		Color result;
		result.r = std::max(0.0, std::min(1.0, c.r));
		result.g = std::max(0.0, std::min(1.0, c.g));
		result.b = std::max(0.0, std::min(1.0, c.b));
		result.a = std::max(0.0, std::min(1.0, c.a));
		return result;
	}

	std::string getTitle() const override
	{
		return std::string("Hue_Sat_Lightness_2");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_up(float x) {\n"
			"	return (1.88200166286601E-03 * x - 4.65545143706978E-01) * x + 2.51008231568770E+02;\n"
			"}\n"
			"\n"
			"float colormap_low(float x) {\n"
			"	return (-1.90879354636631E-03 * x - 5.05775136749144E-01) * x + 2.51839633472648E+02;\n"
			"}\n"
			"\n"
			"float colormap_r1(float x) {\n"
			"	float t = x - 84.41170691108532;\n"
			"	return ((-1.30664056487685E-04 * t - 2.23609578814399E-02) * t - 1.63427831229829E+00) * t + colormap_low(84.41170691108532);\n"
			"}\n"
			"\n"
			"float colormap_r2(float x) {\n"
			"	float t = (x - 172.4679464259528);\n"
			"	return (3.39051205856669E-02 * t + 1.53777364753859E+00) * t + colormap_low(172.4679464259528);\n"
			"}\n"
			"\n"
			"float colormap_g1(float x) {\n"
			"	return (2.06966753567031E-02 * x - 3.81765550976615E+00) * x + 3.70329541512642E+02;\n"
			"}\n"
			"\n"
			"float colormap_g2(float x) {\n"
			"	float t = x - 215.8140719563986;\n"
			"	return (-2.93369381849802E-02 * t - 4.45609461245051E+00) * t + colormap_up(215.8140719563986);\n"
			"}\n"
			"\n"
			"float colormap_b1(float x) {\n"
			"	float t = (x - 129.0039558892991);\n"
			"	return (-2.69029805601284E-02 * t - 1.46365429919324E+00) * t + colormap_up(129.0039558892991);\n"
			"}\n"
			"\n"
			"float colormap_red(float x) {\n"
			"	if (x < 84.41170691108532) {\n"
			"		return colormap_r1(x);\n"
			"	} else if (x < 172.4679464259528) {\n"
			"		return colormap_low(x);\n"
			"	} else if (x < 215.8140719563986) {\n"
			"		return colormap_r2(x);\n"
			"	} else {\n"
			"		return colormap_up(x);\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"	if (x < 84.41170691108532) {\n"
			"		return colormap_low(x);\n"
			"	} else if (x < 129.0039558892991) {\n"
			"		return colormap_g1(x);\n"
			"	} else if (x < 215.8140719563986) {\n"
			"		return colormap_up(x);\n"
			"	} else {\n"
			"		return colormap_g2(x);\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"	if (x < 129.0039558892991) {\n"
			"		return colormap_up(x);\n"
			"	} else if (x < 172.4679464259528) {\n"
			"		return colormap_b1(x);\n"
			"	} else {\n"
			"		return colormap_low(x);\n"
			"	}\n"
			"}\n"
			"\n"
			"// G1 = low\n"
			"// => [x=62.09621943267293,x=84.41170691108532]\n"
			"\n"
			"// G1 = up\n"
			"// => [x=49.16072666680554,x=129.0039558892991]\n"
			"\n"
			"// B1 = low\n"
			"// => [x=66.91982278615977,x=172.4679464259528]\n"
			"\n"
			"// R2 = up\n"
			"// => [x=86.8352194379599,x=215.8140719563986]\n"
			"\n"
			"// low(172.4679464259528) = 107.83220272\n"
			"// up(215.8140719563986) = 238.192608973\n"
			"\n"
			"vec4 colormap(float x) {\n"
			"	float t = x * 255.0;\n"
			"	float r = clamp(colormap_red(t) / 255.0, 0.0, 1.0);\n"
			"	float g = clamp(colormap_green(t) / 255.0, 0.0, 1.0);\n"
			"	float b = clamp(colormap_blue(t) / 255.0, 0.0, 1.0);\n"
			"	return vec4(r, g, b, 1.0);\n"
			"}\n"
		);
	}
};

} // namespace IDL
} // namespace colormap
