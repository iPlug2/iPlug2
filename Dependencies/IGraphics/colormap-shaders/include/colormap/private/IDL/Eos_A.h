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

class EosA : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_Eos_A.frag"
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
		return std::string("Eos_A");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_h(float x) {\n"
			"	if (x < 0.1151580585723306) {\n"
			"		return (2.25507158009032E+00 * x - 1.17973110308697E+00) * x + 7.72551618145170E-01; // H1\n"
			"	} else if (x < (9.89643667779019E-01 - 6.61604251019618E-01) / (2.80520737708568E+00 - 1.40111938331467E+00)) {\n"
			"		return -2.80520737708568E+00 * x + 9.89643667779019E-01; // H2\n"
			"	} else if (x < (6.61604251019618E-01 - 4.13849520734156E-01) / (1.40111938331467E+00 - 7.00489176507247E-01)) {\n"
			"		return -1.40111938331467E+00 * x + 6.61604251019618E-01; // H3\n"
			"	} else if (x < (4.13849520734156E-01 - 2.48319927251200E-01) / (7.00489176507247E-01 - 3.49965224045823E-01)) {\n"
			"		return -7.00489176507247E-01 * x + 4.13849520734156E-01; // H4\n"
			"	} else {\n"
			"		return -3.49965224045823E-01 * x + 2.48319927251200E-01; // H5\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_v(float x) {\n"
			"	float v = 1.0;\n"
			"	if (x < 0.5) {\n"
			"		v = clamp(2.10566088679245E+00 * x + 7.56360684411500E-01, 0.0, 1.0);\n"
			"	} else {\n"
			"		v = clamp(-1.70132918347782E+00 * x + 2.20637371757606E+00, 0.0, 1.0);\n"
			"	}\n"
			"	float period = 4.0 / 105.0;\n"
			"	float len = 3.0 / 252.0;\n"
			"	float t = mod(x + 7.0 / 252.0, period);\n"
			"	if (0.0 <= t && t < len) {\n"
			"		if (x < 0.12) {\n"
			"			v = (1.87862631683169E+00 * x + 6.81498517051705E-01);\n"
			"		} else if (x < 0.73) {\n"
			"			v -= 26.0 / 252.0; \n"
			"		} else {\n"
			"			v = -1.53215278202992E+00 * x + 1.98649818445446E+00;\n"
			"		}\n"
			"	}\n"
			"	return v;\n"
			"}\n"
			"\n"
			"// H1 - H2 = 0\n"
			"// => [x=-0.8359672286003642,x=0.1151580585723306]\n"
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
			"			r *= 1.0 - s * f;\n"
			"			b *= 1.0 - s;\n"
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
			"	float s = 1.0;\n"
			"	float v = colormap_v(clamp(x, 0.0, 1.0));\n"
			"	return colormap_hsv2rgb(h, s, v);\n"
			"}\n"
		);
	}
};

} // namespace IDL
} // namespace colormap
