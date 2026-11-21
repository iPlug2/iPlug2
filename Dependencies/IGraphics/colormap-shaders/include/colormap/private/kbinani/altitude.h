/**
 * This file was automatically created with "create_c++_header.sh".
 * Do not edit manually.
 */
#pragma once
#include "../../colormap.h"

namespace colormap
{
namespace kbinani
{

class Altitude : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/kbinani_altitude.frag"
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
		return std::string("altitude");
	}

	std::string getCategory() const override
	{
		return std::string("kbinani");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_func(float x, float a, float c, float d, float e, float f, float g) {\n"
			"	x = 193.0 * clamp(x, 0.0, 1.0);\n"
			"    return a * exp(-x * x / (2.0 * c * c)) + ((d * x + e) * x + f) * x + g;\n"
			"}\n"
			"\n"
			"vec4 colormap(float x) {\n"
			"    float r = colormap_func(x, 48.6399, 13.3443, 0.00000732641, -0.00154886, -0.211758, 83.3109);\n"
			"    float g = colormap_func(x, 92.7934, 9.66818, 0.00000334955, -0.000491041, -0.189276, 56.8844);\n"
			"    float b = colormap_func(x, 43.4277, 8.92338, 0.00000387675, -0.00112176, 0.0373863, 15.9435);\n"
			"    return vec4(r / 255.0, g / 255.0, b / 255.0, 1.0);\n"
			"}\n"
		);
	}
};

} // namespace kbinani
} // namespace colormap
