/**
 * This file was automatically created with "create_c++_header.sh".
 * Do not edit manually.
 */
#pragma once
#include "../../colormap.h"

namespace colormap
{
namespace MATLAB
{

class Copper : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/MATLAB_copper.frag"
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
		return std::string("copper");
	}

	std::string getCategory() const override
	{
		return std::string("MATLAB");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"    return 80.0 / 63.0 * x + 5.0 / 252.0;\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"    return 0.7936 * x - 0.0124;\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"    return 796.0 / 1575.0 * x + 199.0 / 25200.0;\n"
			"}\n"
			"\n"
			"vec4 colormap(float x) {\n"
			"    float r = clamp(colormap_red(x), 0.0, 1.0);\n"
			"    float g = clamp(colormap_green(x), 0.0, 1.0);\n"
			"    float b = clamp(colormap_blue(x), 0.0, 1.0);\n"
			"    return vec4(r, g, b, 1.0);\n"
			"}\n"
		);
	}
};

} // namespace MATLAB
} // namespace colormap
