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

class BlueWhiteLinear : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_Blue-White_Linear.frag"
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
		return std::string("Blue-White_Linear");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"    if (x < 1.0 / 3.0) {\n"
			"        return 4.0 * x - 2.992156863;\n"
			"    } else if (x < 2.0 / 3.0) {\n"
			"        return 4.0 * x - 2.9882352941;\n"
			"    } else if (x < 2.9843137255 / 3.0) {\n"
			"        return 4.0 * x - 2.9843137255;\n"
			"    } else {\n"
			"        return x;\n"
			"    }\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"    return 1.602642681354730 * x - 5.948580022657070e-1;\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"    return 1.356416928785610 * x + 3.345982835050930e-3;\n"
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

} // namespace IDL
} // namespace colormap
