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

class GreenPink : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_Green-Pink.frag"
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
		return std::string("Green-Pink");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"    if (x < (6.20000000000000E+02 - 6.05717647058824E+02) / (5.00000000000000E+00 - 4.87941176470588E+00)) { // 118.43902439\n"
			"        return 5.00000000000000E+00 * x - 6.20000000000000E+02;\n"
			"    } else if (x < (6.05717647058824E+02 - 3.12000000000000E+02) / (4.87941176470588E+00 - 3.00000000000000E+00)) { // 156.281690141\n"
			"        return 4.87941176470588E+00 * x - 6.05717647058824E+02;\n"
			"    } else if (x < (252.0 + 3.12000000000000E+02) / 3.00000000000000E+00) { // 188\n"
			"        return 3.00000000000000E+00 * x - 3.12000000000000E+02;\n"
			"    } else {\n"
			"        return 252.0;\n"
			"    }\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"    if (x < (2.04536133198612E+02 + 1.47941176470588E+01) / (9.95833333333333E+00 + 1.14822299345429E+00)) {\n"
			"        return 9.95833333333333E+00 * x - 1.47941176470588E+01;\n"
			"    } else if (x < (2.72705547652916E+02 - 2.04536133198612E+02) / (1.69701280227596E+00 - 1.14822299345429E+00)) {\n"
			"        return -1.14822299345429E+00 * x + 2.04536133198612E+02;\n"
			"    } else if (x < 2.72705547652916E+02 / 1.69701280227596E+00) {\n"
			"        return -1.69701280227596E+00 * x + 2.72705547652916E+02;\n"
			"    } else if (x < 7.52000000000000E+02 / 4.00000000000000E+00) {\n"
			"        return 0.0;\n"
			"    } else if (x < (7.52000000000000E+02 - 7.45733990147783E+02) / (4.00000000000000E+00 - 3.95785440613027E+00)) {\n"
			"        return 4.00000000000000E+00 * x - 7.52000000000000E+02;\n"
			"    } else {\n"
			"        return 3.95785440613027E+00 * x - 7.45733990147783E+02;\n"
			"    }\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"    if (x < (2.50785650623886E+02 + 4.04215299263843E+01) / (1.92690173903766E+00 + 4.23796791443850E-01)) {\n"
			"        return 1.92690173903766E+00 * x - 4.04215299263843E+01;\n"
			"    } else if (x < (2.50785650623886E+02 - 1.58221357063404E+02) / (1.75528364849833E-01 + 4.23796791443850E-01)) {\n"
			"        return -4.23796791443850E-01 * x + 2.50785650623886E+02;\n"
			"    } else if (x < (1.58221357063404E+02 - 1.27826659541169E+01) / (9.48066572508303E-01 - 1.75528364849833E-01)) {\n"
			"        return 1.75528364849833E-01 * x + 1.58221357063404E+02;\n"
			"    } else {\n"
			"        return 9.48066572508303E-01 * x + 1.27826659541169E+01;\n"
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
