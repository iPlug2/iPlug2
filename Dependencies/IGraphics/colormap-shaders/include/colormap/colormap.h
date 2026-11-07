#pragma once
#include <cassert>
#include <cmath>
#include <algorithm>
#include <vector>
#include <string>
#include <memory>

namespace colormap
{

struct Color
{
	double r, g, b, a;
};

class Colormap
{
public:
	virtual ~Colormap()
	{}

	virtual Color getColor(double x) const = 0;

	virtual std::string getTitle() const = 0;

	virtual std::string getCategory() const = 0;

	virtual std::string getSource() const = 0;

protected:
	struct vec4
	{
		vec4(double a0, double a1, double a2, double a3)
			: x(a0)
			, y(a1)
			, z(a2)
			, w(a3)
		{
		}

		double operator[](size_t index) const
		{
			assert(index < 4);
			if (index < 2) {
				if (index < 1) {
					return x;
				} else {
					return y;
				}
			} else {
				if (index < 3) {
					return z;
				} else {
					return w;
				}
			}
		}

		double& operator[](size_t index)
		{
			assert(index < 4);
			if (index < 2) {
				if (index < 1) {
					return x;
				} else {
					return y;
				}
			} else {
				if (index < 3) {
					return z;
				} else {
					return w;
				}
			}
		}

		bool operator==(vec4 const& o) const
		{
			return x == o.x && y == o.y && z == o.z && w == o.w;
		}

		vec4 operator*(double v) const
		{
			return vec4(r * v, g * v, b * v, a * v);
		}

		vec4 operator+(vec4 const& v) const
		{
			return vec4(r + v.r, g + v.g, b + v.b, a + v.a);
		}

		std::string to_string() const
		{
			return
				std::string("{") +
				std::to_string(x) + std::string(",") +
				std::to_string(y) + std::string(",") +
				std::to_string(z) + std::string(",") +
				std::to_string(w) +
				std::string("}");
		}

		union {
			double r;
			double x;
		};
		union {
			double g;
			double y;
		};
		union {
			double b;
			double z;
		};
		union {
			double a;
			double w;
		};
	};

	class WrapperBase
	{
	public:
		virtual ~WrapperBase()
		{}

	protected:
		typedef double local_real_t;

		template <class Value, class MinMax>
		typename std::common_type<Value, MinMax>::type clamp(Value v, MinMax min, MinMax max) const
		{
			if (v < min) {
				return min;
			} else if (max < v) {
				return max;
			} else {
				return v;
			}
		}

		template <class Value>
		Value sign(Value v) const
		{
			if (v < (Value)0) {
				return (Value)-1;
			} else if ((Value)0 < v) {
				return (Value)1;
			} else {
				return (Value)0;
			}
		}

		local_real_t abs(local_real_t v) const
		{
			return std::fabs(v);
		}

		local_real_t mod(local_real_t x, local_real_t y) const
		{
			return std::fmod(x, y);
		}
	};
};

} // namespace colormap

#include "./private/all_colormaps.h"

namespace colormap
{
class ColormapList
{
public:
	static std::vector<std::shared_ptr<Colormap const>> getAll()
	{
		return {
			#include "./private/init_colormap_list.inc"
		};
	}

private:
	ColormapList()
	{
	}

	ColormapList(ColormapList const&) = delete;
	ColormapList(ColormapList&&) = delete;
	ColormapList& operator=(ColormapList const&) = delete;
	ColormapList& operator=(ColormapList&&) = delete;
};

} // namespace colormap
