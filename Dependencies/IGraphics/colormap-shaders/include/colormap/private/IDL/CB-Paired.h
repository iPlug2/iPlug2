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

class CBPaired : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_CB-Paired.frag"
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
		return std::string("CB-Paired");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"	if (x < 0.0906416957946237) {\n"
			"		return -1.48766695652174E+03 * x + 1.65896666666667E+02;\n"
			"	} else if (x < 0.181137969063194) {\n"
			"		return 1.62263833992095E+03 * x - 1.16026679841898E+02;\n"
			"	} else if (x < 0.2716806139960391) {\n"
			"		return -1.40227075098820E+03 * x + 4.31899209486178E+02;\n"
			"	} else if (x < 0.3621693275308975) {\n"
			"		return 2.21145652173927E+03 * x - 5.49880434782653E+02;\n"
			"	} else if (x < 0.4514347780510689) {\n"
			"		return -2.73075098814252E+02 * x + 3.49940711462467E+02;\n"
			"	} else if (x < 0.5478389816716595) {\n"
			"		return 2.75424347826088E+02 * x + 1.02328985507251E+02;\n"
			"	} else if (x < 0.6384253260915684) {\n"
			"		return 1.95770750987722E+01 * x + 2.42492094861655E+02;\n"
			"	} else if (x < 0.7280391465804739) {\n"
			"		return -5.92081027667844E+02 * x + 6.32990118576982E+02;\n"
			"	} else if (x < 0.8191050219893012) {\n"
			"		return -1.05189130434770E+03 * x + 9.67749999999916E+02;\n"
			"	} else if (x < 0.9092300295745469) {\n"
			"		return 1.64974505928811E+03 * x - 1.24517391304309E+03;\n"
			"	} else {\n"
			"		return -8.20731225296366E+02 * x + 1.00105731225287E+03;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"	if (x < 0.09069203671589791) {\n"
			"		return -9.49076521739127E+02 * x + 2.05970000000000E+02;\n"
			"	} else if (x < 0.1811205395903491) {\n"
			"		return 1.14400395256917E+03 * x + 1.61442687747026E+01;\n"
			"	} else if (x < 0.271076794014141) {\n"
			"		return -7.04272727272755E+02 * x + 3.50905138339931E+02;\n"
			"	} else if (x < 0.3618506954718166) {\n"
			"		return -6.35000000000221E+01 * x + 1.77206521739141E+02;\n"
			"	} else if (x < 0.4527247821743651) {\n"
			"		return -1.40603557312254E+03 * x + 6.63003952569178E+02;\n"
			"	} else if (x < 0.5472660653935183) {\n"
			"		return 1.73713913043494E+03 * x - 7.59989130434857E+02;\n"
			"	} else if (x < 0.6379975539161487) {\n"
			"		return -7.00507905138483E+02 * x + 5.74052371541584E+02;\n"
			"	} else if (x < 0.7283304980067641) {\n"
			"		return 5.64723320158019E+02 * x - 2.33162055335916E+02;	} else if (x < 0.8189077039268755) {\n"
			"		return -1.29283992094844E+03 * x + 1.11975790513821E+03;	} else if (x < 0.9094178747563795) {\n"
			"		return 2.14293675889271E+03 * x - 1.69382608695601E+03;\n"
			"	} else {\n"
			"		return -1.75290118577070E+03 * x + 1.84911857707505E+03;\n"
			"	}\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"	if (x < 0.1835817221386023) {\n"
			"		return -4.93278367346940E+02 * x + 2.25853877551021E+02;\n"
			"	} else if (x < 0.2718482976477959) {\n"
			"		return -1.04124223602495E+03 * x + 3.26450028232661E+02;\n"
			"	} else if (x < 0.3623519200472859) {\n"
			"		return 1.21151976284592E+03 * x - 2.85959486166031E+02;\n"
			"	} else if (x < 0.4526344257525674) {\n"
			"		return -1.38645849802374E+03 * x + 6.55422924901199E+02;\n"
			"	} else if (x < 0.5474992417588231) {\n"
			"		return 8.80275652173975E+02 * x - 3.70578985507278E+02;\n"
			"	} else if (x < 0.6375259518892261) {\n"
			"		return -1.24038339920972E+03 * x + 7.90480237154278E+02;\n"
			"	} else if (x < 0.7280438873117513) {\n"
			"		return 2.36255138339872E+03 * x - 1.50648418972297E+03;\n"
			"	} else if (x < 0.8192397843702398) {\n"
			"		return -6.51816205533491E+02 * x + 6.88107707509788E+02;\n"
			"	} else if (x < 0.9092328860678134) {\n"
			"		return -1.35533596837590E+01 * x + 1.65217391304318E+02;\n"
			"	} else {\n"
			"		return -1.19420158102770E+03 * x + 1.23870158102770E+03;\n"
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
