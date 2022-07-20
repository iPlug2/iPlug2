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

class Malachite : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/transform_malachite.frag"
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
		return std::string("malachite");
	}

	std::string getCategory() const override
	{
		return std::string("transform");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_blue(float x) {\n"
			"    if (x < 248.25 / 1066.8) {\n"
			"        return 0.0;\n"
			"    } else if (x < 384.25 / 1066.8) {\n"
			"        return (1066.8 * x - 248.25) / 255.0;\n"
			"    } else if (x < 0.5) {\n"
			"        return 136.0 / 255.0;\n"
			"    } else if (x < 595.14 / 1037.9) {\n"
			"        return (-1037.9 * x + 595.14) / 255.0;\n"
			"    } else if (x < 666.68 / 913.22) {\n"
			"        return 0.0;\n"
			"    } else if (x <= 1.0) {\n"
			"        return (913.22 * x - 666.68) / 255.0;\n"
			"    } else {\n"
			"        return 246.0 / 255.0;\n"
			"    }\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"    if (x < 0.0) {\n"
			"        return 253.0 / 255.0;\n"
			"    } else if (x < 248.25 / 1066.8) {\n"
			"        return (-545.75 * x + 253.36) / 255.0;\n"
			"    } else if (x < 384.25 / 1066.8) {\n"
			"        return  (426.18 * x + 19335217.0 / 711200.0) / 255.0;\n"
			"    } else if (x < 0.5) {\n"
			"        return (-385524981.0 / 298300.0 * x + 385524981.0 / 596600.0) / 255.0;\n"
			"    } else if (x < 666.68 / 913.22) {\n"
			"        return (3065810.0 / 3001.0 * x - 1532906.0 / 3001.0) / 255.0;\n"
			"    } else {\n"
			"        return 0.0;\n"
			"    }\n"
			"}\n"
			"\n"
			"float colormap_red(float x) {\n"
			"    if (x < 384.25 / 1066.8) {\n"
			"        return 0.0;\n"
			"    } else if (x < 0.5) {\n"
			"        return (1092.0 * x - 99905.0 / 254.0) / 255.0;\n"
			"    } else if (x < 259.3 / 454.5) {\n"
			"        return (1091.9 * x - 478.18) / 255.0;\n"
			"    } else if (x < 34188.3 / 51989.0) {\n"
			"        return (819.2 * x - 322.6) / 255.0;\n"
			"    } else if (x < 666.68 / 913.22) {\n"
			"        return (299.31 * x + 19.283) / 255.0;\n"
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
