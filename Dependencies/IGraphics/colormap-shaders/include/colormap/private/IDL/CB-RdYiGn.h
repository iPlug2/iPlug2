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

class CBRdYiGn : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_CB-RdYiGn.frag"
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
		return std::string("CB-RdYiGn");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"	if (x < 0.09825118520770205) {\n"
			"		return 5.07556923076926E+02 * x + 1.64923076923077E+02;\n"
			"	} else if (x < 0.2009111350471108) {\n"
			"		return 2.86362637362647E+02 * x + 1.86655677655676E+02;\n"
			"	} else if (x < 0.2994418666360456) {\n"
			"		return 8.90415982485030E+01 * x + 2.26299671592774E+02;\n"
			"	} else if (x < 0.5001300871372223) {\n"
			"		return 9.81627851140242E+00 * x + 2.50023049219689E+02;\n"
			"	} else if (x < 0.9039205014705658) {\n"
			"		return ((-3.30848798119696E+01 * x - 5.65722561191396E+02) * x + 2.78046782759626E+02) * x + 2.61515979057614E+02;\n"
			"	} else {\n"
			"		return -2.53583846153761E+02 * x + 2.55396153846073E+02;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"	if (x < 0.1105575469849737) {\n"
			"		return 4.79433455433456E+02 * x + 3.65079365079361E-01;\n"
			"	} else if (x < 0.3151890079472769) {\n"
			"		return 6.25896582484846E+02 * x - 1.58275246854709E+01;\n"
			"	} else if (x < 0.4023888287265409) {\n"
			"		return 4.80700000000005E+02 * x + 2.99368421052611E+01;\n"
			"	} else if (x < 0.5007980763912201) {\n"
			"		return 3.22042124542111E+02 * x + 9.37789987790044E+01;\n"
			"	} else if (x < 0.9266376793384552) {\n"
			"		return ((-2.91150627193739E+02 * x + 2.73891595228739E+02) * x - 1.97954551648389E+02) * x + 3.22069054828072E+02;\n"
			"	} else {\n"
			"		return -4.70385384615211E+02 * x + 5.78034615384465E+02;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"	if (x < 0.1007720845701718) {\n"
			"		return 1.66813186813184E+01 * x + 3.72910052910053E+01;\n"
			"	} else if (x < 0.2891807195246389) {\n"
			"		return 2.86155895159223E+02 * x + 1.01354904806627E+01;\n"
			"	} else if (x < 0.4061884871072265) {\n"
			"		return 4.02182758620675E+02 * x - 2.34172413793071E+01;\n"
			"	} else if (x < 0.5018816861329155) {\n"
			"		return 5.35500000000025E+02 * x - 7.75691699604942E+01;\n"
			"	} else if (x < 0.604070194492165) {\n"
			"		return -5.10170329670400E+02 * x + 4.47233618233660E+02;\n"
			"	} else if (x < 0.7060918916718424) {\n"
			"		return -3.26878215654109E+02 * x + 3.36512315270959E+02;\n"
			"	} else if (x < 0.812819402403008) {\n"
			"		return -6.62557264957455E+01 * x + 1.52488888888906E+02;\n"
			"	} else {\n"
			"		return -2.16444081632622E+02 * x + 2.74564897959153E+02;\n"
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
