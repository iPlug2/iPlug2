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

class LavaWaves : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/transform_lava_waves.frag"
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
		return std::string("lava_waves");
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
			"        return 124.0 / 255.0;\n"
			"    } else if (x <= 1.0) {\n"
			"        return (128.0 * sin(6.25 * (x + 0.5)) + 128.0) / 255.0;\n"
			"    } else {\n"
			"        return 134.0 / 255.0;\n"
			"    }\n"
			"}\n"
			"\n"
			"\n"
			"float colormap_green(float x) {\n"
			"    if (x < 0.0) {\n"
			"        return 121.0 / 255.0;\n"
			"    } else if (x <= 1.0) {\n"
			"        return (63.0 * sin(x * 99.72) + 97.0) / 255.0;\n"
			"    } else {\n"
			"        return 52.0 / 255.0;\n"
			"    }\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"    if (x < 0.0) {\n"
			"        return 131.0 / 255.0;\n"
			"    } else if (x <= 1.0) {\n"
			"        return (128.0 * sin(6.23 * x) + 128.0) / 255.0;\n"
			"    } else {\n"
			"        return 121.0 / 255.0;\n"
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
