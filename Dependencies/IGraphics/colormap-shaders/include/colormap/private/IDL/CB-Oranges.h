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

class CBOranges : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_CB-Oranges.frag"
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
		return std::string("CB-Oranges");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"	if (x < 0.4798405468463898) {\n"
			"		return 255.0;\n"
			"	} else if (x < 0.7629524491886985) {\n"
			"		return (-3.67617638383468E+02 * x + 3.17332748024787E+02) * x + 1.85373720793787E+02;\n"
			"	} else {\n"
			"		return (3.68357233392831E+02 * x - 1.00617951362078E+03) * x + 7.66695019519326E+02;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"	if (x < 0.748539247687408) {\n"
			"		return (((-8.92644295264035E+01 * x + 3.93421870424412E+02) * x - 4.73834129104390E+02) * x - 5.60962544745416E+01) * x + 2.43354168263028E+02;\n"
			"	} else {\n"
			"		return (1.06683260838348E+02 * x - 3.18020138166420E+02) * x + 2.51126712492908E+02;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"	if (x < 0.76) {\n"
			"		return ((7.32034492098544E+02 * x - 7.55283914444663E+02) * x - 1.53168890861198E+02) * x + 2.33567667053916E+02;\n"
			"	} else {\n"
			"		return 1.23702752385982E+01 * x - 8.09423081765692E+00;\n"
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
