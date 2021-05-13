#pragma once

#include "../../position.h"
#include "../../signal.h"
#include "../../tracked_item.h"
#include "../draggable_item.h"
#include "container_list_model.h"
#include "container_model.h"

class Item;

namespace PropertiesUI
{
    struct ContainerSignals
    {
        Nano::Signal<void(ContainerModel *)> postOpened;
        Nano::Signal<void(ContainerModel *)> preClosed;
        Nano::Signal<bool(ContainerNode *, int, const ItemDrag::DraggableItem *)> itemDropped;
        Nano::Signal<void(ContainerNode *, int)> itemDragStarted;
    };

    struct ContainerNode : public Nano::Observer<>
    {
        ContainerNode(Item *containerItem, ContainerSignals *_signals);
        ContainerNode(Item *containerItem, ContainerNode *parent);
        ~ContainerNode();

        virtual void setIndexInParent(int index) = 0;

        void onDragFinished(ItemDrag::DragOperation::DropResult result);

        virtual std::unique_ptr<ContainerNode> createChildNode(int index) = 0;
        virtual bool isRoot() const noexcept = 0;

        /*
            When an item is moved in a container with opened child containers,
            the indexInParentContainer of the children might need to be updated.
        */
        void itemMoved(int fromIndex, int toIndex);
        void itemInserted(int index);
        void itemRemoved(int index);

        std::vector<uint16_t> indexChain(int index) const;
        std::vector<uint16_t> indexChain() const;

        ContainerModel *model();

        void open();
        void close();
        void toggle();

        void openChild(int index);
        void toggleChild(int index);

        void itemDropEvent(int index, ItemDrag::DraggableItem *droppedItem);
        void itemDragStartEvent(int index);
        Container *container();
        Item *containerItem() const;

        std::unordered_map<int, std::unique_ptr<ContainerNode>> openedChildrenNodes;

      protected:
        ContainerSignals *_signals;
        std::optional<ContainerModel> _model;

        bool opened = false;

      private:
        void trackedItemChanged(Item *trackedItem);
        void trackedContainerChanged(ContainerChange change);

        TrackedContainer trackedContainerItem;
    };

    struct ContainerTree
    {
        ContainerTree();

        struct Root final : public ContainerNode
        {
            Root(MapView *mapView, Position mapPosition, uint16_t tileIndex, Item *containerItem, ContainerSignals *_signals);

            void setIndexInParent(int index) override;

            std::unique_ptr<ContainerNode> createChildNode(int index) override;
            bool isRoot() const noexcept override
            {
                return true;
            }

          private:
            friend struct ContainerNode;
            Position mapPosition;
            MapView *mapView;
        };

        struct Node final : public ContainerNode
        {
            Node(Item *containerItem, ContainerNode *parent, uint16_t parentIndex);

            void setIndexInParent(int index) override;

            std::unique_ptr<ContainerNode> createChildNode(int index) override;
            bool isRoot() const noexcept override
            {
                return false;
            }

            uint16_t indexInParentContainer;
            ContainerNode *parent;
        };

        const Item *rootItem() const;

        void setRootContainer(MapView *mapView, Position position, uint16_t tileIndex, Item *containerItem);
        bool hasRoot() const noexcept;

        void clear();

        void modelAddedEvent(ContainerModel *model);
        void modelRemovedEvent(ContainerModel *model);

        template <auto MemberFunction, typename T>
        void onContainerItemDrop(T *instance);

        template <auto MemberFunction, typename T>
        void onContainerItemDragStart(T *instance);

        ContainerListModel containerListModel;

      private:
        ContainerSignals _signals;
        std::optional<Root> root;
    };

    template <auto MemberFunction, typename T>
    void ContainerTree::onContainerItemDrop(T *instance)
    {
        _signals.itemDropped.connect<MemberFunction>(instance);
    }

    template <auto MemberFunction, typename T>
    void ContainerTree::onContainerItemDragStart(T *instance)
    {
        _signals.itemDragStarted.connect<MemberFunction>(instance);
    }

} // namespace PropertiesUI
