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

class Beach : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_Beach.frag"
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
		return std::string("Beach");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"	if (x <= 0.5) {\n"
			"		return 1.07069284759359E+03 * x - 3.92901626559718E+02; // R1\n"
			"	} else if (x < (4.62452380952399E+02 - 3.16199999999948E+02) / (1.05778571428574E+03 - 8.03199999999881E+02)) { // 0.57447206479\n"
			"		return 1.05778571428574E+03 * x - 4.62452380952399E+02; // R2\n"
			"	} else if (x < (2.16218045113087E+01 + 3.16199999999948E+02) / (8.03199999999881E+02 - 2.93462406015021E+02)) { // 0.66273668746\n"
			"		return 8.03199999999881E+02 * x - 3.16199999999948E+02; // R3\n"
			"	} else if (x < 0.7332708626326772) {\n"
			"		return 2.93462406015021E+02 * x + 2.16218045113087E+01; // R4\n"
			"	} else {\n"
			"		return 0.0;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"	if (x < (2.51230508474576E+02 - 2.71026069518750E+01) / (4.27421457219241E+02 + 5.35095032144939E+02)) { // 0.23285616817\n"
			"		return -5.35095032144939E+02 * x + 2.51230508474576E+02; // G1\n"
			"	} else if (x < (6.91824598930488E+02 - 2.71026069518750E+01) / (4.27421457219241E+02 + 1.39121879297175E+03)) { // 0.36550493804\n"
			"		return 4.27421457219241E+02 * x + 2.71026069518750E+01; // G2\n"
			"	} else if (x < (6.91824598930488E+02 + 5.16725562656262E+02) / (1.02304642956372E+03 + 1.39121879297175E+03)) { // 0.500587156\n"
			"		return -1.39121879297175E+03 * x + 6.91824598930488E+02; // G3\n"
			"	} else if (x < 0.7332708626326772) {\n"
			"		return 1.02304642956372E+03 * x - 5.16725562656262E+02; // G4\n"
			"	} else {\n"
			"		return 0.0;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"	if (x < 0.3584754040051419) {\n"
			"		return 1.07130443548384E+03 * x - 2.48036290322570E+02; // B1\n"
			"	} else if (x < 123.0 / 251.0) {\n"
			"		return 136.0;\n"
			"	} else if (x < 0.7332708626326772) {\n"
			"		return -1.01758796992489E+03 * x + 5.87035338345905E+02; // B2\n"
			"	} else {\n"
			"		return 8.98509790209691E+02 * x - 6.58851048950966E+02; // B3\n"
			"	}\n"
			"}\n"
			"\n"
			"// R2 - R3 = 0\n"
			"// => [x=0.5744720647924222]\n"
			"\n"
			"// B1 - 136 = 0\n"
			"// => [x=0.3584754040051419]\n"
			"\n"
			"// B3 = 0\n"
			"// => [x=0.7332708626326772]\n"
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
