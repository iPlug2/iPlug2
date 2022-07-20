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

class StandardGammaII : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_Standard_Gamma-II.frag"
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
		return std::string("Standard_Gamma-II");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"    if (x < (8.75333333333333E+01 + 2.19676470588235E+02) / (4.76470588235294E+00 + 1.00000000000000E-01)) { // 63.1507456671\n"
			"        return 4.76470588235294E+00 * x - 2.19676470588235E+02;\n"
			"    } else if (x < (8.75333333333333E+01 + 3.11000000000000E+02) / (5.00000000000000E+00 + 1.00000000000000E-01)) { // 78.1437908497\n"
			"        return -1.00000000000000E-01 * x + 8.75333333333333E+01;\n"
			"    } else if (x < (-3.11000000000000E+02 + 3.42345454545455E+02) / (5.32727272727273E+00 - 5.00000000000000E+00)) { // 95.7777777778\n"
			"        return 5.00000000000000E+00 * x - 3.11000000000000E+02;\n"
			"    } else if (x < (255.0 + 3.42345454545455E+02) / 5.32727272727273E+00) { // 112.129692833\n"
			"        return 5.32727272727273E+00 * x - 3.42345454545455E+02;\n"
			"    } else if (x < (1.49279020979021E+03 - 255.0) / 7.68531468531468E+00) { // 161.059144677\n"
			"        return 255.0;\n"
			"    } else if (x < (1.49279020979021E+03 + 7.19657722738218E+02) / (5.10010319917441E+00 + 7.68531468531468E+00)) { // 173.04463198\n"
			"        return -7.68531468531468E+00 * x + 1.49279020979021E+03;\n"
			"    } else if (x < (255.0 + 7.19657722738218E+02) / 5.10010319917441E+00) { // 191.105490355\n"
			"        return 5.10010319917441E+00 * x - 7.19657722738218E+02;\n"
			"    } else {\n"
			"        return 255.0;\n"
			"    }\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"    if (x < (163.0 + 5.65896810850440E+02) / 5.07239736070381E+00) { // 143.69868112\n"
			"        return 5.07239736070381E+00 * x - 5.65896810850440E+02;\n"
			"    } else if (x < (163.0 + 9.06153846153846E+02) / 6.10769230769231E+00) { // 175.050377834\n"
			"        return 163.0;\n"
			"    } else {\n"
			"        return 6.10769230769231E+00 * x - 9.06153846153846E+02;\n"
			"    }\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"    if (x < (5.16164662349676E+02 - 4.77795918367347E+00) / (5.20142857142857E+00 + 5.42900092506938E+00)) { // 48.1059305585\n"
			"        return 5.20142857142857E+00 * x + 4.77795918367347E+00;\n"
			"    } else if (x < 5.16164662349676E+02 / 5.42900092506938E+00) { // 95.0754419595\n"
			"        return -5.42900092506938E+00 * x + 5.16164662349676E+02;\n"
			"    } else if (x < 6.16058823529412E+02 / 4.84558823529412E+00) { // 127.138088012\n"
			"        return 0.0;\n"
			"    } else if (x < (9.31184615384615E+02 + 6.16058823529412E+02) / (4.84558823529412E+00 + 5.89230769230769E+00)) { // 144.091863932\n"
			"        return 4.84558823529412E+00 * x - 6.16058823529412E+02;\n"
			"    } else if (x < 9.31184615384615E+02 / 5.89230769230769E+00) { // 158.033942559\n"
			"        return -5.89230769230769E+00 * x + 9.31184615384615E+02;\n"
			"    } else if (x < 5.64142909048289E+02 / 3.22177684013127E+00) { // 175.103036939\n"
			"        return 0.0;\n"
			"    } else {\n"
			"        return 3.22177684013127E+00 * x - 5.64142909048289E+02;\n"
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
