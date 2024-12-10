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

class Space : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/transform_space.frag"
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
		return std::string("space");
	}

	std::string getCategory() const override
	{
		return std::string("transform");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"    if (x < 37067.0 / 158860.0) {\n"
			"        return 0.0;\n"
			"    } else if (x < 85181.0 / 230350.0) {\n"
			"        float xx = x - 37067.0 / 158860.0;\n"
			"        return (780.25 * xx + 319.71) * xx / 255.0;\n"
			"    } else if (x < (sqrt(3196965649.0) + 83129.0) / 310480.0) {\n"
			"        return ((1035.33580904442 * x - 82.5380748768798) * x - 52.8985266363332) / 255.0;\n"
			"    } else if (x < 231408.0 / 362695.0) {\n"
			"        return (339.41 * x - 33.194) / 255.0;\n"
			"    } else if (x < 152073.0 / 222340.0) {\n"
			"        return (1064.8 * x - 496.01) / 255.0;\n"
			"    } else if (x < 294791.0 / 397780.0) {\n"
			"        return (397.78 * x - 39.791) / 255.0;\n"
			"    } else if (x < 491189.0 / 550980.0) {\n"
			"        return 1.0;\n"
			"    } else if (x < 1.0) {\n"
			"        return (5509.8 * x + 597.91) * x / 255.0;\n"
			"    } else {\n"
			"        return 1.0;\n"
			"    }\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"    float xx;\n"
			"    if (x < 0.0) {\n"
			"        return 0.0;\n"
			"    } else if (x < (-sqrt(166317494.0) + 39104.0) / 183830.0) {\n"
			"        return (-1838.3 * x + 464.36) * x / 255.0;\n"
			"    } else if (x < 37067.0 / 158860.0) {\n"
			"        return (-317.72 * x + 74.134) / 255.0;\n"
			"    } else if (x < (3.0 * sqrt(220297369.0) + 58535.0) / 155240.0) {\n"
			"        return 0.0;\n"
			"    } else if (x < 294791.0 / 397780.0) {\n"
			"        xx = x - (3.0 * sqrt(220297369.0) + 58535.0) / 155240.0;\n"
			"        return (-1945.0 * xx + 1430.2) * xx / 255.0;\n"
			"    } else if (x < 491189.0 / 550980.0) {\n"
			"        return ((-1770.0 * x + 3.92813840044638e3) * x - 1.84017494792245e3) / 255.0;\n"
			"    } else {\n"
			"        return 1.0;\n"
			"    }\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"    if (x < 0.0) {\n"
			"        return 0.0;\n"
			"    } else if (x < 51987.0 / 349730.0) {\n"
			"        return (458.79 * x) / 255.0;\n"
			"    } else if (x < 85181.0 / 230350.0) {\n"
			"        return (109.06 * x + 51.987) / 255.0;\n"
			"    } else if (x < (sqrt(3196965649.0) + 83129.0) / 310480.0) {\n"
			"        return (339.41 * x - 33.194) / 255.0;\n"
			"    } else if (x < (3.0 * sqrt(220297369.0) + 58535.0) / 155240.0) {\n"
			"        return ((-1552.4 * x + 1170.7) * x - 92.996) / 255.0;\n"
			"    } else if (x < 27568.0 / 38629.0) {\n"
			"        return 0.0;\n"
			"    } else if (x < 81692.0 / 96241.0) {\n"
			"        return (386.29 * x - 275.68) / 255.0;\n"
			"    } else if (x <= 1.0) {\n"
			"        return (1348.7 * x - 1092.6) / 255.0;\n"
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
