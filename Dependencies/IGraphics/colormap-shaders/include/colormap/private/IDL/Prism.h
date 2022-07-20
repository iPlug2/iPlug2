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

class Prism : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_Prism.frag"
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
		return std::string("Prism");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"    if (x < (5.35651507139080E+02 + 4.75703324808184E-01) / (3.80568385693018E+00 + 4.18112109994712E+00)) {\n"
			"        return 3.80568385693018E+00 * x - 4.75703324808184E-01;\n"
			"    } else {\n"
			"        return -4.18112109994712E+00 * x + 5.35651507139080E+02;\n"
			"    }\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"    if (x < (7.72815970386039E+02 + 2.57000000000000E+02) / (4.00000000000000E+00 + 4.04283447911158E+00)) {\n"
			"        return 4.00000000000000E+00 * x - 2.57000000000000E+02;\n"
			"    } else {\n"
			"        return -4.04283447911158E+00 * x + 7.72815970386039E+02;\n"
			"    }\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"    if (x < (1.03175883256528E+03 + 4.87540173259576E+02) / (3.86517065024528E+00 + 4.04377880184332E+00)) {\n"
			"        return 3.86517065024528E+00 * x - 4.87540173259576E+02;\n"
			"    } else {\n"
			"        return -4.04377880184332E+00 * x + 1.03175883256528E+03;\n"
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
