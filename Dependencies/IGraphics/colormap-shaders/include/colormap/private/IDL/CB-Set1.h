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

class CBSet1 : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_CB-Set1.frag"
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
		return std::string("CB-Set1");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"	if (x < 0.1218978105733187) {\n"
			"		return -1.41278189149560E+03 * x + 2.27681818181818E+02;\n"
			"	} else if (x < 0.2479420725675653) {\n"
			"		return 1.71483957219249E+02 * x + 3.45632798573976E+01;\n"
			"	} else if (x < 0.3737958863310976) {\n"
			"		return 5.93826871657766E+02 * x - 7.01532976827142E+01;\n"
			"	} else if (x < 0.5581935583402973) {\n"
			"		return 8.16999193548379E+02 * x - 1.53574193548385E+02;\n"
			"	} else if (x < 0.7521306441106583) {\n"
			"		return -7.03338903743339E+02 * x + 6.95068738859197E+02;\n"
			"	} else if (x < 0.8778625049273406) {\n"
			"		return 6.43286656891390E+02 * x - 3.17769611436878E+02;\n"
			"	} else {\n"
			"		return -7.44200222469495E+02 * x + 9.00253096032687E+02;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"	if (x < 0.1210132014777112) {\n"
			"		return 8.22324999999999E+02 * x + 2.59536290322581E+01;\n"
			"	} else if (x < 0.2482778857828818) {\n"
			"		return 3.90762700534761E+02 * x + 7.81783645276292E+01;\n"
			"	} else if (x < 0.3736981279637023) {\n"
			"		return -7.75174853372465E+02 * x + 3.67654875366580E+02;\n"
			"	} else if (x < 0.5000834495110191) {\n"
			"		return 3.88169354838696E+02 * x - 6.70846774193464E+01;\n"
			"	} else if (x < 0.6259565510341616) {\n"
			"		return 1.01632587976547E+03 * x - 3.81215359237582E+02;\n"
			"	} else if (x < 0.7519260802219788) {\n"
			"		return -1.34046122994658E+03 * x + 1.09403097147954E+03;\n"
			"	} else if (x < 0.8802211108953331) {\n"
			"		return 3.40231932773057E+02 * x - 1.69726050420116E+02;\n"
			"	} else {\n"
			"		return 1.89186206896551E+02 * x - 3.67724137931057E+01;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"	if (x < 0.1216936022984158) {\n"
			"		return 1.28638709677419E+03 * x + 2.74375000000001E+01;\n"
			"	} else if (x < 0.2481802512682617) {\n"
			"		return -8.68609237536630E+02 * x + 2.89686766862165E+02;\n"
			"	} else if (x < 0.3738953463082063) {\n"
			"		return 7.06041788856318E+02 * x - 1.01110520527863E+02;\n"
			"	} else if (x < 0.499829701274646) {\n"
			"		return -1.29118218475062E+03 * x + 6.45642228738955E+02;\n"
			"	} else if (x < 0.6262008893543518) {\n"
			"		return 3.99252005347605E+02 * x - 1.99286987522289E+02;\n"
			"	} else if (x < 0.752318389825417) {\n"
			"		return -8.38814516128947E+01 * x + 1.03251612903218E+02;\n"
			"	} else if (x < 0.8779851862270176) {\n"
			"		return 1.20109970674463E+03 * x - 8.63463343108315E+02;\n"
			"	} else {\n"
			"		return -3.03613348164648E+02 * x + 4.57652428624434E+02;\n"
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
