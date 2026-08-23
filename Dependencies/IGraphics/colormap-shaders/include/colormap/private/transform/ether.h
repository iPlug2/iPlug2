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

class Ether : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/transform_ether.frag"
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
		return std::string("ether");
	}

	std::string getCategory() const override
	{
		return std::string("transform");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_f(float x, float b) {\n"
			"    float x2 = x * x;\n"
			"    return ((-1.89814e5 * x2 + 1.50967e4) * x2 + b) / 255.0;\n"
			"}\n"
			"\n"
			"float colormap_f2(float x, float b) {\n"
			"    float x2 = x * x;\n"
			"    return ((1.88330e5 * x2 - 1.50839e4) * x2 + b) / 255.0;\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"    if (x < 0.0e0) {\n"
			"        return 246.0 / 255.0;\n"
			"    } else if (x < 0.25) {\n"
			"        return colormap_f(x - 32.0 / 256.0, 65.0);\n"
			"    } else if (x < 130.0 / 256.0) {\n"
			"        return colormap_f2(x - 97.0 / 256.0, 190.0);\n"
			"    } else if (x < 193.0 / 256.0) {\n"
			"        return colormap_f(x - 161.0 / 256.0, 65.0);\n"
			"    } else if (x < 1.0) {\n"
			"        return colormap_f2(x - 226.0 / 256.0, 190.0);\n"
			"    } else {\n"
			"        return 18.0 / 255.0;\n"
			"    }\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"    if (x < 0.0) {\n"
			"        return 0.0;\n"
			"    } else if (x < 0.20615790927912) {\n"
			"        return ((((-3.81619e4 * x - 2.94574e3) * x + 2.61347e3) * x - 7.92183e1) * x) / 255.0;\n"
			"    } else if (x < 0.54757171958025) {\n"
			"        return (((((2.65271e5 * x - 4.14808e5) * x + 2.26118e5) * x - 5.16491e4) * x + 5.06893e3) * x - 1.80630e2) / 255.0;\n"
			"    } else if (x < 0.71235558668792) {\n"
			"        return ((((1.77058e5 * x - 4.62571e5) * x + 4.39628e5) * x - 1.80143e5) * x + 2.68555e4) / 255.0;\n"
			"    } else if (x < 1.0) {\n"
			"        float xx = ((((1.70556e5 * x - 6.20429e5) * x + 8.28331e5) * x - 4.80913e5) * x + 1.02608e5);\n"
			"        if (xx > 255.0) {\n"
			"            return (510.0 - xx) / 255.0;\n"
			"        } else {\n"
			"            return xx / 255.0;\n"
			"        }\n"
			"    } else {\n"
			"        return 154.0 / 255.0;\n"
			"    }\n"
			"}\n"
			"\n"
			"float colormap_red(float x) {\n"
			"    if (x < 0.0) {\n"
			"        return 2.0 / 255.0;\n"
			"    } else if (x < 1.0) {\n"
			"        float xx = 2.83088e2 * x + 8.17847e-1;\n"
			"        if (xx > 255.0) {\n"
			"            return (510.0 - xx) / 255.0;\n"
			"        } else {\n"
			"            return xx / 255.0;\n"
			"        }\n"
			"    } else {\n"
			"        return 226.0 / 255.0;\n"
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
