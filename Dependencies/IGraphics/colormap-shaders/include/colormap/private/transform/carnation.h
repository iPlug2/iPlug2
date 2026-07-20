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

class Carnation : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/transform_carnation.frag"
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
		return std::string("carnation");
	}

	std::string getCategory() const override
	{
		return std::string("transform");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_f(float x) {\n"
			"    return ((-9.93427e0 * x + 1.56301e1) * x + 2.44663e2 * x) / 255.0;\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"    if (x < 0.0) {\n"
			"        return 11.0 / 255.0;\n"
			"    } else if (x < 0.16531216481302) {\n"
			"        return (((-1635.0 * x) + 1789.0) * x + 3.938) / 255.0;\n"
			"    } else if (x < 0.50663669203696) {\n"
			"        return 1.0;\n"
			"    } else if (x < 0.67502056695956) {\n"
			"        return ((((1.28932e3 * x) - 7.74147e2) * x - 9.47634e2) * x + 7.65071e2) / 255.0;\n"
			"    } else if (x < 1.0) {\n"
			"        return colormap_f(x);\n"
			"    } else {\n"
			"        return 251.0 / 255.0;\n"
			"    }\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"    if (x < 0.0) {\n"
			"        return 0.0;\n"
			"    } else if (x < 0.33807590140751) {\n"
			"        return colormap_f(x);\n"
			"    } else if (x < 0.50663669203696) {\n"
			"        return (((-5.83014e2 * x - 8.38523e2) * x + 2.03823e3) * x - 4.86592e2) / 255.0;\n"
			"    } else if (x < 0.84702285244773) {\n"
			"        return 1.0;\n"
			"    } else if (x < 1.0) {\n"
			"        return (((-5.03306e2 * x + 2.95545e3) * x - 4.19210e3) * x + 1.99128e3) / 255.0;\n"
			"    } else {\n"
			"        return 251.0 / 255.0;\n"
			"    }\n"
			"}\n"
			"\n"
			"float colormap_red(float x) {\n"
			"    if (x < 0.16531216481302) {\n"
			"        return 1.0;\n"
			"    } else if (x < 0.33807590140751) {\n"
			"        return (((-5.15164e3 * x + 5.30564e3) * x - 2.65098e3) * x + 5.70771e2) / 255.0;\n"
			"    } else if (x < 0.67502056695956) {\n"
			"        return colormap_f(x);\n"
			"    } else if (x < 0.84702285244773) {\n"
			"        return (((3.34136e3 * x - 9.01976e3) * x + 8.39740e3) * x - 2.41682e3) / 255.0;\n"
			"    } else {\n"
			"        return 1.0;\n"
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
