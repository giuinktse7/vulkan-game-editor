#include "catch.hpp"

#include "../src/position.h"

TEST_CASE("position.h", "[core]")
{
    SECTION("bresenHams should NOT return the 'from' position.")
    {
        // Returns true if the first position (from) is omitted from the result.
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

SCENARIO("Region2D::contains works after changing 'from' or 'to'", "Region2D")
{
    GIVEN("A Region2D")
    {
        Region2D<WorldPosition> region(WorldPosition(50, 50), WorldPosition(100, 100));

        WHEN("from is changed so that from.x > to.x")
        {
            region.setFrom(WorldPosition(200, 50));

            THEN("contains must still work.")
            {
                REQUIRE(region.contains(WorldPosition(150, 75)));
            }
        }

        WHEN("from is changed so that from.y > to.y")
        {
            region.setFrom(WorldPosition(50, 200));

            THEN("contains must still work.")
            {
                REQUIRE(region.contains(WorldPosition(75, 150)));
            }
        }
    }
}