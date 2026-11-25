#include <catch2/catch_test_macros.hpp>
#include "IPlugQueue.h"
using namespace iplug;
TEST_CASE("IPlugQueue push/pop", "[unit][queue]")
{
  IPlugQueue<int> q {4};
  REQUIRE(q.ElementsAvailable() == 0);
  REQUIRE(q.Push(1));
  int v = 0;
  REQUIRE(q.Pop(v));
  REQUIRE(v == 1);
}
