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

class CBGreys : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_CB-Greys.frag"
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
		return std::string("CB-Greys");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_f1(float x) {\n"
			"	if (x < 0.3849871446504941) {\n"
			"		return (-1.97035589869658E+02 * x - 1.04694505989261E+02) * x + 2.54887830314633E+02;\n"
			"	} else if (x < 0.7524552013985151) {\n"
			"		return (8.71964614639801E+01 * x - 3.79941007690502E+02) * x + 3.18726712728548E+02;\n"
			"	} else {\n"
			"		return (2.28085532626505E+02 * x - 7.25770100421835E+02) * x + 4.99177793972139E+02;\n"
			"	}\n"
			"}\n"
			"\n"
			"vec4 colormap(float x) {\n"
			"	float v = clamp(colormap_f1(x) / 255.0, 0.0, 1.0);\n"
			"	return vec4(v, v, v, 1.0);\n"
			"}\n"
		);
	}
};

} // namespace IDL
} // namespace colormap
