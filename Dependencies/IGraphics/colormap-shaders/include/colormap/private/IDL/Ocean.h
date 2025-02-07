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

class Ocean : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_Ocean.frag"
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
		return std::string("Ocean");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"	if (x < 0.84121424085) {\n"
			"		const float pi = 3.141592653589793238462643383279502884197169399;\n"
			"		const float a = 92.39421034238549;\n"
			"		const float b = 88.02925388837211;\n"
			"		const float c = 0.5467741159150409;\n"
			"		const float d = 0.03040219113949284;\n"
			"		return a * sin(2.0 * pi / c * (x - d)) + b;\n"
			"	} else {\n"
			"		return 105.0;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"	if (x < 0.84121424085) {\n"
			"		const float pi = 3.141592653589793238462643383279502884197169399;\n"
			"		const float a = 92.44399971120093;\n"
			"		const float b = 22.7616696017667;\n"
			"		const float c = 0.3971750420482239;\n"
			"		const float d = 0.1428144080827581;\n"
			"		const float e = 203.7220396611977;\n"
			"		const float f = 49.51517183258432;\n"
			"		float v = (a * x + b) * sin(2.0 * pi / c * (x + d)) + (e * x + f);\n"
			"		if (v > 255.0) {\n"
			"			return 255.0 - (v - 255.0);\n"
			"		} else {\n"
			"			return v;\n"
			"		}\n"
			"	} else {\n"
			"		return 246.0;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"	if (x < 0.84121424085) {\n"
			"		const float pi = 3.141592653589793238462643383279502884197169399;\n"
			"		const float a = 251.0868719483008;\n"
			"		const float b = 0.5472498585835275;\n"
			"		const float c = 0.02985857858149428;\n"
			"		const float d = 225.9495771701237;\n"
			"		float v = a * sin(2.0 * pi / b * (x - c)) + d;\n"
			"		if (v > 255.0) {\n"
			"			return 255.0 - (v - 255.0);\n"
			"		} else if (v < 0.0) {\n"
			"			return -v;\n"
			"		} else {\n"
			"			return v;\n"
			"		}\n"
			"	} else {\n"
			"		return 234.0;\n"
			"	}\n"
			"}\n"
			"\n"
			"// R1 - 105 = 0\n"
			"// => 0.8344881408181015\n"
			"\n"
			"// B1 - 234 = 0\n"
			"// => 0.847940340889657\n"
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
