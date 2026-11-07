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

class Supernova : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/transform_supernova.frag"
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
		return std::string("supernova");
	}

	std::string getCategory() const override
	{
		return std::string("transform");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_f1(float x) {\n"
			"    return (0.3647 * x + 164.02) * x + 154.21;\n"
			"}\n"
			"\n"
			"float colormap_f2(float x) {\n"
			"    return (126.68 * x + 114.35) * x + 0.1551;\n"
			"}\n"
			"\n"
			"float colormap_red(float x) {\n"
			"    if (x < 0.0) {\n"
			"        return 0.0;\n"
			"    } else if (x < 0.136721748106749) {\n"
			"        return colormap_f2(x) / 255.0;\n"
			"    } else if (x < 0.23422409711017) {\n"
			"        return (1789.6 * x - 226.52) / 255.0;\n"
			"    } else if (x < 0.498842730309711) {\n"
			"        return colormap_f1(x) / 255.0;\n"
			"    } else if (x < 0.549121259378134) {\n"
			"        return (-654.951781800243 * x + 562.838873112072) / 255.0;\n"
			"    } else if (x < 1.0) {\n"
			"        return ((3.6897 * x + 11.125) * x + 223.15) / 255.0;\n"
			"    } else {\n"
			"        return 237.0 / 255.0;\n"
			"    }\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"    if (x < 0.0) {\n"
			"        return 154.0 / 255.0;\n"
			"    } else if (x < 3.888853260731947e-2) {\n"
			"        return colormap_f1(x) / 255.0;\n"
			"    } else if (x < 0.136721748106749e0) {\n"
			"        return (-1455.86353067466 * x + 217.205447330541) / 255.0;\n"
			"    } else if (x < 0.330799131955394) {\n"
			"        return colormap_f2(x) / 255.0;\n"
			"    } else if (x < 0.498842730309711) {\n"
			"        return (1096.6 * x - 310.91) / 255.0;\n"
			"    } else if (x < 0.549121259378134) {\n"
			"        return colormap_f1(x) / 255.0;\n"
			"    } else {\n"
			"        return 244.0 / 255.0;\n"
			"    }\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"    if (x < 0.0) {\n"
			"        return 93.0 / 255.0;\n"
			"    } else if (x < 3.888853260731947e-2) {\n"
			"        return (1734.6 * x + 93.133) / 255.0;\n"
			"    } else if (x < 0.234224097110170) {\n"
			"        return colormap_f1(x) / 255.0;\n"
			"    } else if (x < 0.330799131955394) {\n"
			"        return (-1457.96598791534 * x + 534.138211325166) / 255.0;\n"
			"    } else if (x < 0.549121259378134) {\n"
			"        return colormap_f2(x) / 255.0;\n"
			"    } else if (x < 1.0) {\n"
			"        return ((3.8931 * x + 176.32) * x + 3.1505) / 255.0;\n"
			"    } else {\n"
			"        return 183.0 / 255.0;\n"
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
