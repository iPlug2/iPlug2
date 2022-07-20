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

class HueSatValue2 : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_Hue_Sat_Value_2.frag"
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
		return std::string("Hue_Sat_Value_2");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_low(float x) {\n"
			"	return -9.89123311722871E-01 * x + 2.54113856910082E+02;\n"
			"}\n"
			"\n"
			"float colormap_r1(float x) {\n"
			"	float t = x - 44.52807774916808;\n"
			"	return (-2.10743035084859E-02 * t - 1.14339819510944E+00) * t + 255.0;\n"
			"}\n"
			"\n"
			"float colormap_r2(float x) {\n"
			"	float t = x - 173.2142990353825;\n"
			"	return (2.10464655909683E-02 * t + 3.09770350177039E+00) * t + 82.7835558104;\n"
			"}\n"
			"\n"
			"float colormap_g1(float x) {\n"
			"	float t = x - 87.18599073927922;\n"
			"	return (2.18814766236433E-02 * t + 1.07683877405025E+00) * t + 167.876161014;\n"
			"}\n"
			"\n"
			"float colormap_g2(float x) {\n"
			"	float t = x - 216.2347301863598;\n"
			"	return (-1.75617661106684E-02 * t - 5.19390917463437E+00) * t + 255.0;\n"
			"}\n"
			"\n"
			"float colormap_b2(float x) {\n"
			"	float t = x - 130.3078696041572;\n"
			"	return (-1.97675474706200E-02 * t - 3.16561290370380E+00) * t + 255.0;\n"
			"}\n"
			"\n"
			"float colormap_red(float x) {\n"
			"	if (x < 44.52807774916808) {\n"
			"		return 255.0;\n"
			"	} else if (x < 87.18599073927922) {\n"
			"		return colormap_r1(x);\n"
			"	} else if (x < 173.2142990353825) {\n"
			"		return colormap_low(x);\n"
			"	} else if (x < 216.2347301863598) {\n"
			"		return colormap_r2(x);\n"
			"	} else {\n"
			"		return 255.0;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"	if (x < 87.18599073927922) {\n"
			"		return colormap_low(x);\n"
			"	} else if (x < 130.3078696041572) {\n"
			"		return colormap_g1(x);\n"
			"	} else if (x < 216.2347301863598) {\n"
			"		return 255.0;\n"
			"	} else {\n"
			"		return colormap_g2(x);\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"	if (x < 44.52807774916808) {\n"
			"		return (2.31958376441286E-02 * x - 1.01298265446011E+00) * x + 2.54114630079813E+02; // B1\n"
			"	} else if (x < 130.3078696041572) {\n"
			"		return 255.0;\n"
			"	} else if (x < 173.2142990353825) {\n"
			"		return colormap_b2(x);\n"
			"	} else {\n"
			"		return colormap_low(x);\n"
			"	}\n"
			"}\n"
			"\n"
			"// B1 - 255 = 0\n"
			"// => [x=-0.8571972230440585,x=44.52807774916808]\n"
			"\n"
			"// R1 - low = 0\n"
			"// => [x=-5.450356335481052,x=87.18599073927922]\n"
			"\n"
			"// G1 - 255 = 0\n"
			"// => [x=-5.148233003947013,x=130.3078696041572]\n"
			"\n"
			"// B2 - low = 0\n"
			"// => [x=-22.70273917535556,x=173.2142990353825]\n"
			"\n"
			"// R2 - 255 = 0\n"
			"// => [x=-16.99015635858727,x=216.2347301863598]\n"
			"\n"
			"// low(87.18599073927922) = 167.876161014\n"
			"// low(173.2142990353825) = 82.7835558104\n"
			"\n"
			"vec4 colormap(float x) {\n"
			"	float t = x * 255.0;\n"
			"	float r = clamp(colormap_red(t) / 255.0, 0.0, 1.0);\n"
			"	float g = clamp(colormap_green(t) / 255.0, 0.0, 1.0);\n"
			"	float b = clamp(colormap_blue(t) / 255.0, 0.0, 1.0);\n"
			"	return vec4(r, g, b, 1.0);\n"
			"}\n"
		);
	}
};

} // namespace IDL
} // namespace colormap
