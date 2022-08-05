#include "catch.hpp"

#include <optional>

#include "common/item.h"
#include "common/items.h"

struct ItemObserver
{
    void onAddressChanged(Item *item)
    {
        receivedAddressChange = true;
    }

    void onPropertyChanged(Item *item, ItemChangeType changeType)
    {
        latestPropertyChange = changeType;
    }

    bool receivedAddressChange = false;
    std::optional<ItemChangeType> latestPropertyChange;
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

TEST_CASE("observable_item.h", "[observer][item]")
{
    SECTION("Item signals are cleared when no longer needed")
    {
        {
            Item item(2554);

            {
                ItemObserver observer;

                auto tracked = ObservableItem(&item);
                tracked.onAddressChanged<&ItemObserver::onAddressChanged>(&observer);
                tracked.onPropertyChanged<&ItemObserver::onPropertyChanged>(&observer);

                REQUIRE(Items::items.itemSignals.size() == 1);

                Item i2 = item.deepCopy();

                SECTION("Item address signal works")
                {
                    Items::items.itemAddressChanged(&i2);
                    REQUIRE(observer.receivedAddressChange);
                }

                SECTION("Item property signal works for count")
                {
                    item.setCount(25);
                    bool ok = observer.latestPropertyChange.has_value() && (*observer.latestPropertyChange) == ItemChangeType::Count;
                    REQUIRE(ok);
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
