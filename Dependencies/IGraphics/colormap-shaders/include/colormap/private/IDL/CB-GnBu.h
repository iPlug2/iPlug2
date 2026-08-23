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

class CBGnBu : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_CB-GnBu.frag"
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
		return std::string("CB-GnBu");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"	float v = ((((-2.83671754639782E+03 * x + 6.51753994553536E+03) * x - 5.00110948171466E+03) * x + 1.30359712298773E+03) * x - 2.89958300810074E+02) * x + 2.48458039402758E+02;\n"
			"	if (v < 8.0) {\n"
			"		return 8.0;\n"
			"	} else {\n"
			"		return v;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"	return (((((-1.36304822155833E+03 * x + 4.37691418182849E+03) * x - 5.01802019417285E+03) * x + 2.39971481269598E+03) * x - 5.65401491984724E+02) * x - 1.48189675724133E+01) * x + 2.50507618187374E+02;\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"	if (x < 0.3756393599187693) {\n"
			"		return (9.62948273917718E+01 * x - 1.96136874142438E+02) * x + 2.41033490809633E+02;\n"
			"	} else if (x < 0.6215448666633865) {\n"
			"		return 1.21184043778803E+02 * x + 1.35422939068100E+02;\n"
			"	} else if (x < 0.8830064316178203) {\n"
			"		return -1.53052165744713E+02 * x + 3.05873047350666E+02;\n"
			"	} else {\n"
			"		return -3.49468965517114E+02 * x + 4.79310344827486E+02;\n"
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
