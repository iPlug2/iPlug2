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

class PurpleRedStripes : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_Purple-Red+Stripes.frag"
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
		return std::string("Purple-Red+Stripes");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_h(float x) {\n"
			"	if (x < 0.3310005332481092) {\n"
			"		return (4.38091557495284E-01 * x - 6.45518032448820E-01) * x + 8.32595065985926E-01; // H1\n"
			"	} else if (x < 0.836653386) {\n"
			"		return -9.95987681712782E-01 * x + 9.96092924041492E-01; // H2\n"
			"	} else if (x < 0.9144920256360881) {\n"
			"		return (-3.25656183098909E+00 * x + 4.39104086393490E+00) * x - 1.23205476222211E+00; // H3\n"
			"	} else {\n"
			"		return (1.68724608409684E+00 * x - 3.93349028637749E+00) * x + 2.24617746415606E+00; // H4\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_s(float x) {\n"
			"	if (x < 0.9124516770628384) {\n"
			"		return -2.49531958245657E+00 * x + 3.07915192631601E+00; // S1\n"
			"	} else {\n"
			"		return 2.28056601637550E+00 * x - 1.27861289779857E+00; // S2\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_v(float x) {\n"
			"	float v = clamp(7.55217853407034E-01 * x + 7.48186662435193E-01, 0.0, 1.0);\n"
			"	float period = 0.039840637;\n"
			"	float t = x - 0.027888446;\n"
			"	float tt = t - float(int(t / period)) * period;\n"
			"	if (0.0 <= tt && tt < 0.007968127) {\n"
			"		v -= 0.2;\n"
			"	}\n"
			"	return v;\n"
			"}\n"
			"\n"
			"// H1 - H2 = 0\n"
			"// => [x=-1.186402343934078,x=0.3310005332481092]\n"
			"\n"
			"// H3 - H4 = 0\n"
			"// => [x=0.7693377859773962,x=0.9144920256360881]\n"
			"\n"
			"// S1 - 1 = 0\n"
			"// => [x=0.9124516770628384]\n"
			"\n"
			"vec4 colormap_hsv2rgb(float h, float s, float v) {\n"
			"	float r = v;\n"
			"	float g = v;\n"
			"	float b = v;\n"
			"	if (s > 0.0) {\n"
			"		h *= 6.0;\n"
			"		int i = int(h);\n"
			"		float f = h - float(i);\n"
			"		if (i == 1) {\n"
			"            r *= 1.0 - s * f;\n"
			"            b *= 1.0 - s;\n"
			"		} else if (i == 2) {\n"
			"			r *= 1.0 - s;\n"
			"			b *= 1.0 - s * (1.0 - f);\n"
			"		} else if (i == 3) {\n"
			"			r *= 1.0 - s;\n"
			"			g *= 1.0 - s * f;\n"
			"		} else if (i == 4) {\n"
			"			r *= 1.0 - s * (1.0 - f);\n"
			"			g *= 1.0 - s;\n"
			"		} else if (i == 5) {\n"
			"			g *= 1.0 - s;\n"
			"			b *= 1.0 - s * f;\n"
			"		} else {\n"
			"			g *= 1.0 - s * (1.0 - f);\n"
			"			b *= 1.0 - s;\n"
			"		}\n"
			"	}\n"
			"	return vec4(r, g, b, 1.0);\n"
			"}\n"
			"\n"
			"vec4 colormap(float x) {\n"
			"	float h = colormap_h(clamp(x, 0.0, 1.0));\n"
			"	float s = colormap_s(clamp(x, 0.0, 1.0));\n"
			"	float v = colormap_v(clamp(x, 0.0, 1.0));\n"
			"	return colormap_hsv2rgb(h, s, v);\n"
			"}\n"
		);
	}
};

} // namespace IDL
} // namespace colormap
