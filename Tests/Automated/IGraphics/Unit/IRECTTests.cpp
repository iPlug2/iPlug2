#include <catch2/catch_test_macros.hpp>
#include "IGraphicsStructs.h"
using namespace iplug; using namespace igraphics;
TEST_CASE("IRECT basics", "[unit][irect]")
{
  IRECT r {0, 0, 100, 50};
  REQUIRE(r.W() == 100.f);
  REQUIRE(r.Contains(10, 10));
}
