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

class CBPuBuGn : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_CB-PuBuGn.frag"
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
		return std::string("CB-PuBuGn");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"	if (x < 0.4874515235424042) {\n"
			"		return ((-4.67299169932026E+02 * x - 9.61558801325737E+01) * x - 1.42239969624745E+02) * x + 2.55174176214321E+02;\n"
			"	} else if (x < 0.7547346238979944) {\n"
			"		return -4.01060227272736E+02 * x + 3.04365078671334E+02;\n"
			"	} else {\n"
			"		return -4.15773809523774E+00 * x + 4.80902777777746E+0;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"	if (x < 0.6119840443134308) {\n"
			"		return ((-1.86575654193875E+02 * x + 1.34788465541787E+02) * x - 1.76719831637092E+02) * x + 2.46604381236178E+02;\n"
			"	} else {\n"
			"		return ((-8.15961797068245E+02 * x + 1.63871690240040E+03) * x - 1.21797539379991E+03) * x + 4.64835362086844E+02;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"	if (x < 0.626910001039505) {\n"
			"		return (((1.51117885576561E+02 * x - 3.02884959951043E+02) * x + 1.53322134227491E+02) * x - 1.09362189530744E+02) * x + 2.51508758745276E+02;\n"
			"	} else {\n"
			"		return ((6.51665430892026E+02 * x - 1.29049314879284E+03) * x + 4.20233542130992E+02) * x + 2.75096926634035E+02;\n"
			"	}\n"
			"}\n"
			"\n"
			"vec4 colormap(float x) {\n"
			"	float r = clamp(colormap_red(x) / 255.0, 0.0, 1.0);\n"
			"	float g = clamp(colormap_green(x) / 255.0, 0.0, 1.0);\n"
			"	float b = clamp(colormap_blue(x) / 255.0, 0.0, 1.0);\n"
			"	return vec4(r, g, b, 1.0);\n"
			"}\n"
		);
	}
};

} // namespace IDL
} // namespace colormap
