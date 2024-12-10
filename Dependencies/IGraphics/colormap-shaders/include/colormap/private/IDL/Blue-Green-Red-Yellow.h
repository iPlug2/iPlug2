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

class BlueGreenRedYellow : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_Blue-Green-Red-Yellow.frag"
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
		return std::string("Blue-Green-Red-Yellow");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"    if (x < (8.40958823529412E+02 - 5.2E+02) / (7.50588235294118 - 5.0)) {\n"
			"        return 7.50588235294118 * x - 8.40958823529412E+02;\n"
			"    } else if (x < (5.2E+02 + 1.27747747747748E+02) / (5.0 - 5.0e-1)) {\n"
			"        return 5.0 * x - 5.2E+02;\n"
			"    } else {\n"
			"        return 5.0e-1 * x + 1.27747747747748E+02;\n"
			"    }\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"    if (x < (150.0 + 1.00373100303951E+02) / 3.12386018237082) { // 80.1486256385\n"
			"        return 3.12386018237082 * x - 1.00373100303951E+02;\n"
			"    } else if (x < (2.08794117647059E+02 - 150.0) / 6.17647058823529E-01) { // 95.1904761905\n"
			"        return 150.0;\n"
			"    } else if (x < (4.19041176470588E+02 - 2.08794117647059E+02) / (-6.17647058823529E-01 + 2.49411764705882E+00)) { // 112.043887147\n"
			"        return -6.17647058823529E-01 * x + 2.08794117647059E+02;\n"
			"    } else if (x < (8.97617647058824E+02 - 4.19041176470588E+02) / (-2.49411764705882E+00 - -6.23529411764706E+00)) { // 127.921383648\n"
			"        return -2.49411764705882E+00 * x + 4.19041176470588E+02;\n"
			"    } else if (x < (8.97617647058824E+02 - - 3.32600912600913E+02) / (2.30624780624781E+00 - -6.23529411764706E+00)) { // 144.027690857\n"
			"        return -6.23529411764706E+00 * x + 8.97617647058824E+02;\n"
			"    } else {\n"
			"        return 2.30624780624781E+00 * x - 3.32600912600913E+02;\n"
			"    }\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"    if (x < (100.0 - - 3.36734693877551E-01) / 2.07815892314373E+00) {\n"
			"        return 2.07815892314373E+00 * x - 3.36734693877551E-01;\n"
			"    } else if (x < (3.49317448680352E+02 - 100.0) / 3.12243401759531E+00) {\n"
			"        return 100.0;\n"
			"    } else {\n"
			"        return -3.12243401759531E+00 * x + 3.49317448680352E+02;\n"
			"    }\n"
			"}\n"
			"\n"
			"vec4 colormap(float x) {\n"
			"    float t = x * 255.0;\n"
			"    float r = clamp(colormap_red(t) / 255.0, 0.0, 1.0);\n"
			"    float g = clamp(colormap_green(t) / 255.0, 0.0, 1.0);\n"
			"    float b = clamp(colormap_blue(t) / 255.0, 0.0, 1.0);\n"
			"    return vec4(r, g, b, 1.0);\n"
			"}\n"
		);
	}
};

} // namespace IDL
} // namespace colormap
