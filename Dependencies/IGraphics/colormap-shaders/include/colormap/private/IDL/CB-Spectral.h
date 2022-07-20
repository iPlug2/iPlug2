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

class CBSpectral : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_CB-Spectral.frag"
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
		return std::string("CB-Spectral");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"	if (x < 0.09752005946586478) {\n"
			"		return 5.63203907203907E+02 * x + 1.57952380952381E+02;\n"
			"	} else if (x < 0.2005235116443438) {\n"
			"		return 3.02650769230760E+02 * x + 1.83361538461540E+02;\n"
			"	} else if (x < 0.2974133397506856) {\n"
			"		return 9.21045429665647E+01 * x + 2.25581007115501E+02;\n"
			"	} else if (x < 0.5003919130598823) {\n"
			"		return 9.84288115246108E+00 * x + 2.50046722689075E+02;\n"
			"	} else if (x < 0.5989021956920624) {\n"
			"		return -2.48619704433547E+02 * x + 3.79379310344861E+02;\n"
			"	} else if (x < 0.902860552072525) {\n"
			"		return ((2.76764884219295E+03 * x - 6.08393126459837E+03) * x + 3.80008072407485E+03) * x - 4.57725185424742E+02;\n"
			"	} else {\n"
			"		return 4.27603478260530E+02 * x - 3.35293188405479E+02;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"	if (x < 0.09785836420571035) {\n"
			"		return 6.23754529914529E+02 * x + 7.26495726495790E-01;\n"
			"	} else if (x < 0.2034012006283468) {\n"
			"		return 4.60453201970444E+02 * x + 1.67068965517242E+01;\n"
			"	} else if (x < 0.302409765476316) {\n"
			"		return 6.61789401709441E+02 * x - 2.42451282051364E+01;\n"
			"	} else if (x < 0.4005965758690823) {\n"
			"		return 4.82379130434784E+02 * x + 3.00102898550747E+01;\n"
			"	} else if (x < 0.4981907026473237) {\n"
			"		return 3.24710622710631E+02 * x + 9.31717541717582E+01;\n"
			"	} else if (x < 0.6064345916502067) {\n"
			"		return -9.64699507389807E+01 * x + 3.03000000000023E+02;\n"
			"	} else if (x < 0.7987472620841592) {\n"
			"		return -2.54022986425337E+02 * x + 3.98545610859729E+02;\n"
			"	} else {\n"
			"		return -5.71281628959223E+02 * x + 6.51955082956207E+02;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"	if (x < 0.0997359608740309) {\n"
			"		return 1.26522393162393E+02 * x + 6.65042735042735E+01;\n"
			"	} else if (x < 0.1983790695667267) {\n"
			"		return -1.22037851037851E+02 * x + 9.12946682946686E+01;\n"
			"	} else if (x < 0.4997643530368805) {\n"
			"		return (5.39336225400169E+02 * x + 3.55461986381562E+01) * x + 3.88081126069087E+01;\n"
			"	} else if (x < 0.6025972254407099) {\n"
			"		return -3.79294261294313E+02 * x + 3.80837606837633E+02;\n"
			"	} else if (x < 0.6990141388105746) {\n"
			"		return 1.15990231990252E+02 * x + 8.23805453805459E+01;\n"
			"	} else if (x < 0.8032653181119567) {\n"
			"		return 1.68464957265204E+01 * x + 1.51683418803401E+02;\n"
			"	} else if (x < 0.9035796343050095) {\n"
			"		return 2.40199023199020E+02 * x - 2.77279202279061E+01;\n"
			"	} else {\n"
			"		return -2.78813846153774E+02 * x + 4.41241538461485E+02;\n"
			"	}\n"
			"}\n"
			"\n"
			"vec4 colormap(float x) {\n"
			"	float r = clamp(colormap_red(x) / 255.0, 0.0, 1.0);\n"
			"	float g = clamp(colormap_green(x) / 255.0, 0.0, 1.0);\n"
			"	float b = clamp(colormap_blue(x) / 255.0, 0.0, 1.0);\n"
			"	return vec4(r, g, b, 1.0);\n"
			"}\n"
		);
	}
};

} // namespace IDL
} // namespace colormap
