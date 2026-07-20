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

class CBPurples : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_CB-Purples.frag"
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
		return std::string("CB-Purples");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"	return (((-3.03160346735036E+02 * x + 7.95537661771755E+02) * x - 6.68077287777559E+02) * x - 9.83523581613554E+00) * x + 2.49241870138829E+02;\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"	if (x < 0.6238275468349457) {\n"
			"		return ((((-1.64962450015544E+03 * x + 3.91401450219750E+03) * x - 2.81559997409582E+03) * x + 5.71903768479824E+02) * x - 1.63510103061329E+02) * x + 2.52440721674553E+02;\n"
			"	} else {\n"
			"		return (8.00008172182588E+01 * x - 4.62535128626795E+02) * x + 3.83781070034667E+02;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"	return (((((1.42855146044492E+03 * x - 4.10835541903972E+03) * x + 4.43536620247364E+03) * x - 2.15825854188203E+03) * x + 3.66481133684515E+02) * x - 9.02285603303462E+01) * x + 2.53802694290353E+02;\n"
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
