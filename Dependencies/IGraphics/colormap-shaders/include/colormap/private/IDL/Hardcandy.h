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

class Hardcandy : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_Hardcandy.frag"
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
		return std::string("Hardcandy");
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
			"	float v = sin(2.0 * pi * (1.0 / (2.0 * 0.143365330321852)) * x + 1.55) * (322.0 * x) + 191.0 * x;\n"
			"	if (v < 0.0) {\n"
			"		v = -v;\n"
			"	} else if (v > 255.0) {\n"
			"		v = 255.0 - (v - 255.0);\n"
			"	}\n"
			"	return v;\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"	const float pi = 3.141592653589793238462643383279502884197169399;\n"
			"	float v = sin(2.0 * pi / 0.675 * (x + 0.015)) * 190.0 + 25.0;\n"
			"	if (v < 0.0) {\n"
			"		return -v;\n"
			"	} else {\n"
			"		return v;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"	const float pi = 3.141592653589793238462643383279502884197169399;\n"
			"	return sin(2.0 * pi * (x * -3.45 - 0.02)) * 127.5 + 127.5;\n"
			"}\n"
			"\n"
			"vec4 colormap(float x) {\n"
			"    float r = clamp(colormap_red(x) / 255.0, 0.0, 1.0);\n"
			"    float g = clamp(colormap_green(x) / 255.0, 0.0, 1.0);\n"
			"    float b = clamp(colormap_blue(x) / 255.0, 0.0, 1.0);\n"
			"    return vec4(r, g, b, 1.0);\n"
			"}\n"
		);
	}
};

} // namespace IDL
} // namespace colormap
