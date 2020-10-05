#include "catch.hpp"

#include "../src/position.h"

TEST_CASE("position.h", "[core]")
{
  SECTION("bresenHams should NOT return the 'from' position.")
  {
    // Omits the first position (from)
    auto omitsFirst = [](Position from, Position to) {
      auto result = Position::bresenHams(from, to);
      return std::find(result.begin(), result.end(), from) == result.end();
    };

    REQUIRE(omitsFirst(Position(0, 0, 7), Position(0, 0, 7)));
    REQUIRE(omitsFirst(Position(5, 5, 7), Position(5, 5, 7)));

    // X
    REQUIRE(omitsFirst(Position(0, 0, 7), Position(1, 0, 7)));
    REQUIRE(omitsFirst(Position(1, 0, 7), Position(0, 0, 7)));

    // Y
    REQUIRE(omitsFirst(Position(0, 0, 7), Position(0, 1, 7)));
    REQUIRE(omitsFirst(Position(0, 1, 7), Position(0, 0, 7)));
  }
}