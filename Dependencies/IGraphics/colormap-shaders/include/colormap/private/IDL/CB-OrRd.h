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

class CBOrRd : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_CB-OrRd.frag"
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
		return std::string("CB-OrRd");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_erf(float x) {\n"
			"	// erf approximation formula\n"
			"	const float pi = 3.141592653589793238462643383279502884197169399;\n"
			"	const float a = -8.0 * (pi - 3.0) / (3.0 * pi * (pi - 4.0));\n"
			"	float v = 1.0 - exp(-x * x * (4.0 / pi + a * x * x) / (1.0 + a * x * x));\n"
			"	return sign(x) * sqrt(v);\n"
			"}\n"
			"\n"
			"float colormap_red(float x) {\n"
			"	return 127.0548665301906 * (1.0 - colormap_erf(2.926357498911938 * (x - 1.0)));\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"	if (x < 0.3619403852314316) {\n"
			"		return (-1.54198428391755E+02 * x - 1.02444772146395E+02) * x + 2.46537152765234E+02;\n"
			"	} else {\n"
			"		return (-9.36243338922068E+01 * x - 2.52981049073614E+02) * x + 2.93087053416795E+02;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"	if (x < 0.257177186368595) {\n"
			"		return -3.12872659968267E+02 * x + 2.36904283447911E+02;\n"
			"	} else if (x < 0.3782584246092673) {\n"
			"		return -2.10348217636022E+02 * x + 2.10537335834895E+02;\n"
			"	} else if (x < 0.4977365973504272) {\n"
			"		return -3.49969458128060E+02 * x + 2.63350246305405E+02;\n"
			"	} else if (x < 0.6243924786616242) {\n"
			"		return -1.32763025210105E+02 * x + 1.55238655462198E+02;\n"
			"	} else if (x < 0.7554357868699768) {\n"
			"		return -3.23593609804140E+02 * x + 2.74391837181314E+02;\n"
			"	} else {\n"
			"		return -2.41701581027554E+02 * x + 2.12527667984095E+02;\n"
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
