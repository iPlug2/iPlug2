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

class SternSpecial : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_Stern_Special.frag"
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
		return std::string("Stern_Special");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"    if (x < (3.27037346938775E+02 + 9.33750000000000E+00) / (1.81250000000000E+01 + 5.18306122448980E+00)) { // 14.0428817217\n"
			"        return 1.81250000000000E+01 * x - 9.33750000000000E+00;\n"
			"    } else if (x <= 64.0) {\n"
			"        return -5.18306122448980E+00 * x + 3.27037346938775E+02;\n"
			"    } else {\n"
			"        return x;\n"
			"    }\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"    if (x < (8.01533134203946E+02 + 1.96917113893858E+00) / (1.99964221824687E+00 + 4.25020839121978E+00)) { // 128.063441841\n"
			"        return 1.99964221824687E+00 * x - 1.96917113893858E+00;\n"
			"    } else if (x < (8.01533134203946E+02 + 7.17997825045893E+02) / (3.80632931598691E+00 + 4.25020839121978E+00)) {\n"
			"        return -4.25020839121978E+00 * x + 8.01533134203946E+02;\n"
			"    } else {\n"
			"        return 3.80632931598691E+00 * x - 7.17997825045893E+02;\n"
			"    }\n"
			"}\n"
			"\n"
			"vec4 colormap(float x) {\n"
			"    float t = x * 255.0;\n"
			"    float r = clamp(colormap_red(t) / 255.0, 0.0, 1.0);\n"
			"    float b = clamp(colormap_blue(t) / 255.0, 0.0, 1.0);\n"
			"    return vec4(r, x, b, 1.0);\n"
			"}\n"
		);
	}
};

} // namespace IDL
} // namespace colormap
