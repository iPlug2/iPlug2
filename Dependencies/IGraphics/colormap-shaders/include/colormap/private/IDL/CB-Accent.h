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

class CBAccent : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_CB-Accent.frag"
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
		return std::string("CB-Accent");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"	if (x < 0.3953623640294495) {\n"
			"		return 4.43904418116747E+02 * x + 1.27235098111810E+02; // R1\n"
			"	} else if (x < 0.5707952559161218) {\n"
			"		return -1.40693770913775E+03 * x + 8.58988416988442E+02; // R2\n"
			"	} else if (x < 0.7167119613307117) {\n"
			"		return 1.26199099099112E+03 * x - 6.64423423423499E+02; // R3\n"
			"	} else if (x < 0.8579249381242428) {\n"
			"		return -3.45591749644313E+02 * x + 4.87750355618723E+02; // R4\n"
			"	} else {\n"
			"		return -6.12161344537705E+02 * x + 7.16447058823441E+02; // R5\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"	if (x < 0.1411385125927322) {\n"
			"		return -1.90229018492177E+02 * x + 2.01102418207681E+02; // G1\n"
			"	} else if (x < 0.2839213365162221) {\n"
			"		return 1.26397818871505E+02 * x + 1.56414177335230E+02; // G2\n"
			"	} else if (x < 0.4292115788900552) {\n"
			"		return 4.30655855855847E+02 * x + 7.00288288288335E+01; // G3\n"
			"	} else if (x < 0.5716859888719092) {\n"
			"		return -1.03353736732686E+03 * x + 6.98477513951204E+02; // G4\n"
			"	} else if (x < 0.7167535225418249) {\n"
			"		return -7.29556302521079E+02 * x + 5.24695798319375E+02; // G5\n"
			"	} else if (x < 0.8577494771113141) {\n"
			"		return 6.31469498069341E+02 * x - 4.50824238524115E+02; // G6\n"
			"	} else {\n"
			"		return 7.78344916345091E+01 * x + 2.40558987558802E+01; // G7\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"	if (x < 0.1418174223507038) {\n"
			"		return 5.96339971550497E+02 * x + 1.27334281650071E+02; // B1\n"
			"	} else if (x < 0.2832901246500529) {\n"
			"		return -5.48535392535404E+02 * x + 2.89697554697557E+02; // B2\n"
			"	} else if (x < 0.4021748491445084) {\n"
			"		return 1.22844950213372E+02 * x + 9.95021337126610E+01; // B3\n"
			"	} else if (x < 0.5699636786725797) {\n"
			"		return 1.60703217503224E+02 * x + 8.42764907764923E+01; // B4\n"
			"	} else if (x < 0.7149788470960766) {\n"
			"		return -3.30626890756314E+02 * x + 3.64316806722696E+02; // B5\n"
			"	} else if (x < 0.8584529189762473) {\n"
			"		return -7.30288215340717E+02 * x + 6.50066199802944E+02; // B6\n"
			"	} else {\n"
			"		return 5.42814671814679E+02 * x - 4.42832689832695E+02; // B7\n"
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
