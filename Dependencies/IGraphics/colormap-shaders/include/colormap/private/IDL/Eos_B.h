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

class EosB : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_Eos_B.frag"
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
		return std::string("Eos_B");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_h(float x) {\n"
			"	if (x < 0.1167535483837128) {\n"
			"		return 2.0 / 3.0; // H1\n"
			"	} else if (x < 0.1767823398113251) {\n"
			"		return ((-3.19659402385354E+02 * x + 1.14469539590179E+02) * x - 1.52210982227697E+01) * x + 1.39214703883044E+00; // H2\n"
			"	} else if (x < 0.2266354262828827) {\n"
			"		return ((-3.55166097640991E+02 * x + 2.51218596935272E+02) * x - 6.08853752315044E+01) * x + 5.38727123476564E+00; // H3\n"
			"	} else if (x < (6.95053970124612E-01 - 4.13725796136428E-01) / (1.48914458632691E+00 - 6.97458630656247E-01)) {\n"
			"		return -1.48914458632691E+00 * x + 6.95053970124612E-01; // H4\n"
			"	} else if (x < (4.13725796136428E-01 - 2.48329223043123E-01) / (6.97458630656247E-01 - 3.48617475202321E-01)) {\n"
			"		return -6.97458630656247E-01 * x + 4.13725796136428E-01; // H5\n"
			"	} else {\n"
			"		return -3.48617475202321E-01 * x + 2.48329223043123E-01; // H6\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_v(float x) {\n"
			"	float v = 1.0;\n"
			"	if (x < 0.115834504365921) {\n"
			"		v = 4.18575376272140E+00 * x + 5.15145240089963E-01; // V1-Hi\n"
			"	} else if (x < (1.90980360972022E+00 + 9.13724751363001E-01) / (7.87450639585523E+00 + 7.87450803534638E+00)) {\n"
			"		v = -7.87450803534638E+00 * x + 1.90980360972022E+00; // V2-Hi\n"
			"	} else if (x < 0.5) {\n"
			"		v = 7.87450639585523E+00 * x - 9.13724751363001E-01; // V3-Hi\n"
			"	} else {\n"
			"		v = -1.87540494049556E+00 * x + 2.33603077812338E+00; // V4-Hi\n"
			"	}\n"
			"	v = clamp(v, 0.0, 1.0);\n"
			"\n"
			"	float period = 4.0 / 105.0;\n"
			"	float len = 3.0 / 252.0;\n"
			"	float t = mod(x + 7.0 / 252.0, period);\n"
			"	if (0.0 <= t && t < len) {\n"
			"		if (x < 0.115834504365921) {\n"
			"			v = 3.74113124408467E+00 * x + 4.64654322955584E-01; // V1-Lo\n"
			"		} else if (x < (1.90980360972022E+00 + 9.13724751363001E-01) / (7.87450639585523E+00 + 7.87450803534638E+00)) {\n"
			"			v = -3.97326878048783E+00 * x + 1.25308500609757E+00; // V2-Lo\n"
			"		} else if (x < 0.25) {\n"
			"			v = 6.99297032967038E+00 * x - 8.03946549450558E-01; // V3-Lo\n"
			"		} else if (x < 0.72) {\n"
			"			v -= 26.0 / 255.0;\n"
			"		} else {\n"
			"			v = -1.67870020621040E+00 * x + 2.09414636280895E+00; // V4-Lo\n"
			"		}\n"
			"	}\n"
			"\n"
			"	return v;\n"
			"}\n"
			"\n"
			"// H1 - H2 = 0\n"
			"// => [x=0.1167535483837128]\n"
			"\n"
			"// H2 - H3 = 0\n"
			"// => [x=0.1767823398113251,x=0.1822494566440582,x=3.492328017950058]\n"
			"\n"
			"// H3 - H4 = 0\n"
			"// => [x=0.2266354262828827]\n"
			"\n"
			"// V1-Hi - 1 = 0\n"
			"// => [x=0.115834504365921]\n"
			"\n"
			"// V2-Hi - V3-Hi = 0\n"
			"// => \n"
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
			"	float s = 1.0;\n"
			"	float v = colormap_v(clamp(x, 0.0, 1.0));\n"
			"	return colormap_hsv2rgb(h, s, v);\n"
			"}\n"
		);
	}
};

} // namespace IDL
} // namespace colormap
