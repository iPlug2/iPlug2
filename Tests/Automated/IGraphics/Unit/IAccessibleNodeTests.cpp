/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Unit tests for IAccessibleNode, the description of a control given to assistive technology.

#include <string>

#include <catch2/catch_test_macros.hpp>

#include "IGraphicsStructs.h"

using namespace iplug;
using namespace igraphics;

namespace
{
IAccessibleNode MakeNode()
{
  IAccessibleNode node;
  node.role = EAccessibleRole::Slider;
  node.label.Set("Gain");
  node.valueText.Set("-6 dB");
  node.description.Set("Output level");
  node.bounds = IRECT(10.f, 20.f, 110.f, 70.f);
  node.hasNumericValue = true;
  node.value = -6.;
  node.min = -70.;
  node.max = 12.;
  node.step = 0.1;
  node.focusable = true;
  return node;
}
} // namespace

TEST_CASE("IAccessibleNode: default construction", "[unit][iaccessiblenode]")
{
  IAccessibleNode node;
  REQUIRE(node.role == EAccessibleRole::None);
  REQUIRE(std::string(node.label.Get()).empty());
  REQUIRE(std::string(node.valueText.Get()).empty());
  REQUIRE(std::string(node.description.Get()).empty());
  REQUIRE(node.bounds.Empty());
  REQUIRE_FALSE(node.hasNumericValue);
  REQUIRE(node.value == 0.);
  REQUIRE(node.min == 0.);
  REQUIRE(node.max == 1.);
  REQUIRE(node.step > 0.);
  REQUIRE(node.toggled == -1);
  REQUIRE(node.expanded == -1);
  REQUIRE_FALSE(node.selected);
  REQUIRE_FALSE(node.disabled);
  REQUIRE_FALSE(node.focusable);
  REQUIRE_FALSE(node.IsInteractive());
}

TEST_CASE("IAccessibleNode: IsInteractive depends on the role", "[unit][iaccessiblenode]")
{
  IAccessibleNode node;

  for (EAccessibleRole role : {EAccessibleRole::Button, EAccessibleRole::Toggle, EAccessibleRole::Slider, EAccessibleRole::ComboBox,
                               EAccessibleRole::RadioButton, EAccessibleRole::Tab, EAccessibleRole::TextInput})
  {
    node.role = role;
    INFO("role " << static_cast<int>(role));
    REQUIRE(node.IsInteractive());
  }

  for (EAccessibleRole role : {EAccessibleRole::None, EAccessibleRole::Group, EAccessibleRole::Label, EAccessibleRole::RadioGroup,
                               EAccessibleRole::TabList, EAccessibleRole::Meter, EAccessibleRole::Image})
  {
    node.role = role;
    INFO("role " << static_cast<int>(role));
    REQUIRE_FALSE(node.IsInteractive());
  }

  // the other properties make no difference
  node.role = EAccessibleRole::Label;
  node.focusable = true;
  node.hasNumericValue = true;
  REQUIRE_FALSE(node.IsInteractive());

  node.role = EAccessibleRole::Button;
  node.disabled = true;
  REQUIRE(node.IsInteractive());
}

TEST_CASE("IAccessibleNode: equality", "[unit][iaccessiblenode]")
{
  const IAccessibleNode node = MakeNode();

  SECTION("identical nodes are equal")
  {
    REQUIRE(node == MakeNode());
    REQUIRE_FALSE(node != MakeNode());
    REQUIRE(IAccessibleNode() == IAccessibleNode());
    REQUIRE(node != IAccessibleNode());
  }

  SECTION("copies are equal")
  {
    const IAccessibleNode copy = node;
    REQUIRE(copy == node);
    REQUIRE(std::string(copy.label.Get()) == "Gain");
    REQUIRE(std::string(copy.valueText.Get()) == "-6 dB");
    REQUIRE(std::string(copy.description.Get()) == "Output level");
  }

  SECTION("a difference in any one member makes nodes unequal")
  {
    IAccessibleNode other = MakeNode();
    other.role = EAccessibleRole::Meter;
    REQUIRE(other != node);
    REQUIRE_FALSE(other == node);

    other = MakeNode();
    other.label.Set("Pan");
    REQUIRE(other != node);

    other = MakeNode();
    other.valueText.Set("-7 dB");
    REQUIRE(other != node);

    other = MakeNode();
    other.description.Set("");
    REQUIRE(other != node);

    other = MakeNode();
    other.bounds = IRECT(10.f, 20.f, 110.f, 71.f);
    REQUIRE(other != node);

    other = MakeNode();
    other.hasNumericValue = false;
    REQUIRE(other != node);

    other = MakeNode();
    other.value = -7.;
    REQUIRE(other != node);

    other = MakeNode();
    other.min = -71.;
    REQUIRE(other != node);

    other = MakeNode();
    other.max = 13.;
    REQUIRE(other != node);

    other = MakeNode();
    other.step = 1.;
    REQUIRE(other != node);

    other = MakeNode();
    other.toggled = 1;
    REQUIRE(other != node);

    other = MakeNode();
    other.expanded = 0;
    REQUIRE(other != node);

    other = MakeNode();
    other.selected = true;
    REQUIRE(other != node);

    other = MakeNode();
    other.disabled = true;
    REQUIRE(other != node);

    other = MakeNode();
    other.focusable = false;
    REQUIRE(other != node);
  }

  SECTION("strings are compared by content")
  {
    IAccessibleNode other = MakeNode();
    other.label.Set("Ga");
    other.label.Append("in");
    REQUIRE(other == node);
  }
}

TEST_CASE("IAccessibleNode: lists of nodes can be compared", "[unit][iaccessiblenode]")
{
  IAccessibleNodeList a {MakeNode(), IAccessibleNode()};
  IAccessibleNodeList b {MakeNode(), IAccessibleNode()};
  REQUIRE(a == b);

  b[1].role = EAccessibleRole::Label;
  REQUIRE(a != b);

  b.pop_back();
  REQUIRE(a != b);
}
