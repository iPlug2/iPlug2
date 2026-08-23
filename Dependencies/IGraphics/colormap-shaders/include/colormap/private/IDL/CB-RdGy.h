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

class CBRdGy : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_CB-RdGy.frag"
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
		return std::string("CB-RdGy");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_f(float x) {\n"
			"	if (x < 0.8110263645648956) {\n"
			"		return (((4.41347880412638E+03 * x - 1.18250308887283E+04) * x + 1.13092070303101E+04) * x - 4.94879610401395E+03) * x + 1.10376673162241E+03;\n"
			"	} else {\n"
			"		return (4.44045986053970E+02 * x - 1.34196160353499E+03) * x + 9.26518306556645E+02;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_red(float x) {\n"
			"	if (x < 0.09384074807167053) {\n"
			"		return 7.56664615384615E+02 * x + 1.05870769230769E+02;\n"
			"	} else if (x < 0.3011957705020905) {\n"
			"		return (-2.97052932130813E+02 * x + 4.43575866219751E+02) * x + 1.37867123966178E+02;\n"
			"	} else if (x < 0.3963058760920129) {\n"
			"		return 8.61868131868288E+01 * x + 2.18562881562874E+02;\n"
			"	} else if (x < 0.5) {\n"
			"		return 2.19915384615048E+01 * x + 2.44003846153861E+02;\n"
			"	} else {\n"
			"		return colormap_f(x);\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"	if (x < 0.09568486400411116) {\n"
			"		return 2.40631111111111E+02 * x + 1.26495726495727E+00;\n"
			"	} else if (x < 0.2945883673263987) {\n"
			"		return 7.00971783488427E+02 * x - 4.27826773670273E+01;\n"
			"	} else if (x < 0.3971604611945229) {\n"
			"		return 5.31775726495706E+02 * x + 7.06051282052287E+00;\n"
			"	} else if (x < 0.5) {\n"
			"		return 3.64925470085438E+02 * x + 7.33268376068493E+01;\n"
			"	} else {\n"
			"		return colormap_f(x);\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"	if (x < 0.09892375498249567) {\n"
			"		return 1.30670329670329E+02 * x + 3.12116402116402E+01;\n"
			"	} else if (x < 0.1985468629735229) {\n"
			"		return 3.33268034188035E+02 * x + 1.11699145299146E+01;\n"
			"	} else if (x < 0.2928770209555256) {\n"
			"		return 5.36891330891336E+02 * x - 2.92588522588527E+01;\n"
			"	} else if (x < 0.4061551302245808) {\n"
			"		return 6.60915763546766E+02 * x - 6.55827586206742E+01;\n"
			"	} else if (x < 0.5) {\n"
			"		return 5.64285714285700E+02 * x - 2.63359683794383E+01;\n"
			"	} else {\n"
			"		return colormap_f(x);\n"
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
