/**
 * This file was automatically created with "create_c++_header.sh".
 * Do not edit manually.
 */
#pragma once
#include "../../colormap.h"

namespace colormap
{
namespace IDL
{

class GreenRedBlueWhite : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_Green-Red-Blue-White.frag"
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
		return std::string("Green-Red-Blue-White");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"    if (x < 0.2648221343873518) {\n"
			"        return 1518.00 * x - 162.00;\n"
			"    } else if (x < 0.2806324110671937) {\n"
			"        return 759.00 * x + 39.00;\n"
			"    } else if (x < 0.2964426877470356) {\n"
			"        return 252.0;\n"
			"    } else if (x < 0.3122529644268774) {\n"
			"        return -253.00 * x + 327.00;\n"
			"    } else if (x < 0.3280632411067194) {\n"
			"        return 248.0;\n"
			"    } else if (x < 0.3596837944664031) {\n"
			"        return -253.00 * x + 331.00;\n"
			"    } else if (x < 0.3636363636363636) {\n"
			"        return 240.0;\n"
			"    } else if (x < 0.3794466403162055) {\n"
			"        return -253.00 * x + 332.00;\n"
			"    } else if (x < 0.391304347826087) {\n"
			"        return 236.0;\n"
			"    } else if (x < 0.4229249011857708) {\n"
			"        return -253.00 * x + 335.00;\n"
			"    } else if (x < 0.4387351778656127) {\n"
			"        return 228.0;\n"
			"    } else if (x < 0.4861660079051384) {\n"
			"        return -253.00 * x + 339.00;\n"
			"    } else if (x < 0.5019762845849802) {\n"
			"        return 216.0;\n"
			"    } else if (x < 0.549407114624506) {\n"
			"        return -253.00 * x + 343.00;\n"
			"    } else if (x < 0.5652173913043478) {\n"
			"        return 204.0;\n"
			"    } else if (x < 0.5968379446640316) {\n"
			"        return -253.00 * x + 347.00;\n"
			"    } else if (x < 0.6126482213438735) {\n"
			"        return 196.0;\n"
			"    } else if (x < 0.6600790513833992) {\n"
			"        return -253.00 * x + 351.00;\n"
			"    } else if (x < 0.6758893280632411) {\n"
			"        return 184.0;\n"
			"    } else if (x < 0.7075098814229249) {\n"
			"        return -253.00 * x + 355.00;\n"
			"    } else if (x < 0.7233201581027668) {\n"
			"        return 176.0;\n"
			"    } else if (x < 0.7707509881422925) {\n"
			"        return -253.00 * x + 359.00;\n"
			"    } else if (x < 0.7865612648221344) {\n"
			"        return 164.0;\n"
			"    } else if (x < 0.83399209486166) {\n"
			"        return -253.00 * x + 363.00;\n"
			"    } else if (x < 0.849802371541502) {\n"
			"        return 152.0;\n"
			"    } else if (x < 0.8662737248407505) {\n"
			"        return -253.00 * x + 367.00;\n"
			"    } else {\n"
			"        return 8.24946218487293E+02 * x - 5.66796485866989E+02;\n"
			"    }\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"    if (x < 0.04321209459549381) {\n"
			"        return 9.10799999999998E+02 * x + 6.80363636363637E+01;\n"
			"    } else if (x < 0.1067193675889328) {\n"
			"        return 2277.00 * x + 9.00;\n"
			"    } else if (x < 0.1225296442687747) {\n"
			"        return -759.00 * x + 333.00;\n"
			"    } else if (x < 0.6113554850777934) {\n"
			"        return -1518.00 * x + 426.00;\n"
			"    } else if (x < 0.9924501603814814) {\n"
			"        return 1.97884558823513E+03 * x - 1.71181573083763E+03;\n"
			"    } else {\n"
			"        return 253.00 * x + 1.00;\n"
			"    }\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"    return 5.23682489688790E+02 * x - 1.55016347956506E+02;\n"
			"}\n"
			"\n"
			"vec4 colormap(float x) {\n"
			"    float r = clamp(colormap_red(x) / 255.0, 0.0, 1.0);\n"
			"    float g = clamp(colormap_green(x) / 255.0, 0.0, 1.0);\n"
			"    float b = clamp(colormap_blue(x) / 255.0, 0.0, 1.0);\n"
			"    return vec4(r, g, b, 1.0);\n"
			"}\n"
		);
	}
};

} // namespace IDL
} // namespace colormap
