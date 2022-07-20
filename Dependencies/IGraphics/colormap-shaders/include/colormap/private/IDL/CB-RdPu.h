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

class CBRdPu : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_CB-RdPu.frag"
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
		return std::string("CB-RdPu");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"	if (x < 0.4668049514293671) {\n"
			"		return -1.36007661451525E+01 * x + 2.54876081825334E+02;\n"
			"	} else {\n"
			"		return ((9.11043267377652E+02 * x - 2.27422817830303E+03) * x + 1.47691217772832E+03) * x - 3.80041369120933E+01;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"	return ((((-2.12978937384858E+03 * x + 5.05211767883971E+03) * x - 3.95843947196006E+03) * x + 9.49632208843715E+02) * x - 2.70366761763812E+02) * x + 2.48595803511253E+02;\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"	if (x < 0.2484821379184723) {\n"
			"		return ((1.12923860577866E+02 * x - 2.02431339810602E+02) * x - 1.60306874714734E+02) * x + 2.42581612831587E+02;\n"
			"	} else if (x < 0.5019654333591461) {\n"
			"		return (-2.24073120483401E+02 * x + 4.46032892337713E+01) * x + 1.94733826112356E+02;\n"
			"	} else if (x < 0.7505462467670441) {\n"
			"		return (-4.08932859712077E+02 * x + 3.70448937862306E+02) * x + 7.77495522761299E+01;\n"
			"	} else {\n"
			"		return (-1.99803137524475E+02 * x + 2.71497008797383E+02) * x + 3.42106616941255E+01;\n"
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
