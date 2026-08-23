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

class CBPuRd : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_CB-PuRd.frag"
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
		return std::string("CB-PuRd");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"	if (x < 25.0 / 254.0) {\n"
			"		return -1.41567647058826E+02 * x + 2.46492647058824E+02;\n"
			"	} else if (x < 0.3715468440779919) {\n"
			"		return (1.64817020395145E+02 * x - 2.01032852327719E+02) * x + 2.52820173539371E+02;\n"
			"	} else if (x < 0.6232413065898157) {\n"
			"		return (((2.61012828741073E+04 * x - 5.18905872811356E+04) * x + 3.78968358931486E+04) * x - 1.19124127524292E+04) * x + 1.55945779375675E+03;\n"
			"	} else if (x < 0.7481208809057023) {\n"
			"		return -2.02469919786095E+02 * x + 3.57739416221033E+02;\n"
			"	} else {\n"
			"		return -4.08324020737294E+02 * x + 5.11743167562695E+02;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"	if (x < 0.1303350956955242) {\n"
			"		return -1.59734759358287E+02 * x + 2.44376470588235E+02;\n"
			"	} else if (x < 0.6227215280200861) {\n"
			"		return (((1.21347373400442E+03 * x - 2.42854832541048E+03) * x + 1.42039752537243E+03) * x - 6.27806679597789E+02) * x + 2.86280758506240E+02;\n"
			"	} else {\n"
			"		return (1.61877993987291E+02 * x - 4.06294499392671E+02) * x + 2.32401278080262E+02;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"	if (x < 0.7508644163608551) {\n"
			"		return ((((2.96852143551409E+03 * x - 6.12155011029541E+03) * x + 4.21719423212110E+03) * x - 1.29520280960574E+03) * x + 2.78723913454450E+01) * x + 2.47133504519275E+02;\n"
			"	} else {\n"
			"		return ((-6.55064010825706E+02 * x + 1.23635622822904E+03) * x - 8.68481725874416E+02) * x + 3.18158180572088E+02;\n"
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
