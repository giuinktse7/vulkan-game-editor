#include "catch.hpp"

#include "core/item.h"

TEST_CASE("item.h", "[core][item]")
{
    SECTION("Items can be compared for equality")
    {
        Item base(2148);

        SECTION("Equality considers serverId")
        {
            Item a(2148);
            Item b(2500);

            REQUIRE(base == a);
            REQUIRE(base != b);
        }

        SECTION("Equality considers attributes")
        {
            Item a(2148);

            base.setActionId(1000);
            REQUIRE(base != a);

            a.setActionId(1000);
            REQUIRE(base == a);

            base.setText("Some text");
            REQUIRE(base != a);

            a.setText("Some text");
            REQUIRE(base == a);
        }
    }
}
