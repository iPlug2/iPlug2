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

class Waves : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_Waves.frag"
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
		return std::string("Waves");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_f(float x, float phase) {\n"
			"	const float pi = 3.141592653589793238462643383279502884197169399;\n"
			"	const float a = 126.9634465941118;\n"
			"	const float b = 1.011727672706345;\n"
			"	const float c = 0.0038512319231245;\n"
			"	const float d = 127.5277540583575;\n"
			"	return a * sin(2.0 * pi / b * x + 2.0 * pi * (c + phase)) + d;\n"
			"}\n"
			"\n"
			"float colormap_red(float x) {\n"
			"	return colormap_f(x, 0.5);\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"	const float pi = 3.141592653589793238462643383279502884197169399;\n"
			"	const float a = 63.19460736097507;\n"
			"	const float b = 0.06323746667143024;\n"
			"	const float c = 0.06208443629833329;\n"
			"	const float d = 96.56305326777574;\n"
			"	return a * sin(2.0 * pi / b * x + 2.0 * pi * c) + d;\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"	return colormap_f(x, 0.0);\n"
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
