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

class GreenWhiteExponential : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_Green-White_Exponential.frag"
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
		return std::string("Green-White_Exponential");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"    if (x < (5.33164074896858E-01 + 3.69558823529412E+01) / (6.61764705882353E-01 - 3.80845483226613E-01)) { // 133.451339048\n"
			"        return 3.80845483226613E-01 * x + 5.33164074896858E-01;\n"
			"    } else if(x < (2.21853643274093E+02 - 3.69558823529412E+01) / (1.86816585713397E+00 - 6.61764705882353E-01)) { // 153.263912861\n"
			"        return 6.61764705882353E-01 * x - 3.69558823529412E+01;\n"
			"    } else {\n"
			"        return 1.86816585713397E+00 * x - 2.21853643274093E+02;\n"
			"    }\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"    if (x < (8.74223522059742E+01 - 3.33294186729301E-01) / (1.34076340457443E+00 - 6.66705813270699E-01)) { // 129.201212393\n"
			"        return 6.66705813270699E-01 * x - 3.33294186729301E-01;\n"
			"    } else {\n"
			"        return 1.34076340457443E+00 * x - 8.74223522059742E+01;\n"
			"    }\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"    if (x < (4.92898927047827E+02 - 4.63219741480611E-01) / (2.93126567624928E+00 - 2.63081042553601E-01)) {\n"
			"        return 2.63081042553601E-01 * x - 4.63219741480611E-01;\n"
			"    } else {\n"
			"        return 2.93126567624928E+00 * x - 4.92898927047827E+02;\n"
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
