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

class Rainbow : public Colormap
{
private:
	class Wrapper : public WrapperBase
	{
	public:
		#ifdef float
			#error "TODO"
		#endif
		#define float local_real_t
		#include "../../../../shaders/glsl/IDL_Rainbow.frag"
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
		return std::string("Rainbow");
	}

	std::string getCategory() const override
	{
		return std::string("IDL");
	}

	std::string getSource() const override
	{
		return std::string(
			"float colormap_red(float x) {\n"
			"    if (x < 100.0) {\n"
			"        return (-9.55123422981038E-02 * x + 5.86981763554179E+00) * x - 3.13964093701986E+00;\n"
			"    } else {\n"
			"        return 5.25591836734694E+00 * x - 8.32322857142857E+02;\n"
			"    }\n"
			"}\n"
			"\n"
			"float colormap_green(float x) {\n"
			"    if (x < 150.0) {\n"
			"        return 5.24448979591837E+00 * x - 3.20842448979592E+02;\n"
			"    } else {\n"
			"        return -5.25673469387755E+00 * x + 1.34195877551020E+03;\n"
			"    }\n"
			"}\n"
			"\n"
			"float colormap_blue(float x) {\n"
			"    if (x < 80.0) {\n"
			"        return 4.59774436090226E+00 * x - 2.26315789473684E+00;\n"
			"    } else {\n"
			"        return -5.25112244897959E+00 * x + 8.30385102040816E+02;\n"
			"    }\n"
			"}\n"
			"\n"
			"vec4 colormap(float x) {\n"
			"    float t = x * 255.0;\n"
			"    float r = clamp(colormap_red(t) / 255.0, 0.0, 1.0);\n"
			"    float g = clamp(colormap_green(t) / 255.0, 0.0, 1.0);\n"
			"    float b = clamp(colormap_blue(t) / 255.0, 0.0, 1.0);\n"
			"    return vec4(r, g, b, 1.0);\n"
			"}\n"
		);
	}
};

} // namespace IDL
} // namespace colormap
