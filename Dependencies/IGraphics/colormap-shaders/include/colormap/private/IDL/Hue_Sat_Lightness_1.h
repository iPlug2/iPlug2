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

class HueSatLightness1 : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_Hue_Sat_Lightness_1.frag"
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
		return std::string("Hue_Sat_Lightness_1");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_low(float x) {\n"
			"	return (-1.91005335917480E-03 * x + 1.49751468348116E+00) * x - 6.97037614414503E+00; // lower\n"
			"}\n"
			"\n"
			"float colormap_up(float x) {\n"
			"	return (1.88420526249161E-03 * x - 5.03556849093925E-01) * x + 2.55777688663313E+02; // upper\n"
			"}\n"
			"\n"
			"float colormap_red(float x) {\n"
			"	if (x < 43.76015739302458) { // first root of `lower = B1`\n"
			"		return colormap_up(x);\n"
			"	} else if (x < 86.85552651930304) { // first root of `lower = R1`\n"
			"		return (3.42882679808412E-02 * x - 7.47424573913507E+00) * x + 4.99200716753466E+02; // R1\n"
			"	} else if (x < 174.6136813850324) { // first root of `low = B2`\n"
			"		return colormap_low(x);\n"
			"	} else {\n"
			"		return ((1.12237347384081E-04 * x - 7.83534162528667E-02) * x + 1.86033275155350E+01) * x - 1.25879271751642E+03;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"	if (x < 86.85552651930304) {\n"
			"		return colormap_low(x);\n"
			"	} else if (x < 130.6514942376722) {\n"
			"		return (-2.86318899478317E-02 * x + 8.83599571161434E+00) * x - 4.43544771805581E+02; // G1\n"
			"	} else {\n"
			"		return colormap_up(x);\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"	if (x < 43.76015739302458) {\n"
			"		return (-3.50205069618621E-02 * x + 7.15746326474339E+00) * x - 8.79902788903102E+00; // B1\n"
			"	} else if (x < 130.6514942376722) { // first root of `upper = G1`\n"
			"		return colormap_up(x);\n"
			"	} else if (x < 174.6136813850324) { // first root of `low = B2`\n"
			"		return (1.99506804131033E-02 * x - 6.64847464240324E+00) * x + 7.48898397192062E+02; // B2\n"
			"	} else {\n"
			"		return colormap_low(x);\n"
			"	}\n"
			"}\n"
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
