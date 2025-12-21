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

class Plasma : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_Plasma.frag"
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
		return std::string("Plasma");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"	const float pi = 3.141592653589793238462643383279502884197169399;\n"
			"	const float a = 12.16378802377247;\n"
			"	const float b = 0.05245257017955226;\n"
			"	const float c = 0.2532139106569052;\n"
			"	const float d = 0.02076964056039702;\n"
			"	const float e = 270.124167081014;\n"
			"	const float f = 1.724941960305955;\n"
			"	float v = (a * x + b) * sin(2.0 * pi / c * (x - d)) + e * x + f;\n"
			"	if (v > 255.0) {\n"
			"		return 255.0 - (v - 255.0);\n"
			"	} else {\n"
			"		return v;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"	const float pi = 3.141592653589793238462643383279502884197169399;\n"
			"	const float a = 88.08537391182792;\n"
			"	const float b = 0.25280516046667;\n"
			"	const float c = 0.05956080245692388;\n"
			"	const float d = 106.5684078925541;\n"
			"	return a * sin(2.0 * pi / b * (x - c)) + d;\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"	const float pi = 3.141592653589793238462643383279502884197169399;\n"
			"	const float a = 63.89922420106684;\n"
			"	const float b = 0.4259605778503662;\n"
			"	const float c = 0.2529247343450655;\n"
			"	const float d = 0.5150868195804643;\n"
			"	const float e = 938.1798072557968;\n"
			"	const float f = 503.0883490697431;\n"
			"	float v = (a * x + b) * sin(2.0 * pi / c * x + d * 2.0 * pi) - e * x + f;\n"
			"	if (v > 255.0) {\n"
			"		return 255.0 - (v - 255.0);\n"
			"	} else {\n"
			"		return mod(v, 255.0);\n"
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
