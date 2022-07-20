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

class MorningGlory : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/transform_morning_glory.frag"
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
		return std::string("morning_glory");
	}

	std::string getCategory() const override
	{
		return std::string("transform");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"    if (x < 0.0) {\n"
			"        return 0.0;\n"
			"    } else if (x <= 1.0) {\n"
			"        float xx = 270.9 * x + 0.7703;\n"
			"        if (xx > 255.0) {\n"
			"            return (510.0 - xx) / 266.0;\n"
			"        } else {\n"
			"            return xx / 255.0;\n"
			"        }\n"
			"    } else {\n"
			"        return 239.0 / 255.0;\n"
			"    }\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"    if (x < 0.0) {\n"
			"        return 124.0 / 255.0;\n"
			"    } else if (x <= 1.0) {\n"
			"        float xx = 180.0 * sin(x * 3.97 + 9.46) + 131.0;\n"
			"        if (xx < 0.0) {\n"
			"            return abs(xx) / 255.0;\n"
			"        } else if (xx > 255.0) {\n"
			"            return (510.0 - xx) / 255.0;\n"
			"        } else {\n"
			"            return xx / 255.0;\n"
			"        }\n"
			"    } else {\n"
			"        return 242.0 / 255.0;\n"
			"    }\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"    if (x < 0.0) {\n"
			"        return 78.0 / 255.0;\n"
			"    } else if (x <= 1.0e0) {\n"
			"        return (95.0 * sin((x - 0.041) * 7.46) + 106.9) / 255.0;\n"
			"    } else {\n"
			"        return 179.0 / 255.0;\n"
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
