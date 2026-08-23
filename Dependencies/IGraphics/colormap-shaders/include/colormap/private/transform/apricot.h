/**
 * This file was automatically created with "create_c++_header.sh".
 * Do not edit manually.
 */
#pragma once
#include "../../colormap.h"

namespace colormap
{
namespace transform
{

class Apricot : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/transform_apricot.frag"
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
		return std::string("apricot");
	}

	std::string getCategory() const override
	{
		return std::string("transform");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_pi = 3.141592653589793;\n"
			"\n"
			"float colormap_f(float x, float c) {\n"
			"    return abs((((-5.563e-5 * x + 3.331e-16) * x + 3.045e-1) * x + 4.396e-12) * x + c);\n"
			"}\n"
			"\n"
			"float colormap_f2(float x) {\n"
			"    return 262.0 * x + 12.0 * x * sin(((x - 8.0) * x + 66.0 * colormap_pi) * x);\n"
			"}\n"
			"\n"
			"float colormap_red(float x) {\n"
			"    if (x < 0.0) {\n"
			"        return 0.0;\n"
			"    } else if (x < 1.0) {\n"
			"        float r = colormap_f2(x);\n"
			"        if (r > 255.0) {\n"
			"            r = 510.0 - r;\n"
			"        }\n"
			"        return r / 255.0;\n"
			"    } else {\n"
			"        return 1.0;\n"
			"    }\n"
			"}\n"
			"\n"
			"\n"
			"float colormap_green(float x) {\n"
			"    if (x < 0.0) {\n"
			"        return 0.0;\n"
			"    } else if (x < 1.0) {\n"
			"        return (109.0 * x + 25.0 * sin(9.92 * colormap_pi * x) * x) / 255.0;\n"
			"    } else {\n"
			"        return 102.0 / 255.0;\n"
			"    }\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"    float b = 0.0;\n"
			"    x = x * 256.0;\n"
			"\n"
			"    if (x < 0.0) {\n"
			"        b = 241.0 / 255.0;\n"
			"    } else if (x < 66.82) {\n"
			"        b = colormap_f(x - 32.0, -27.0);\n"
			"        if (x > 32.0 && b > 225.0) {\n"
			"            b = b - 10.0;\n"
			"        }\n"
			"    } else if (x < 126.67) {\n"
			"        b = colormap_f(x - 97.0, 30.0);\n"
			"    } else if (x < 195.83) {\n"
			"        b = colormap_f(x - 161.0, -27.0);\n"
			"        if (x > 161.0 && b > 225.0) {\n"
			"            b -= 10.0;\n"
			"        }\n"
			"    } else if (x < 256.0) {\n"
			"        b = colormap_f(x - 226.0, 30.0);\n"
			"    } else {\n"
			"        b = 251.0;\n"
			"    }\n"
			"    if (b > 255.0) {\n"
			"        b = 255.0;\n"
			"    }\n"
			"\n"
			"    return b / 255.0;\n"
			"}\n"
			"\n"
			"vec4 colormap(float x) {\n"
			"    return vec4(colormap_red(x), colormap_green(x), colormap_blue(x), 1.0);\n"
			"}\n"
		);
	}
};

} // namespace transform
} // namespace colormap
