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

class Saturn : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/transform_saturn.frag"
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
		return std::string("saturn");
	}

	std::string getCategory() const override
	{
		return std::string("transform");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_f1(float x) {\n"
			"    return -510.0 * x + 255.0;\n"
			"}\n"
			"\n"
			"float colormap_f2(float x) {\n"
			"    return (-1891.7 * x + 217.46) * x + 255.0;\n"
			"}\n"
			"\n"
			"float colormap_f3(float x) {\n"
			"    return 9.26643676359015e1 * sin((x - 4.83450094847127e-1) * 9.93) + 1.35940451627965e2;\n"
			"}\n"
			"\n"
			"float colormap_f4(float x) {\n"
			"    return -510.0 * x + 510.0;\n"
			"}\n"
			"\n"
			"float colormap_f5(float x) {\n"
			"    float xx = x - 197169.0 / 251000.0;\n"
			"    return (2510.0 * xx - 538.31) * xx;\n"
			"}\n"
			"\n"
			"float colormap_red(float x) {\n"
			"    if (x < 0.0) {\n"
			"        return 1.0;\n"
			"    } else if (x < 10873.0 / 94585.0) {\n"
			"        float xx = colormap_f2(x);\n"
			"        if (xx > 255.0) {\n"
			"            return (510.0 - xx) / 255.0;\n"
			"        } else {\n"
			"            return xx / 255.0;\n"
			"        }\n"
			"    } else if (x < 0.5) {\n"
			"        return 1.0;\n"
			"    } else if (x < 146169.0 / 251000.0) {\n"
			"        return colormap_f4(x) / 255.0;\n"
			"    } else if (x < 197169.0 / 251000.0) {\n"
			"        return colormap_f5(x) / 255.0;\n"
			"    } else {\n"
			"        return 0.0;\n"
			"    }\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"    if (x < 10873.0 / 94585.0) {\n"
			"        return 1.0;\n"
			"    } else if (x < 36373.0 / 94585.0) {\n"
			"        return colormap_f2(x) / 255.0;\n"
			"    } else if (x < 0.5) {\n"
			"        return colormap_f1(x) / 255.0;\n"
			"    } else if (x < 197169.0 / 251000.0) {\n"
			"        return 0.0;\n"
			"    } else if (x <= 1.0) {\n"
			"        return abs(colormap_f5(x)) / 255.0;\n"
			"    } else {\n"
			"        return 0.0;\n"
			"    }\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"    if (x < 0.0) {\n"
			"        return 0.0;\n"
			"    } else if (x < 36373.0 / 94585.0) {\n"
			"        return colormap_f1(x) / 255.0;\n"
			"    } else if (x < 146169.0 / 251000.0) {\n"
			"        return colormap_f3(x) / 255.0;\n"
			"    } else if (x <= 1.0) {\n"
			"        return colormap_f4(x) / 255.0;\n"
			"    } else {\n"
			"        return 0.0;\n"
			"    }\n"
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
