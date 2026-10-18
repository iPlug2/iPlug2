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

class CBBuPu : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_CB-BuPu.frag"
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
		return std::string("CB-BuPu");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"	if (x < 0.5) {\n"
			"		return ((9.36778020337806E+02 * x - 6.50118321723071E+02) * x - 1.17443717298418E+02) * x + 2.45640353186226E+02;\n"
			"	} else if (x < 0.8774830563107102) {\n"
			"		return (-1.07698286837105E+02 * x + 1.18941048271099E+02) * x + 1.07226580391914E+02;\n"
			"	} else {\n"
			"		return (-9.74524977247347E+01 * x - 2.28433367883516E+02) * x + 4.04152727778340E+02;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"	if (x < 0.8733318961770479) {\n"
			"		return ((5.48640317293175E+01 * x - 2.50041384768192E+02) * x - 9.10862643329019E+01) * x + 2.50330566129102E+02;\n"
			"	} else {\n"
			"		return (1.99199516170089E+02 * x - 4.99460567863480E+02) * x + 3.00881779984708E+02;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"	return ((((-5.85479883993044E+02 * x + 1.17558327595144E+03) * x - 8.53086782991886E+02) * x + 1.82921150609850E+02) * x - 9.97610091178212E+01) * x + 2.53898307388663E+02;\n"
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
