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

class Nature : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_Nature.frag"
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
		return std::string("Nature");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"	if (x < 0.8) {\n"
			"		const float pi = 3.141592653589793238462643383279502884197169399;\n"
			"		float v = sin(2.0 * pi * (x * 2.440771851872335 + 0.03082889566781979)) * 94.0 + 86.68712190457872;\n"
			"		if (v < 0.0) {\n"
			"			return -v;\n"
			"		} else {\n"
			"			return v;\n"
			"		}\n"
			"	} else {\n"
			"		return 82.0;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"	if (x < 0.8) {\n"
			"		const float pi = 3.141592653589793238462643383279502884197169399;\n"
			"		const float a = - 133.8180196373718;\n"
			"		const float b = 105.5963045788319;\n"
			"		const float c = 1.192621559448461;\n"
			"		const float d = 4.00554233186818;\n"
			"		const float e = 0.04779355732364274;\n"
			"		const float f = 218.2356517672776;\n"
			"		const float g = -269.6049419208264;\n"
			"		float v = (a * x + b) * sin(2.0 * pi / c * (d * x + e)) + f + g * x;\n"
			"		if (v > 255.0) {\n"
			"			return 255.0 - (v - 255.0);\n"
			"		} else {\n"
			"			return v;\n"
			"		}\n"
			"	} else {\n"
			"		return 0.0;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"	if (x < 0.8) {\n"
			"		const float pi = 3.141592653589793238462643383279502884197169399;\n"
			"		const float a = 2.443749115965466;\n"
			"		const float b = 0.02934035424870109;\n"
			"		const float c = 253.745120022022;\n"
			"		const float d = 226.671125688366;\n"
			"		float v = sin(2.0 * pi * (x * a + b))*c + d;\n"
			"		if (v > 255.0) {\n"
			"			return 255.0 - (v - 255.0);\n"
			"		} else if (v < 0.0) {\n"
			"			return -v;\n"
			"		} else {\n"
			"			return v;\n"
			"		}\n"
			"	} else {\n"
			"		return 214.0;\n"
			"	}\n"
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
