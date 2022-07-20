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

class BlueWaves : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_Blue_Waves.frag"
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
		return std::string("Blue_Waves");
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
			"	const float a = 94.58052830612496;\n"
			"	const float b = 0.5059881077994055;\n"
			"	const float c = 0.5079410623689743;\n"
			"	const float d = 86.68342149719986;\n"
			"	float v = a * sin(2.0 * pi / b * x + 2.0 * pi * c) + d;\n"
			"	if (v < 0.0) {\n"
			"		return -v;\n"
			"	} else {\n"
			"		return v;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"	const float pi = 3.141592653589793238462643383279502884197169399;\n"
			"	const float a = 99.19361086687104;\n"
			"	const float b = 1.095358975873744;\n"
			"	const float c = 0.3679001899352902;\n"
			"	const float d = 0.7603616684267874;\n"
			"	const float e = 219.7852186508229;\n"
			"	const float f = 1.161240703555854;\n"
			"	float v = (a * x + b) * sin(2.0 * pi * (x / c + d)) + e * x + f;\n"
			"	if (v > 255.0) {\n"
			"		return 510.0 - v;\n"
			"	} else {\n"
			"		return v;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"	const float pi = 3.141592653589793238462643383279502884197169399;\n"
			"	const float a = 253.8552881642787;\n"
			"	const float b = 0.5059881077994055;\n"
			"	const float c = 0.5079410623689743;\n"
			"	const float d = 226.7149651787587;\n"
			"	float v = a * sin(2.0 * pi * (x / b + c)) + d;\n"
			"	if (v > 255.0) {\n"
			"		return 510.0 - v;\n"
			"	} else if (v < 0.0) {\n"
			"		return -v;\n"
			"	} else {\n"
			"		return v;\n"
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
