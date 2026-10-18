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

class CBPuBu : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_CB-PuBu.frag"
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
		return std::string("CB-PuBu");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"	if (x < 0.7520372909206926) {\n"
			"		return (((9.68615208861418E+02 * x - 1.16097242960380E+03) * x + 1.06173672031378E+02) * x - 1.68616613530379E+02) * x + 2.56073136099945E+02;\n"
			"	} else {\n"
			"		return -1.20830453148990E+01 * x + 1.44337397593436E+01;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"	if (x < 0.7485333535031721) {\n"
			"		return (((-4.58537247030064E+02 * x + 5.67323181593790E+02) * x - 2.56714665792882E+02) * x - 1.14205365680507E+02) * x + 2.47073841488433E+02;\n"
			"	} else {\n"
			"		return ((-2.99774273328017E+02 * x + 4.12147041403012E+02) * x - 2.49880079288168E+02) * x + 1.93578601034431E+02;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"	if (x < 0.7628468501376879) {\n"
			"		return ((-5.44257972228224E+01 * x + 2.70890554876532E+01) * x - 9.12766750739247E+01) * x + 2.52166182860177E+02;\n"
			"	} else {\n"
			"		return (((4.55621137729287E+04 * x - 1.59960900638524E+05) * x + 2.09530452721547E+05) * x - 1.21704642900945E+05) * x + 2.66644674068694E+04;\n"
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
