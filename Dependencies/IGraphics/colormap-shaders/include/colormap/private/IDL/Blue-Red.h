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

class BlueRed : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_Blue-Red.frag"
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
		return std::string("Blue-Red");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"    return 4.04377880184332E+00 * x - 5.17956989247312E+02;\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"    if (x < (5.14022177419355E+02 + 1.13519230769231E+01) / (4.20313644688645E+00 + 4.04233870967742E+00)) {\n"
			"        return 4.20313644688645E+00 * x - 1.13519230769231E+01;\n"
			"    } else {\n"
			"        return -4.04233870967742E+00 * x + 5.14022177419355E+02;\n"
			"    }\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"    if (x < 1.34071303331385E+01 / (4.25125657510228E+00 - 1.0)) { // 4.12367649967\n"
			"        return x;\n"
			"    } else if (x < (255.0 + 1.34071303331385E+01) / 4.25125657510228E+00) { // 63.1359518278\n"
			"        return 4.25125657510228E+00 * x - 1.34071303331385E+01;\n"
			"    } else if (x < (1.04455240613432E+03 - 255.0) / 4.11010047593866E+00) { // 192.100512082\n"
			"        return 255.0;\n"
			"    } else {\n"
			"        return -4.11010047593866E+00 * x + 1.04455240613432E+03;\n"
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
