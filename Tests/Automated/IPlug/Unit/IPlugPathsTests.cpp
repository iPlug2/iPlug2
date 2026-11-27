/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

// Unit tests for the platform independent path validation helpers in IPlugPaths.h.

#include <string>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "IPlugPaths.h"

using namespace iplug;

TEST_CASE("PathHasDotDotComponent: detects directory traversal", "[unit][paths]")
{
  SECTION("paths with a .. component")
  {
    const std::string path = GENERATE(as<std::string> {}, "..", "../", "../etc/passwd", "/..", "/../", "a/..", "a/../b",
                                      "/home/user/../other", "/a/b/c/..", "./..", "a//../b", "../..", "x/y/../../z");
    CAPTURE(path);
    REQUIRE(PathHasDotDotComponent(path.c_str()));
  }

  SECTION("paths without one")
  {
    const std::string path = GENERATE(as<std::string> {}, "", "/", "//", ".", "./", "a", "a/b", "/a/b/c", "/home/user/.config",
                                      "...", "/a/.../b", "..a", "a..", "a..b", "/a/..b/c", "/a/b../c", "file..txt",
                                      ". .", ".. ", " ..", "/a/. ./b");
    CAPTURE(path);
    REQUIRE_FALSE(PathHasDotDotComponent(path.c_str()));
  }

  SECTION("a null pointer is not a traversal")
  {
    REQUIRE_FALSE(PathHasDotDotComponent(nullptr));
  }
}

TEST_CASE("IsSafeAbsoluteDir: accepts only absolute paths without traversal", "[unit][paths]")
{
  SECTION("safe")
  {
    const std::string dir = GENERATE(as<std::string> {}, "/", "/home/user", "/home/user/", "/home/user/.config",
                                     "/a/./b", "/a/...", "/..a", "//double/slash");
    CAPTURE(dir);
    REQUIRE(IsSafeAbsoluteDir(dir.c_str()));
  }

  SECTION("unsafe")
  {
    const std::string dir = GENERATE(as<std::string> {}, "", "relative", "relative/path", "./here", "~/home", " /leading/space",
                                     "/..", "/home/../etc", "/home/user/..", "../up", "C:/Windows");
    CAPTURE(dir);
    REQUIRE_FALSE(IsSafeAbsoluteDir(dir.c_str()));
  }

  SECTION("null")
  {
    REQUIRE_FALSE(IsSafeAbsoluteDir(nullptr));
  }
}
