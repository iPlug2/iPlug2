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

class SixteenLevel : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_16_Level.frag"
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
		return std::string("16_Level");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"vec4 colormap(float x) {\n"
			"    if (x < 0.0) {\n"
			"        return vec4(0.0, 0.0, 0.0, 1.0);\n"
			"    } else if (1.0 < x) {\n"
			"        return vec4(1.0, 1.0, 1.0, 1.0);\n"
			"    } else if (x < 1.0 / 16.0) {\n"
			"        return vec4(0.0, 84.0 / 255.0, 0.0, 1.0);\n"
			"    } else if (x < 2.0 / 16.0) {\n"
			"        return vec4(0.0, 168.0 / 255.0, 0.0, 1.0);\n"
			"    } else if (x < 3.0 / 16.0) {\n"
			"        return vec4(0.0, 1.0, 0.0, 1.0);\n"
			"    } else if (x < 4.0 / 16.0) {\n"
			"        return vec4(0.0, 1.0, 84.0 / 255.0, 1.0);\n"
			"    } else if (x < 5.0 / 16.0) {\n"
			"        return vec4(0.0, 1.0, 168.0 / 255.0, 1.0);\n"
			"    } else if (x < 6.0 / 16.0) {\n"
			"        return vec4(0.0, 1.0, 1.0, 1.0);\n"
			"    } else if (x < 7.0 / 16.0) {\n"
			"        return vec4(0.0, 0.0, 1.0, 1.0);\n"
			"    } else if (x < 8.0 / 16.0) {\n"
			"        return vec4(128.0 / 255.0, 0.0, 1.0, 1.0);\n"
			"    } else if (x < 9.0 / 16.0) {\n"
			"        return vec4(1.0, 0.0, 220.0 / 255.0, 1.0);\n"
			"    } else if (x < 10.0 / 16.0) {\n"
			"        return vec4(1.0, 0.0, 180.0 / 255.0, 1.0);\n"
			"    } else if (x < 11.0 / 16.0) {\n"
			"        return vec4(1.0, 0.0, 128.0 / 255.0, 1.0);\n"
			"    } else if (x < 12.0 / 16.0) {\n"
			"        return vec4(1.0, 0.0, 64.0 / 255.0, 1.0);\n"
			"    } else if (x < 13.0 / 16.0) {\n"
			"        return vec4(1.0, 0.0, 0.0, 1.0);\n"
			"    } else if (x < 14.0 / 16.0) {\n"
			"        return vec4(220.0 / 255.0, 190.0 / 255.0, 190.0 / 255.0, 1.0);\n"
			"    } else if (x < 15.0 / 16.0) {\n"
			"        return vec4(220.0 / 255.0, 220.0 / 255.0, 220.0 / 255.0, 1.0);\n"
			"    } else {\n"
			"        return vec4(1.0, 1.0, 1.0, 1.0);\n"
			"    }\n"
			"}\n"
		);
	}
};

} // namespace IDL
} // namespace colormap
