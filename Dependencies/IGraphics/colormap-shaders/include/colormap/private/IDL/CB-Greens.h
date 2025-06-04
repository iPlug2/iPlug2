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

class CBGreens : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_CB-Greens.frag"
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
		return std::string("CB-Greens");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"	if (x < 0.6193682068820651) {\n"
			"		return ((1.41021531432983E+02 * x - 3.78122271460656E+02) * x - 1.08403692154170E+02) * x + 2.45743977533647E+02;\n"
			"	} else {\n"
			"		return ((-8.63146749682724E+02 * x + 1.76195389457266E+03) * x - 1.43807716183136E+03) * x + 4.86922446232568E+02;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"	return (-1.37013460576160E+02 * x - 4.54698187198101E+01) * x + 2.52098684286706E+02;\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"	if (x < 0.5062477983469252) {\n"
			"		return ((3.95067226937040E+02 * x - 4.52381961582927E+02) * x - 1.25304923569201E+02) * x + 2.43770002412197E+02;\n"
			"	} else {\n"
			"		return ((2.98249378459208E+02 * x - 6.14859580726999E+02) * x + 2.22299590241459E+02) * x + 1.21998454489668E+02;\n"
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
