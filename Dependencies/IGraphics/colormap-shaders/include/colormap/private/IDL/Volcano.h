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

class Volcano : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_Volcano.frag"
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
		return std::string("Volcano");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_r1(float x) {\n"
			"	const float pi = 3.141592653589793238462643383279502884197169399;\n"
			"	const float a = 216.6901575438631;\n"
			"	const float b = 1.073972444219095;\n"
			"	const float c = 0.6275803332110022;\n"
			"	const float d = 221.6241814852619;\n"
			"	float v = a * sin(2.0 * pi / b * x + 2.0 * pi * c) + d;\n"
			"	if (v < 0.0) {\n"
			"		return -v;\n"
			"	} else {\n"
			"		return v;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_r2(float x) {\n"
			"	const float pi = 3.141592653589793238462643383279502884197169399;\n"
			"	const float a = 202.3454274460618;\n"
			"	const float b = 1.058678309228987;\n"
			"	const float c = 0.4891299991060677;\n"
			"	const float d = -72.38173481234448;\n"
			"	return a * sin(2.0 * pi / b * x + 2.0 * pi * c) + d;\n"
			"}\n"
			"\n"
			"float colormap_red(float x) {\n"
			"	if (x < 0.4264009656413063) {\n"
			"		return colormap_r1(x);\n"
			"	} else if (x < 0.6024851624202665) {\n"
			"		const float a = (0.0 - 255.0) / (0.6024851624202665 - 0.4264009656413063);\n"
			"		const float b = -0.6024851624202665 * a;\n"
			"		return a * x + b;\n"
			"	} else {\n"
			"		return colormap_r2(x);\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"	const float pi = 3.141592653589793238462643383279502884197169399;\n"
			"	const float a = 126.3856859482602;\n"
			"	const float b = 0.6744554815524477;\n"
			"	const float c = 0.01070628027163306;\n"
			"	const float d = 26.95058522613648;\n"
			"	float v = a * sin(2.0 * pi / b * x + 2.0 * pi * c) + d;\n"
			"	if (v < 0.0) {\n"
			"		return -v;\n"
			"	} else {\n"
			"		return v;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"	const float pi = 3.141592653589793238462643383279502884197169399;\n"
			"	const float a = 126.9540413031656;\n"
			"	const float b = 0.2891013907955124;\n"
			"	const float c = 0.5136633102640619;\n"
			"	const float d = 126.5159759632338;\n"
			"	return a * sin(2.0 * pi / b * x + 2.0 * pi * c) + d;\n"
			"}\n"
			"\n"
			"// R1 - 255 = 0\n"
			"// => 0.4264009656413063\n"
			"\n"
			"// R2 = 0\n"
			"// => 0.6024851624202665\n"
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
