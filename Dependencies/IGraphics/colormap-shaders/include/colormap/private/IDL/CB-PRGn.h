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

class CBPRGn : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_CB-PRGn.frag"
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
		return std::string("CB-PRGn");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"	if (x < 0.09963276982307434) {\n"
			"		return 5.56064615384614E+02 * x + 6.34200000000000E+01;\n"
			"	} else if (x < 0.4070911109447479) {\n"
			"		return ((-1.64134573262743E+03 * x + 1.26126075878571E+03) * x + 8.30228593437549E+01) * x + 9.96536529647110E+01;\n"
			"	} else if (x < 0.5013306438922882) {\n"
			"		return 1.64123076923114E+02 * x + 1.64926153846145E+02;\n"
			"	} else if (x < 0.9049346148967743) {\n"
			"		return ((((-4.17783076764345E+04 * x + 1.55735420068591E+05) * x - 2.27018068370619E+05) * x + 1.61149115838926E+05) * x - 5.60588504546212E+04) * x + 7.93919652573346E+03;\n"
			"	} else {\n"
			"		return -2.67676923076906E+02 * x + 2.68590769230752E+02;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"	if (x < 0.1011035144329071) {\n"
			"		return 4.30627692307691E+02 * x - 1.56923076923038E-01;\n"
			"	} else if (x < 0.5013851821422577) {\n"
			"		return ((2.21240993583109E+02 * x - 7.23499016773187E+02) * x + 8.74292145995924E+02) * x - 3.78460594811949E+01;\n"
			"	} else {\n"
			"		return ((((-8.82260255008935E+03 * x + 3.69735516719018E+04) * x - 5.94940784200438E+04) * x + 4.54236515662453E+04) * x - 1.66043372157228E+04) * x + 2.59449114260420E+03;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"	if (x < 0.50031378865242) {\n"
			"		return ((((1.32543265346288E+04 * x - 1.82876583834065E+04) * x + 9.17087085537958E+03) * x - 2.45909850441496E+03) * x + 7.42893247681885E+02) * x + 7.26668497072812E+01;\n"
			"	} else if (x < 0.609173446893692) {\n"
			"		return -3.50141636141726E+02 * x + 4.22147741147797E+02;\n"
			"	} else {\n"
			"		return ((1.79776073728688E+03 * x - 3.80142452792079E+03) * x + 2.10214624189039E+03) * x - 6.74426111651015E+01;\n"
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
