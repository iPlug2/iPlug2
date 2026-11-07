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

class Haze : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_Haze.frag"
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
		return std::string("Haze");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"    if (x < 0.0) {\n"
			"        return 167.0;\n"
			"    } else if (x < (2.54491177159840E+02 + 2.49117061281287E+02) / (1.94999353031535E+00 + 1.94987400471999E+00)) {\n"
			"        return -1.94987400471999E+00 * x + 2.54491177159840E+02;\n"
			"    } else if (x <= 255.0) {\n"
			"        return 1.94999353031535E+00 * x - 2.49117061281287E+02;\n"
			"    } else {\n"
			"        return 251.0;\n"
			"    }\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"    if (x < 0.0) {\n"
			"        return 112.0;\n"
			"    } else if (x < (2.13852573128775E+02 + 1.42633630462899E+02) / (1.31530121382008E+00 + 1.39181683887691E+00)) {\n"
			"        return -1.39181683887691E+00 * x + 2.13852573128775E+02;\n"
			"    } else if (x <= 255.0) {\n"
			"        return 1.31530121382008E+00 * x - 1.42633630462899E+02;\n"
			"    } else {\n"
			"        return 195.0;\n"
			"    }\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"    if (x < 0.0) {\n"
			"        return 255.0;\n"
			"    } else if (x <= 255.0) {\n"
			"        return -9.84241021836929E-01 * x + 2.52502692064968E+02;\n"
			"    } else {\n"
			"        return 0.0;\n"
			"    }\n"
			"}\n"
			"\n"
			"vec4 colormap(float x) {\n"
			"    float t = x * 255.0;\n"
			"    float r = colormap_red(t) / 255.0;\n"
			"    float g = colormap_green(t) / 255.0;\n"
			"    float b = colormap_blue(t) / 255.0;\n"
			"    return vec4(r, g, b, 1.0);\n"
			"}\n"
		);
	}
};

} // namespace IDL
} // namespace colormap
