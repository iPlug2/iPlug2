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

class CBSet3 : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_CB-Set3.frag"
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
		return std::string("CB-Set3");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"	if (x < 0.09082479229584027) {\n"
			"		return 1.24879652173913E+03 * x + 1.41460000000000E+02;\n"
			"	} else if (x < 0.1809653122266933) {\n"
			"		return -7.21339920948626E+02 * x + 3.20397233201581E+02;\n"
			"	} else if (x < 0.2715720097177793) {\n"
			"		return 6.77416996047422E+02 * x + 6.72707509881444E+01;\n"
			"	} else if (x < 0.3619607687891861) {\n"
			"		return -1.36850782608711E+03 * x + 6.22886666666710E+02;\n"
			"	} else if (x < 0.4527609316115322) {\n"
			"		return 1.38118774703557E+03 * x - 3.72395256916997E+02;\n"
			"	} else if (x < 0.5472860687991931) {\n"
			"		return -7.81436521739194E+02 * x + 6.06756521739174E+02;\n"
			"	} else if (x < 0.6360981817705944) {\n"
			"		return 8.06836521739242E+02 * x - 2.62483188405869E+02;\n"
			"	} else if (x < 0.8158623444475089) {\n"
			"		return -3.49616157878512E+02 * x + 4.73134258402717E+02;\n"
			"	} else if (x < 0.9098023786863947) {\n"
			"		return 1.72428853754953E+02 * x + 4.72173913043111E+01;\n"
			"	} else {\n"
			"		return 5.44142292490101E+02 * x - 2.90968379446626E+02;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"	if (x < 0.08778161310534617) {\n"
			"		return 4.88563478260870E+02 * x + 2.10796666666667E+02;\n"
			"	} else if (x < 0.2697669137324175) {\n"
			"		return -6.96835646006769E+02 * x + 3.14852913968545E+02;\n"
			"	} else if (x < 0.3622079895714037) {\n"
			"		return 5.40799130434797E+02 * x - 1.90200000000068E+01;\n"
			"	} else if (x < 0.4519795462045253) {\n"
			"		return 3.23774703557373E+01 * x + 1.65134387351785E+02;\n"
			"	} else if (x < 0.5466820192751115) {\n"
			"		return 4.43064347826088E+02 * x - 2.04876811594176E+01;\n"
			"	} else if (x < 0.6368889369442862) {\n"
			"		return -1.83472332015826E+02 * x + 3.22028656126484E+02;\n"
			"	} else if (x < 0.728402572416003) {\n"
			"		return 1.27250988142231E+02 * x + 1.24132411067220E+02;\n"
			"	} else if (x < 0.8187333479165154) {\n"
			"		return -9.82116600790428E+02 * x + 9.32198616600708E+02;\n"
			"	} else if (x < 0.9094607880855196) {\n"
			"		return 1.17713438735149E+03 * x - 8.35652173912769E+02;\n"
			"	} else {\n"
			"		return 2.13339920948864E+01 * x + 2.15502964426857E+02;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"	if (x < 0.09081516507716858) {\n"
			"		return -2.27937391304345E+02 * x + 1.99486666666666E+02;\n"
			"	} else if (x < 0.1809300436999751) {\n"
			"		return 4.33958498023703E+02 * x + 1.39376482213440E+02;\n"
			"	} else if (x < 0.2720053156712806) {\n"
			"		return -1.14300000000004E+03 * x + 4.24695652173923E+02;\n"
			"	} else if (x < 0.3616296568054424) {\n"
			"		return 1.08175889328072E+03 * x - 1.80450592885399E+02;\n"
			"	} else if (x < 0.4537067088757783) {\n"
			"		return -1.22681999999994E+03 * x + 6.54399999999974E+02;\n"
			"	} else if (x < 0.5472726179445029) {\n"
			"		return 8.30770750988243E+01 * x + 6.00909090909056E+01;\n"
			"	} else if (x < 0.6374811920489858) {\n"
			"		return 1.36487351778676E+03 * x - 6.41401185770872E+02;\n"
			"	} else if (x < 0.7237636846906381) {\n"
			"		return -1.27390769230737E+02 * x + 3.09889230769173E+02;\n"
			"	} else if (x < 0.8178226469606309) {\n"
			"		return -3.01831168831021E+02 * x + 4.36142857142782E+02;\n"
			"	} else if (x < 0.9094505664375214) {\n"
			"		return 8.47622811970801E+01 * x + 1.19977978543158E+02;\n"
			"	} else {\n"
			"		return -9.06117391304296E+02 * x + 1.02113405797096E+03;\n"
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
