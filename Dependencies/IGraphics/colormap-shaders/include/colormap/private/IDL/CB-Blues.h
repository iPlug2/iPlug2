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

class CBBlues : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_CB-Blues.frag"
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
		return std::string("CB-Blues");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"	if (x < 0.8724578971287745) {\n"
			"		return ((((-2.98580898761749E+03 * x + 6.75014845489710E+03) * x - 4.96941610635258E+03) * x + 1.20190439358912E+03) * x - 2.94374708396149E+02) * x + 2.48449410219242E+02;\n"
			"	} else {\n"
			"		return 8.0;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"	if (x < 0.3725897611307026) {\n"
			"		return -1.30453729372935E+02 * x + 2.51073069306930E+02;\n"
			"	} else {\n"
			"		return (-4.97095598364922E+01 * x - 1.77638812495581E+02) * x + 2.75554584848896E+02;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"	if (x < 0.8782350698420436) {\n"
			"		return (((-1.66242968759033E+02 * x + 2.50865766027010E+02) * x - 1.82046165445353E+02) * x - 3.29698266187334E+01) * x + 2.53927912915449E+02;\n"
			"	} else {\n"
			"		return -3.85153281423831E+02 * x + 4.93849833147981E+02;\n"
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
