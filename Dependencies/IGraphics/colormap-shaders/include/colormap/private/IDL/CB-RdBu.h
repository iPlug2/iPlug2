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

class CBRdBu : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_CB-RdBu.frag"
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
		return std::string("CB-RdBu");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"	if (x < 0.09771832105856419) {\n"
			"		return 7.60263247863246E+02 * x + 1.02931623931624E+02;\n"
			"	} else if (x < 0.3017162107441106) {\n"
			"		return (-2.54380938558548E+02 * x + 4.29911571188803E+02) * x + 1.37642085716717E+02;\n"
			"	} else if (x < 0.4014205790737471) {\n"
			"		return 8.67103448276151E+01 * x + 2.18034482758611E+02;\n"
			"	} else if (x < 0.5019932233215039) {\n"
			"		return -6.15461538461498E+01 * x + 2.77547692307680E+02;\n"
			"	} else if (x < 0.5969483882550937) {\n"
			"		return -3.77588522588624E+02 * x + 4.36198819698878E+02;\n"
			"	} else if (x < 0.8046060096654594) {\n"
			"		return (-6.51345897546620E+02 * x + 2.09780968434337E+02) * x + 3.17674951640855E+02;\n"
			"	} else {\n"
			"		return -3.08431855203590E+02 * x + 3.12956742081421E+02;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"	if (x < 0.09881640500975222) {\n"
			"		return 2.41408547008547E+02 * x + 3.50427350427364E-01;\n"
			"	} else if (x < 0.5000816285610199) {\n"
			"		return ((((1.98531871433258E+04 * x - 2.64108262469187E+04) * x + 1.10991785969817E+04) * x - 1.92958444776211E+03) * x + 8.39569642882186E+02) * x - 4.82944517518776E+01;\n"
			"	} else if (x < 0.8922355473041534) {\n"
			"		return (((6.16712686949223E+03 * x - 1.59084026055125E+04) * x + 1.45172137257997E+04) * x - 5.80944127411621E+03) * x + 1.12477959061948E+03;\n"
			"	} else {\n"
			"		return -5.28313797313699E+02 * x + 5.78459299959206E+02;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"	if (x < 0.1033699568661857) {\n"
			"		return 1.30256410256410E+02 * x + 3.08518518518519E+01;\n"
			"	} else if (x < 0.2037526071071625) {\n"
			"		return 3.38458128078815E+02 * x + 9.33004926108412E+00;\n"
			"	} else if (x < 0.2973267734050751) {\n"
			"		return (-1.06345054944861E+02 * x + 5.93327252747168E+02) * x - 3.81852747252658E+01;\n"
			"	} else if (x < 0.4029109179973602) {\n"
			"		return 6.68959706959723E+02 * x - 7.00740740740798E+01;\n"
			"	} else if (x < 0.5006715489526758) {\n"
			"		return 4.87348695652202E+02 * x + 3.09898550724286E+00;\n"
			"	} else if (x < 0.6004396902588283) {\n"
			"		return -6.85799999999829E+01 * x + 2.81436666666663E+02;\n"
			"	} else if (x < 0.702576607465744) {\n"
			"		return -1.81331701891043E+02 * x + 3.49137263626287E+02;\n"
			"	} else if (x < 0.9010407030582428) {\n"
			"		return (2.06124143164576E+02 * x - 5.78166906665595E+02) * x + 5.26198653917172E+02;\n"
			"	} else {\n"
			"		return -7.36990769230737E+02 * x + 8.36652307692262E+02;\n"
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
