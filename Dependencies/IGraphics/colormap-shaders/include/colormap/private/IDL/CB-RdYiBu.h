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

class CBRdYiBu : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_CB-RdYiBu.frag"
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
		return std::string("CB-RdYiBu");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"	if (x < 0.09790863520700754) {\n"
			"		return 5.14512820512820E+02 * x + 1.64641025641026E+02;\n"
			"	} else if (x < 0.2001887081633112) {\n"
			"		return 2.83195402298854E+02 * x + 1.87288998357964E+02;\n"
			"	} else if (x < 0.3190117539655621) {\n"
			"		return 9.27301587301214E+01 * x + 2.25417989417999E+02;\n"
			"	} else if (x < 0.500517389125164) {\n"
			"		return 255.0;\n"
			"	} else if (x < 0.6068377196788788) {\n"
			"		return -3.04674876847379E+02 * x + 4.07495073891681E+02;\n"
			"	} else if (x < 0.9017468988895416) {\n"
			"		return (1.55336390191951E+02 * x - 7.56394659038288E+02) * x + 6.24412733169483E+02;\n"
			"	} else {\n"
			"		return -1.88350769230735E+02 * x + 2.38492307692292E+02;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"	if (x < 0.09638568758964539) {\n"
			"		return 4.81427692307692E+02 * x + 4.61538461538488E-01;\n"
			"	} else if (x < 0.4987066686153412) {\n"
			"		return ((((3.25545903568267E+04 * x - 4.24067109461319E+04) * x + 1.83751375886345E+04) * x - 3.19145329617892E+03) * x + 8.08315127034676E+02) * x - 1.44611527812961E+01;\n"
			"	} else if (x < 0.6047312345537269) {\n"
			"		return -1.18449917898218E+02 * x + 3.14234811165860E+02;\n"
			"	} else if (x < 0.7067635953426361) {\n"
			"		return -2.70822112753102E+02 * x + 4.06379036672115E+02;\n"
			"	} else {\n"
			"		return (-4.62308723214883E+02 * x + 2.42936159122279E+02) * x + 2.74203431802418E+02;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"	if (x < 0.09982818011951204) {\n"
			"		return 1.64123076923076E+01 * x + 3.72646153846154E+01;\n"
			"	} else if (x < 0.2958717460833126) {\n"
			"		return 2.87014675052409E+02 * x + 1.02508735150248E+01;\n"
			"	} else if (x < 0.4900527540014758) {\n"
			"		return 4.65475113122167E+02 * x - 4.25505279034673E+01;\n"
			"	} else if (x < 0.6017014681258838) {\n"
			"		return 5.61032967032998E+02 * x - 8.93789173789407E+01;\n"
			"	} else if (x < 0.7015737100463595) {\n"
			"		return -1.51655677655728E+02 * x + 3.39446886446912E+02;\n"
			"	} else if (x < 0.8237156500567735) {\n"
			"		return -2.43405347593559E+02 * x + 4.03816042780725E+02;\n"
			"	} else {\n"
			"		return -3.00296889157305E+02 * x + 4.50678495922638E+02;\n"
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
