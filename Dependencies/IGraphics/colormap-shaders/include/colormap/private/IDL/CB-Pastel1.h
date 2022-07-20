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

class CBPastel1 : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_CB-Pastel1.frag"
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
		return std::string("CB-Pastel1");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"	if (x < 0.122867923365625) {\n"
			"		return -5.81788489736069E+02 * x + 2.50471590909091E+02;\n"
			"	} else if (x < 0.2449046174927113) {\n"
			"		return 1.99984352773830E+02 * x + 1.54416785206258E+02;\n"
			"	} else if (x < 0.3729729104526915) {\n"
			"		return 1.43786086956516E+02 * x + 1.68180000000001E+02;\n"
			"	} else if (x < 0.5011116081610979) {\n"
			"		return 2.52012802275928E+02 * x + 1.27814366998585E+02;\n"
			"	} else if (x < 0.6239282365941264) {\n"
			"		return 7.85450500555661E+00 * x + 2.50164923989616E+02;\n"
			"	} else if (x < 0.7520403577351265) {\n"
			"		return -2.00555718475049E+02 * x + 3.80197947214058E+02;\n"
			"	} else if (x < 0.8796535309192707) {\n"
			"		return 1.86622408963526E+02 * x + 8.90243697479360E+01;\n"
			"	} else {\n"
			"		return -9.30674082313196E+01 * x + 3.35054505005547E+02;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"	if (x < 0.2498801528138394) {\n"
			"		return 2.21725710445469E+02 * x + 1.79002480158730E+02;\n"
			"	} else if (x < 0.3735167574956272) {\n"
			"		return -2.52975806451616E+02 * x + 2.97620967741935E+02;\n"
			"	} else if (x < 0.5007872003710714) {\n"
			"		return 1.09439266615749E+02 * x + 1.62252864782272E+02;\n"
			"	} else if (x < 0.6262274652716027) {\n"
			"		return 3.02956451612894E+02 * x + 6.53419354838611E+01;\n"
			"	} else if (x < 0.752848702686641) {\n"
			"		return -3.10470307917895E+02 * x + 4.49486620234600E+02;\n"
			"	} else if (x < 0.8827503622135592) {\n"
			"		return 2.27675070027963E+01 * x + 1.98608963585427E+02;\n"
			"	} else {\n"
			"		return 1.95678708265011E+02 * x + 4.59715380404256E+01;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"	if (x < 0.1232989588096424) {\n"
			"		return 4.29695747800585E+02 * x + 1.74153409090909E+02;\n"
			"	} else if (x < 0.2476314320040304) {\n"
			"		return -2.40499266862156E+02 * x + 2.56787756598238E+02;\n"
			"	} else if (x < 0.3742360961829455) {\n"
			"		return 2.41095161290329E+02 * x + 1.37529838709676E+02;\n"
			"	} else if (x < 0.4998594481260504) {\n"
			"		return -4.90936497326148E+02 * x + 4.11482508912633E+02;\n"
			"	} else if (x < 0.6256351261233096) {\n"
			"		return 2.96955882352941E+02 * x + 1.76470588235230E+01;\n"
			"	} else if (x < 0.7525509527474964) {\n"
			"		return -1.11771301446066E+02 * x + 2.73361142009640E+02;\n"
			"	} else if (x < 0.8785969154660433) {\n"
			"		return 3.73063712757765E+02 * x - 9.15019098547990E+01;\n"
			"	} else {\n"
			"		return 4.55448275862047E+01 * x + 1.96255172413811E+02;\n"
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
