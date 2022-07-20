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

class RedPurple : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_Red-Purple.frag"
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
		return std::string("Red-Purple");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"    if (x < 25.97288868211422) {\n"
			"        return 3.07931034482759E+00 * x - 1.62758620689655E+00;\n"
			"    } else if(x < 154.7883608200706) {\n"
			"        return (-0.002335409922053 * x + 1.770196213987500) * x + 33.949335775363600;\n"
			"    } else {\n"
			"        return 252.0;\n"
			"    }\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"    return ((7.125813968310300E-05 * x - 2.223039020276470E-02) * x + 2.367815929630070E+00) * x - 7.739188304766140E+01;\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"    if (x < (2.51577880184332E+01 - 5.67741935483871E-01) / (9.88497695852535E-01 - 1.70189098998888E-01)) { // 30.0498444933\n"
			"        return 1.70189098998888E-01 * x - 5.67741935483871E-01;\n"
			"    } else if(x < 150.2124460352976) {\n"
			"        return 9.88497695852535E-01 * x - 2.51577880184332E+01;\n"
			"    } else {\n"
			"        return (-3.85393764961783E-03 * x + 2.82261880442729E+00) * x - 2.13706208872841E+02;\n"
			"    }\n"
			"}\n"
			"\n"
			"vec4 colormap(float x) {\n"
			"    float t = x * 255.0;\n"
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
