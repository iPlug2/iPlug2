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

class CBBrBG : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_CB-BrBG.frag"
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
		return std::string("CB-BrBG");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"	if (x < 0.4128910005092621) {\n"
			"		return (-6.30796693758704E+02 * x + 6.59139629181867E+02) * x + 8.16592339699109E+01;\n"
			"	} else if (x < 0.5004365747118258) {\n"
			"		return -1.99292307692284E+01 * x + 2.54503076923075E+02;\n"
			"	} else if (x < 0.6000321805477142) {\n"
			"		return -4.46903540903651E+02 * x + 4.68176638176691E+02;\n"
			"	} else {\n"
			"		return ((2.43537534073204E+03 * x - 5.03831150657605E+03) * x + 2.73595321475367E+03) * x - 1.53778856560153E+02;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"	if (x < 0.3067105114459991) {\n"
			"		return (((((-1.43558931121826E+06 * x + 1.21789289489746E+06) * x - 3.88754308517456E+05) * x + 5.87745165729522E+04) * x - 3.61237992835044E+03) * x + 4.00139210969209E+02) * x + 4.80612502318691E+01;\n"
			"	} else if (x < 0.4045854562297116) {\n"
			"		return 3.64978461538455E+02 * x + 8.50984615384636E+01;\n"
			"	} else if (x < 0.5035906732082367) {\n"
			"		return 1.25827692307720E+02 * x + 1.81855384615367E+02;\n"
			"	} else {\n"
			"		return ((((-2.83948052403926E+04 * x + 1.08768529946603E+05) * x - 1.62569302478295E+05) * x + 1.17919256227845E+05) * x - 4.16776268978779E+04) * x + 6.01529271177582E+03;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"	if (x < 0.1012683545126085) {\n"
			"		return 5.85993431855501E+01 * x + 4.56403940886700E+00;\n"
			"	} else if (x < 0.2050940692424774) {\n"
			"		return 3.51072173913048E+02 * x - 2.50542028985514E+01;\n"
			"	} else if (x < 0.5022056996822357) {\n"
			"		return (-7.65121475963620E+02 * x + 1.20827362856208E+03) * x - 1.68677387505814E+02;\n"
			"	} else if (x < 0.5970333516597748) {\n"
			"		return -1.62299487179500E+02 * x + 3.26660512820525E+02;\n"
			"	} else {\n"
			"		return ((1.27993125066091E+03 * x - 3.19799978871341E+03) * x + 2.16242391471484E+03) * x - 1.93738146367890E+02;\n"
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
