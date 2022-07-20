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

class BluePastelRed : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_Blue-Pastel-Red.frag"
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
		return std::string("Blue-Pastel-Red");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"	if (x < 0.1131206452846527) {\n"
			"		return (-9.40943766883858E+02 * x - 1.84146720562529E+02) * x + 3.28713709677420E+01;\n"
			"	} else if (x < 0.5116005837917328) {\n"
			"		return 0.0;\n"
			"	} else if (x < 0.5705677568912506) {\n"
			"		return (-2.22507913165263E+03 * x + 2.76053354341733E+03) * x - 8.29909138655453E+02;\n"
			"	} else if (x < 0.622047244) {\n"
			"		return (-1.84774532967032E+04 * x + 2.30647002747253E+04) * x - 7.12389120879120E+03;\n"
			"	} else if (x < 0.7922459542751312) {\n"
			"		return ((((1.29456468589020E+06 * x - 4.64095889653844E+06) * x + 6.62951004830418E+06) * x - 4.71587036142377E+06) * x + 1.67048886368434E+06) * x - 2.35682532934682E+05;\n"
			"	} else {\n"
			"		return 3.34889230769210E+02 * x - 1.41006123680226E+02;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"	if (x < 0.114394336938858) {\n"
			"		return 0.0;\n"
			"	} else if (x < 0.4417250454425812) {\n"
			"		return (9.43393359191585E+02 * x + 1.86774918014536E+02) * x - 3.37113020096108E+01;\n"
			"	} else if (x < 0.4964917968308496) {\n"
			"		return 3.11150000000070E+02 * x + 9.54249999999731E+01;\n"
			"	} else if (x < 0.6259051214039278) {\n"
			"		return -1.03272635599706E+03 * x + 7.62648586707481E+02;\n"
			"	} else if (x < 0.8049814403057098) {\n"
			"		return -2.92799028677160E+02 * x + 2.99524283071235E+02;\n"
			"	} else {\n"
			"		return (1.34145201311283E+03 * x - 2.75066701126586E+03) * x + 1.40880802982723E+03;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"	if (x < 0.4424893036638088) {\n"
			"		return 3.09636968527514E+02 * x + 9.62203074056821E+01;\n"
			"	} else if (x < 0.5) {\n"
			"		return -4.59921428571535E+02 * x + 4.36741666666678E+02;\n"
			"	} else if (x < 0.5691165986930345) {\n"
			"		return -1.81364912280674E+03 * x + 1.05392982456125E+03;\n"
			"	} else if (x < 0.6279306709766388) {\n"
			"		return 1.83776470588197E+02 * x - 8.28382352940910E+01;\n"
			"	} else {\n"
			"		return ((-1.14087926835422E+04 * x + 2.47091243363548E+04) * x - 1.80428756181930E+04) * x + 4.44421976986281E+03;\n"
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
