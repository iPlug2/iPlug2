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

class PurpleHaze : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/transform_purple_haze.frag"
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
		return std::string("purple_haze");
	}

	std::string getCategory() const override
	{
		return std::string("transform");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_e = exp(1.0);\n"
			"\n"
			"float colormap_red(float x) {\n"
			"    if (x < 0.0) {\n"
			"        return 13.0 / 255.0;\n"
			"    } else if (x < colormap_e * 0.1) {\n"
			"        return (706.48 * x + 13.06) / 255.0;\n"
			"    } else if (x < colormap_e * 0.1 + 149.0 / 510.0) {\n"
			"        return (166.35 * x + 28.3) / 255.0;\n"
			"    } else if (x < colormap_e * 0.1 + 298.0 / 510.0) {\n"
			"        return (313.65 * x - 47.179) / 255.0;\n"
			"    } else if (x < colormap_e * 0.05 + 202.0 / 255.0) {\n"
			"        return (557.93 * x - 310.05) / 255.0;\n"
			"    } else if (x <= 1.0) {\n"
			"        return (319.64 * x + 439093.0 / 34000.0 * colormap_e - 1030939.0 / 8500.0) / 255.0;\n"
			"    } else {\n"
			"        return 249.0 / 255.0;\n"
			"     }\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"    if (x < colormap_e * 0.1) {\n"
			"        return 0.0;\n"
			"    } else if (x < colormap_e * 0.1 + 149.0 / 510.0) {\n"
			"        return ((3166.59 / 14.9 * colormap_e + 2098.7 / 74.5) * x - (316.659 / 14.9 * colormap_e + 209.87 / 74.5) * colormap_e) / 255.0;\n"
			"    } else if (x < colormap_e * 0.1 + 298.0 / 510.0) {\n"
			"        return (725.0 * x - 394.35) / 255.0;\n"
			"    } else if (x <= 1.0) {\n"
			"        return (-716.23 * x + 721.38) / 255.0;\n"
			"    } else {\n"
			"        return 5.0 / 255.0;\n"
			"    }\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"    if (x < 0.0) {\n"
			"        return 16.0 / 255.0;\n"
			"    } else if (x < colormap_e * 0.1) {\n"
			"        return (878.72 * x + 16.389) / 255.0;\n"
			"    } else if (x < colormap_e * 0.1 + 149.0 / 510.0) {\n"
			"        return (-166.35 * x + 227.7) / 255.0;\n"
			"    } else if (x < colormap_e * 0.1 + 298.0 / 510.0) {\n"
			"        return (-317.2 * x + 305.21) / 255.0;\n"
			"    } else if (x < 1.0) {\n"
			"        return ((1530.0 / (212.0 -51.0 * colormap_e)) * x + (153.0 * colormap_e + 894.0) / (51.0 * colormap_e - 212.0)) / 255.0;\n"
			"    } else {\n"
			"        return 2.0 / 255.0;\n"
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
