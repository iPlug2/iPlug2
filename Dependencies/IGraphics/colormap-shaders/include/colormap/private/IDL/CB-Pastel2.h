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

class CBPastel2 : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_CB-Pastel2.frag"
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
		return std::string("CB-Pastel2");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"	if (x < 0.1414470427532423) {\n"
			"		return 5.23716927453769E+02 * x + 1.79102418207681E+02;\n"
			"	} else if (x < 0.2832126252873305) {\n"
			"		return -3.55011583011593E+02 * x + 3.03395967395968E+02;\n"
			"	} else if (x < 0.4293789173286286) {\n"
			"		return 2.81737389211071E+02 * x + 1.23060619323778E+02;\n"
			"	} else if (x < 0.5703484841123749) {\n"
			"		return -9.85406162465110E+01 * x + 2.86343977591045E+02;\n"
			"	} else if (x < 0.7170614267751989) {\n"
			"		return 1.69092460881909E+02 * x + 1.33699857752520E+02;\n"
			"	} else if (x < 0.859829619768543) {\n"
			"		return -9.94710581026121E+01 * x + 3.26276397855329E+02;\n"
			"	} else {\n"
			"		return -2.57056149732620E+02 * x + 4.61772727272750E+02;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"	if (x < 0.1411063922737659) {\n"
			"		return -1.49220483641537E+02 * x + 2.26007112375533E+02;\n"
			"	} else if (x < 0.2816283290590322) {\n"
			"		return 5.77629343629288E+01 * x + 1.96800429000430E+02;\n"
			"	} else if (x < 0.4291887492428612) {\n"
			"		return -7.38876244665610E+01 * x + 2.33876955903267E+02;\n"
			"	} else if (x < 0.571830104540257) {\n"
			"		return 3.01873399715509E+02 * x + 7.26045519203479E+01;\n"
			"	} else if (x < 0.7190262682310248) {\n"
			"		return -2.25206477732972E+01 * x + 2.58102834008109E+02;\n"
			"	} else if (x < 0.8491803538380496) {\n"
			"		return -1.16468292682893E+02 * x + 3.25653658536549E+02;\n"
			"	} else {\n"
			"		return -1.44447728516695E+02 * x + 3.49413245758086E+02;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"	if (x < 0.1425168965591466) {\n"
			"		return -2.26903238866400E+02 * x + 2.04742307692308E+02;\n"
			"	} else if (x < 0.2851292529683606) {\n"
			"		return 4.18120091673021E+02 * x + 1.12815584415585E+02;\n"
			"	} else if (x < 0.4319360871262316) {\n"
			"		return -3.09335813546247E+01 * x + 2.40853922748656E+02;\n"
			"	} else if (x < 0.7146533590447866) {\n"
			"		return -1.88956299440485E+02 * x + 3.09109637275714E+02;\n"
			"	} else if (x < 0.8619542566532371) {\n"
			"		return 2.06196082722327E+02 * x + 2.67126600285119E+01;\n"
			"	} else {\n"
			"		return -6.48097784562050E+00 * x + 2.10030557677552E+02;\n"
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
