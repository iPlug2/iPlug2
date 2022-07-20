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

class RainbowBlack : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_Rainbow+Black.frag"
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
		return std::string("Rainbow+Black");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
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
			"	if (x < 0.0) {\n"
			"		return vec4(0.0, 0.0, 0.0, 1.0);\n"
			"	} else if (1.0 < x) {\n"
			"		return vec4(0.0, 0.0, 0.0, 1.0);\n"
			"	} else {\n"
			"		float h = clamp(-9.42274071356572E-01 * x + 8.74326827903982E-01, 0.0, 1.0);\n"
			"		float s = 1.0;\n"
			"		float v = clamp(4.90125513855204E+00 * x + 9.18879034690780E-03, 0.0, 1.0);\n"
			"		return colormap_hsv2rgb(h, s, v);\n"
			"	}\n"
			"}\n"
		);
	}
};

} // namespace IDL
} // namespace colormap
