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

class Rainbow : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/transform_rainbow.frag"
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
		return std::string("rainbow");
	}

	std::string getCategory() const override
	{
		return std::string("transform");
	}

	std::string getSource() const override
	{
		return std::string(
			"vec4 colormap(float x) {\n"
			"    float r = 0.0, g = 0.0, b = 0.0;\n"
			"\n"
			"    if (x < 0.0) {\n"
			"        r = 127.0 / 255.0;\n"
			"    } else if (x <= 1.0 / 9.0) {\n"
			"        r = 1147.5 * (1.0 / 9.0 - x) / 255.0;\n"
			"    } else if (x <= 5.0 / 9.0) {\n"
			"        r = 0.0;\n"
			"    } else if (x <= 7.0 / 9.0) {\n"
			"        r = 1147.5 * (x - 5.0 / 9.0) / 255.0;\n"
			"    } else {\n"
			"        r = 1.0;\n"
			"    }\n"
			"\n"
			"    if (x <= 1.0 / 9.0) {\n"
			"        g = 0.0;\n"
			"    } else if (x <= 3.0 / 9.0) {\n"
			"        g = 1147.5 * (x - 1.0 / 9.0) / 255.0;\n"
			"    } else if (x <= 7.0 / 9.0) {\n"
			"        g = 1.0;\n"
			"    } else if (x <= 1.0) {\n"
			"        g = 1.0 - 1147.5 * (x - 7.0 / 9.0) / 255.0;\n"
			"    } else {\n"
			"        g = 0.0;\n"
			"    }\n"
			"\n"
			"    if (x <= 3.0 / 9.0) {\n"
			"        b = 1.0;\n"
			"    } else if (x <= 5.0 / 9.0) {\n"
			"        b = 1.0 - 1147.5 * (x - 3.0 / 9.0) / 255.0;\n"
			"    } else {\n"
			"        b = 0.0;\n"
			"    }\n"
			"\n"
			"    return vec4(r, g, b, 1.0);\n"
			"}\n"
		);
	}
};

} // namespace transform
} // namespace colormap
