#include "catch.hpp"

#include <optional>

#include "../src/item.h"
#include "../src/items.h"

struct ItemObserver
{
    void onChanged(Item *item)
    {
        receivedChange = true;
    }

    bool receivedChange = false;
};

struct ContainerObserver
{
    void onContainerChanged(ContainerChange change)
    {
        latestChange = change;
    }

    bool latestChangeIsOfType(ContainerChangeType type)
    {
        return latestChange.has_value() && latestChange.value().type == type;
    }

    std::optional<ContainerChange> latestChange;
};

TEST_CASE("tracked_item.h", "[observer][item]")
{
    SECTION("Item signals are cleared when no longer needed")
    {
        {
            Item item(2554);

            {
                ItemObserver observer;

                auto tracked = TrackedItem(&item);
                tracked.onChanged<&ItemObserver::onChanged>(&observer);

                REQUIRE(Items::items.itemSignals.size() == 1);

                Item i2 = item.deepCopy();

                SECTION("Item signal works")
                {
                    Items::items.itemMoved(&i2);
                    REQUIRE(observer.receivedChange);
                }
            }

            SECTION("Item signal is correctly cleared")
            {
                REQUIRE(Items::items.itemSignals.size() == 0);
            }
        }
    }

    SECTION("Container signals are cleared when no longer needed")
    {
        Item item(1987);
        auto container = item.getOrCreateContainer();
        container->addItem(Item(2148));
        container->addItem(Item(2554));

        {
            ContainerObserver observer;

            auto tracked = TrackedContainer(&item);
            tracked.onContainerChanged<&ContainerObserver::onContainerChanged>(&observer);

            SECTION("Container signal works")
            {
                container->dropItemTracked(1);

                REQUIRE(observer.latestChangeIsOfType(ContainerChangeType::Remove));
            }
        }

        SECTION("Item Container signal is correctly cleared")
        {
            REQUIRE(Items::items.containerSignals.size() == 0);
        }
    }
}
