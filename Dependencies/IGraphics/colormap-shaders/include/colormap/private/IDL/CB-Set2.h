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

class CBSet2 : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_CB-Set2.frag"
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
		return std::string("CB-Set2");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"	if (x < 0.1417230259672803) {\n"
			"		return 1.05935704125178E+03 * x + 1.01981507823613E+02;\n"
			"	} else if (x < 0.2832918395614404) {\n"
			"		return -7.84521574205797E+02 * x + 3.63301564722620E+02;\n"
			"	} else if (x < 0.4297095252306736) {\n"
			"		return 6.13507692307669E+02 * x - 3.27487179487109E+01;\n"
			"	} else if (x < 0.5711437760817145) {\n"
			"		return -4.56971171171184E+02 * x + 4.27246246246258E+02;\n"
			"	} else if (x < 0.7170125543394245) {\n"
			"		return 6.08443812233345E+02 * x - 1.81258890469442E+02;\n"
			"	} else if (x < 0.8601498726198596) {\n"
			"		return -1.86129554655803E+02 * x + 3.88460188933823E+02;\n"
			"	} else {\n"
			"		return -3.44927731092377E+02 * x + 5.25050420168008E+02;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"	if (x < 0.1415748781045371) {\n"
			"		return -3.73352299668090E+02 * x + 1.93944523470839E+02;\n"
			"	} else if (x < 0.2833646736626308) {\n"
			"		return 1.32720720720722E+02 * x + 1.22297297297298E+02;\n"
			"	} else if (x < 0.4289465541806786) {\n"
			"		return -1.49887515045407E+02 * x + 2.02378487799539E+02;\n"
			"	} else if (x < 0.5702226800496928) {\n"
			"		return 5.49972024656235E+02 * x - 9.78238501659562E+01;\n"
			"	} else if (x < 0.7146023128494844) {\n"
			"		return 8.99797570850851E+00 * x + 2.10651821862350E+02;\n"
			"	} else if (x < 0.8745816247778255) {\n"
			"		return -1.44022832980902E+02 * x + 3.20000845665901E+02;\n"
			"	} else {\n"
			"		return -1.15363546797967E+02 * x + 2.94935960591057E+02;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"	if (x < 0.1415014758409641) {\n"
			"		return -4.74217638691322E+02 * x + 1.65038406827880E+02;\n"
			"	} else if (x < 0.2835702564175819) {\n"
			"		return 7.39899952584171E+02 * x - 6.76102418207873E+00;\n"
			"	} else if (x < 0.4295821637397355) {\n"
			"		return -5.64243717401478E+01 * x + 2.19052868658126E+02;\n"
			"	} else if (x < 0.5693521153970126) {\n"
			"		return -7.86119327731098E+02 * x + 5.32516806722689E+02;\n"
			"	} else if (x < 0.7156505014449713) {\n"
			"		return -2.61647700331917E+02 * x + 2.33907776197252E+02;\n"
			"	} else if (x < 0.858726752429748) {\n"
			"		return 7.09638034795805E+02 * x - 4.61193347193250E+02;\n"
			"	} else {\n"
			"		return 2.09176470588245E+02 * x - 3.14336134453805E+01;\n"
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
