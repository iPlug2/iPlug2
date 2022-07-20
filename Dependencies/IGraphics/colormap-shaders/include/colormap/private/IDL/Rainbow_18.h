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

class Rainbow18 : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_Rainbow_18.frag"
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
		return std::string("Rainbow_18");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"vec4 colormap(float x) {\n"
			"	float x16 = x * 16.0;\n"
			"	const float a = 255.0;\n"
			"	const float s = 1.0 / 255.0;\n"
			"	if (x16 < 1.0) {\n"
			"		return vec4(150.0, 0.0, 150.0, a) * s;\n"
			"	} else if (x16 < 2.0) {\n"
			"		return vec4(200.0, 0.0, 200.0, a) * s;\n"
			"	} else if (x16 < 3.0) {\n"
			"		return vec4(100.0, 100.0, 150.0, a) * s;\n"
			"	} else if (x16 < 4.0) {\n"
			"		return vec4(100.0, 100.0, 200.0, a) * s;\n"
			"	} else if (x16 < 5.0) {\n"
			"		return vec4(100.0, 100.0, 255.0, a) * s;\n"
			"	} else if (x16 < 6.0) {\n"
			"		return vec4(0.0, 140.0, 0.0, a) * s;\n"
			"	} else if (x16 < 7.0) {\n"
			"		return vec4(150.0, 170.0, 0.0, a) *s;\n"
			"	} else if (x16 < 8.0) {\n"
			"		return vec4(200.0, 200.0, 0.0, a) * s;\n"
			"	} else if (x16 < 9.0) {\n"
			"		return vec4(150.0, 200.0, 0.0, a) * s;\n"
			"	} else if (x16 < 10.0) {\n"
			"		return vec4(200.0, 255.0, 120.0, a) * s;\n"
			"	} else if (x16 < 11.0) {\n"
			"		return vec4(255.0, 255.0, 0.0, a) * s;\n"
			"	} else if (x16 < 12.0) {\n"
			"		return vec4(255.0, 200.0, 0.0, a) * s;\n"
			"	} else if (x16 < 13.0) {\n"
			"		return vec4(255.0, 160.0, 0.0, a) * s;\n"
			"	} else if (x16 < 14.0) {\n"
			"		return vec4(255.0, 125.0, 0.0, a) * s;\n"
			"	} else if (x16 < 15.0) {\n"
			"		return vec4(200.0, 50.0, 100.0, a) * s;\n"
			"	} else {\n"
			"		return vec4(175.0, 50.0, 75.0, a) * s;\n"
			"	}\n"
			"}\n"
		);
	}
};

} // namespace IDL
} // namespace colormap
