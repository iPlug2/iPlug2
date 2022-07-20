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

class CBPuOr : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_CB-PuOr.frag"
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
		return std::string("CB-PuOr");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"	if (x < 0.3021800816059113) {\n"
			"		return ((-8.74206463186070E+02 * x - 1.85426300850231E+02) * x + 5.51285275571572E+02) * x + 1.26643739338646E+02;\n"
			"	} else if (x < 0.5021429359912872) {\n"
			"		return (-3.88346609612054E+02 * x + 2.84470703793855E+02) * x + 2.01677105728121E+02;\n"
			"	} else {\n"
			"		return ((((-1.46535873909592E+04 * x + 5.53022453680932E+04) * x - 8.13970779339075E+04) * x + 5.81771336215697E+04) * x - 2.05245461242988E+04) * x + 3.14144977294882E+03;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"	if (x < 0.05763718485832214) {\n"
			"		return 2.95595213675214E+02 * x + 5.90683760683760E+01;\n"
			"	} else if (x < 0.5007581412792206) {\n"
			"		return ((((2.62405984206199E+04 * x - 3.27451861162955E+04) * x + 1.22107381531950E+04) * x - 1.00838782028235E+03) * x + 2.70642765981150E+02) * x + 6.36961934872237E+01;\n"
			"	} else if (x < 0.8994744718074799) {\n"
			"		return ((-2.60499440597370E+02 * x - 1.69367458674853E+02) * x + 1.12913289561831E+02) * x + 2.65534872970494E+02;\n"
			"	} else {\n"
			"		return -3.86892551892486E+02 * x + 3.88498371998310E+02;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"	if (x < 0.09708546522206818) {\n"
			"		return -2.18830769230769E+01 * x + 8.03384615384615E+00;\n"
			"	} else if (x < 0.2041181623935699) {\n"
			"		return 1.36731800766284E+02 * x - 7.36535303776692E+00;\n"
			"	} else if (x < 0.4982341825962067) {\n"
			"		return (-3.57661007879491E+02 * x + 1.02135850891279E+03) * x - 1.73032069862183E+02;\n"
			"	} else if (x < 0.6063862144947052) {\n"
			"		return -1.17230769230812E+02 * x + 3.05467236467253E+02;\n"
			"	} else {\n"
			"		return (((((6.76307238761902E+05 * x - 3.23892241330779E+06) * x + 6.41842651908440E+06) * x - 6.73646783203943E+06) * x + 3.94913439210334E+06) * x - 1.22625641797298E+06) * x + 1.57856249228480E+05;\n"
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
