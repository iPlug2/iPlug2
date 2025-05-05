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

class Steps : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_Steps.frag"
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
		return std::string("Steps");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_erf(float x) {\n"
			"    // erf approximation formula\n"
			"    const float pi = 3.141592653589793238462643383279502884197169399;\n"
			"    const float a = -8.0 * (pi - 3.0) / (3.0 * pi * (pi - 4.0));\n"
			"    float v = 1.0 - exp(-x * x * (4.0 / pi + a * x * x) / (1.0 + a * x * x));\n"
			"    return sign(x) * sqrt(v);\n"
			"}\n"
			"\n"
			"float colormap_red(float x) {\n"
			"    if (x <= 95.5) {\n"
			"        return 8.14475806451613E+00 * x - 5.23967741935484E+02;\n"
			"    } else {\n"
			"        return colormap_erf((x - 145.0) * 0.028) * 131.0 + 125.0;\n"
			"    }\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"    if (x < (3.14410256410256E+02 + 2.14285714285714E-01) / (4.25000000000000E+01 + 9.81196581196581E+00)) {\n"
			"        return 4.25000000000000E+01 * x - 2.14285714285714E-01;\n"
			"    } else if (x < 192.0) { // actual root: 193.529143410603\n"
			"        return -9.81196581196581E+00 * x + 3.14410256410256E+02;\n"
			"    } else {\n"
			"        return ((5.35129859215999E-04 * x - 2.98599683017528E-01) * x + 5.69466901216655E+01) * x - 3.71604038989543E+03;\n"
			"    }\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"    if (x < 63.0) {\n"
			"        return 8.22620967741936E+00 * x - 2.63729032258065E+02;\n"
			"    } else if (x <= 95.5) {\n"
			"        return 4.97690615835777E+00 * x - 3.16414039589443E+02;\n"
			"    } else {\n"
			"        return (((-7.88871743679920E-05 * x + 7.21525684930384E-02) * x - 2.45956037640571E+01) * x + 3.70824713134765E+03) * x - 2.08852518066406E+05;\n"
			"    }\n"
			"}\n"
			"\n"
			"vec4 colormap(float x) {\n"
			"    float t = x * 255.0 - 0.5;\n"
			"    float r = clamp(colormap_red(t) / 255.0, 0.0, 1.0);\n"
			"    float g = clamp(colormap_green(t) / 255.0, 0.0, 1.0);\n"
			"    float b = clamp(colormap_blue(t) / 255.0, 0.0, 1.0);\n"
			"    return vec4(r, g, b, 1.0);\n"
			"}\n"
		);
	}
};

} // namespace IDL
} // namespace colormap
