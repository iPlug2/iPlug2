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

class HueSatValue1 : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_Hue_Sat_Value_1.frag"
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
		return std::string("Hue_Sat_Value_1");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_low(float x) {\n"
			"	return (3.31549320112257E-06 * x + 9.90093135228017E-01) * x - 2.85569629002368E-01;\n"
			"}\n"
			"\n"
			"float colormap_r2(float x) {\n"
			"	float t = x - 172.2021990097892;\n"
			"	return (-2.39029325463818E-02 * t + 2.98715296752437E+00) * t + 170.308961782;\n"
			"}\n"
			"\n"
			"float colormap_g1(float x) {\n"
			"	float t = x - 86.01791713538523;\n"
			"	return (-2.29102048531908E-02 * t + 4.93089581616270E+00) * t + 84.9047112396;\n"
			"}\n"
			"\n"
			"float colormap_g2(float x) {\n"
			"	float t = x - 215.6804047700857;\n"
			"	return ((-2.84214232291614E-04 * t + 3.97502254733824E-02) * t - 1.21659773743153E+00) * t + 255.0;\n"
			"}\n"
			"\n"
			"float colormap_b2(float x) {\n"
			"	float t = x - 129.1625547389263;\n"
			"	return (2.47358957372179E-02 * t - 3.03236899995258E+00) * t + 255.0;\n"
			"}\n"
			"\n"
			"float colormap_red(float x) {\n"
			"	if (x < 43.15291462916737) {\n"
			"		return 255.0;\n"
			"	} else if (x < 86.01791713538523) {\n"
			"		return (2.31649880284905E-02 * x - 6.92920742890475E+00) * x + 5.09541054138852E+02; // R1\n"
			"	} else if (x < 172.2021990097892) {\n"
			"		return colormap_low(x);\n"
			"	} else if (x < 215.6804047700857) {\n"
			"		return colormap_r2(x);\n"
			"	} else {\n"
			"		return 255.0;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"	if (x < 86.01791713538523) {\n"
			"		return colormap_low(x);\n"
			"	} else if (x < 129.1625547389263) {\n"
			"		return colormap_g1(x);\n"
			"	} else if (x < 215.6804047700857) {\n"
			"		return 255.0;\n"
			"	} else {\n"
			"		return colormap_g2(x);\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"	if (x < 43.15291462916737) {\n"
			"		return (-2.29056417125175E-02 * x + 6.89833449327894E+00) * x - 2.89480825884616E-02; // B1\n"
			"	} else if (x < 129.1625547389263) {\n"
			"		return 255.0;\n"
			"	} else if (x < 172.2021990097892) {\n"
			"		return colormap_b2(x);\n"
			"	} else {\n"
			"		return colormap_low(x);\n"
			"	}\n"
			"}\n"
			"\n"
			"// B1 - 255 = 0\n"
			"// => [x=43.15291462916737,x=258.0102040408121]\n"
			"\n"
			"// R1 - low = 0\n"
			"// => [x=86.01791713538523,x=255.8961027639475]\n"
			"\n"
			"// G1 - 255 = 0\n"
			"// => [x=129.1625547389263,x=258.1003299995292]\n"
			"\n"
			"// B2 - low = 0\n"
			"// => [x=172.2021990097892,x=248.7957319298701]\n"
			"\n"
			"// R2 - 255 = 0\n"
			"// => [x=215.6804047700857,x=253.6941391396688]\n"
			"\n"
			"// low(86.01791713538523) = 84.9047112396\n"
			"// low(172.2021990097892) = 170.308961782\n"
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
