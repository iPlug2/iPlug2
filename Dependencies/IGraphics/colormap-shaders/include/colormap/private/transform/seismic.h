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

class Seismic : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/transform_seismic.frag"
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
		return std::string("seismic");
	}

	std::string getCategory() const override
	{
		return std::string("transform");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_f(float x) {\n"
			"    return ((-2010.0 * x + 2502.5950459) * x - 481.763180924) / 255.0;\n"
			"}\n"
			"\n"
			"float colormap_red(float x) {\n"
			"    if (x < 0.0) {\n"
			"        return 3.0 / 255.0;\n"
			"    } else if (x < 0.238) {\n"
			"        return ((-1810.0 * x + 414.49) * x + 3.87702) / 255.0;\n"
			"    } else if (x < 51611.0 / 108060.0) {\n"
			"        return (344441250.0 / 323659.0 * x - 23422005.0 / 92474.0) / 255.0;\n"
			"    } else if (x < 25851.0 / 34402.0) {\n"
			"        return 1.0;\n"
			"    } else if (x <= 1.0) {\n"
			"        return (-688.04 * x + 772.02) / 255.0;\n"
			"    } else {\n"
			"        return 83.0 / 255.0;\n"
			"    }\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"    if (x < 0.0) {\n"
			"        return 0.0;\n"
			"    } else if (x < 0.238) {\n"
			"        return 0.0;\n"
			"    } else if (x < 51611.0 / 108060.0) {\n"
			"        return colormap_f(x);\n"
			"    } else if (x < 0.739376978894039) {\n"
			"        float xx = x - 51611.0 / 108060.0;\n"
			"        return ((-914.74 * xx - 734.72) * xx + 255.) / 255.0;\n"
			"    } else {\n"
			"        return 0.0;\n"
			"    }\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"    if (x < 0.0) {\n"
			"        return 19.0 / 255.0;\n"
			"    } else if (x < 0.238) {\n"
			"        float xx = x - 0.238;\n"
			"        return (((1624.6 * xx + 1191.4) * xx + 1180.2) * xx + 255.0) / 255.0;\n"
			"    } else if (x < 51611.0 / 108060.0) {\n"
			"        return 1.0;\n"
			"    } else if (x < 174.5 / 256.0) {\n"
			"        return (-951.67322673866 * x + 709.532730938451) / 255.0;\n"
			"    } else if (x < 0.745745353439206) {\n"
			"        return (-705.250074130877 * x + 559.620050530617) / 255.0;\n"
			"    } else if (x <= 1.0) {\n"
			"        return ((-399.29 * x + 655.71) * x - 233.25) / 255.0;\n"
			"    } else {\n"
			"        return 23.0 / 255.0;\n"
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
