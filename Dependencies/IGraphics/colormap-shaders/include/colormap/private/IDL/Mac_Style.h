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

class MacStyle : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_Mac_Style.frag"
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
		return std::string("Mac_Style");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_h(float x) {\n"
			"	return -7.44992704834645E-01 * x + 7.47986634976377E-01;\n"
			"}\n"
			"\n"
			"float colormap_s(float x) {\n"
			"	return 1.0;\n"
			"}\n"
			"\n"
			"float colormap_v(float x) {\n"
			"	float i = mod(mod(x * 256.0, 2.0) + 2.0, 2.0);\n"
			"	if (0.0 <= i && i < 1.0) {\n"
			"		return 1.0;\n"
			"	} else {\n"
			"		return 254.0 / 255.0;\n"
			"	}\n"
			"}\n"
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
