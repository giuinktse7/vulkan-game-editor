#include "editor_action.h"

#include "brushes/brush.h"
#include "brushes/raw_brush.h"
#include "debug.h"
#include "items.h"
#include "selection.h"

MouseAction::MapBrush::MapBrush(Brush *brush)
    : brush(brush) {}

MouseAction::MapBrush::MapBrush(uint32_t serverId)
    : brush(Brush::getOrCreateRawBrush(serverId)) {}

void MouseAction::MapBrush::nextVariation()
{
    int prev = variationIndex;
    variationIndex = (variationIndex + 1) % brush->variationCount();
    if (prev != variationIndex)
    {
        brush->updatePreview(variationIndex);
    }
}

void MouseAction::MapBrush::prevVariation()
{
    int prev = variationIndex;
    if (variationIndex == 0)
    {
        variationIndex = brush->variationCount() - 1;
    }
    else
    {
        variationIndex = (variationIndex - 1) % brush->variationCount();
    }

    if (prev != variationIndex)
    {
        brush->updatePreview(variationIndex);
    }
}

void EditorAction::setRawBrush(uint32_t serverId) noexcept
{
    set(MouseAction::MapBrush(serverId));
}

void EditorAction::setBrush(Brush *brush) noexcept
{
    set(MouseAction::MapBrush(brush));
    brush->updatePreview(0);
}

bool MouseAction::MoveAction::isMoving() const
{
    return moveOrigin.has_value() && moveDelta.has_value() && moveDelta.value() != PositionConstants::Zero;
}

void MouseAction::Select::updateMoveDelta(const Selection &selection, const Position &currentPosition)
{
    DEBUG_ASSERT(moveOrigin.has_value(), "There must be a move origin.");
    DEBUG_ASSERT(!selection.empty(), "The selection should never be empty here.");

    auto delta = currentPosition - moveOrigin.value();
    if (delta == moveDelta)
        return;

    auto topLeftCorrection = selection.getCorner(0, 0, 0).value() + delta;
    delta.x -= std::min(topLeftCorrection.x, 0);
    delta.y -= std::min(topLeftCorrection.y, 0);

    moveDelta = delta;
}

void MouseAction::MoveAction::setMoveOrigin(const Position &origin)
{
    moveOrigin = origin;
    moveDelta = PositionConstants::Zero;
}

void MouseAction::MoveAction::reset()
{
    moveOrigin.reset();
    moveDelta.reset();
}

MouseAction::DragDropItem::DragDropItem(Tile *tile, Item *item)
    : tile(tile), item(item)
{
    moveOrigin = tile->position();
    moveDelta = PositionConstants::Zero;
}

void MouseAction::DragDropItem::updateMoveDelta(const Position &currentPosition)
{
    DEBUG_ASSERT(moveOrigin.has_value(), "There must be a move origin.");

    auto delta = currentPosition - moveOrigin.value();
    if (delta == moveDelta)
        return;

    moveDelta = delta;
}

bool MouseAction::Pan::active() const
{
    return cameraOrigin && mouseOrigin;
}

void MouseAction::Pan::stop()
{
    cameraOrigin.reset();
    mouseOrigin.reset();
}

MouseAction::PasteMapBuffer::PasteMapBuffer(MapCopyBuffer *buffer)
    : buffer(buffer) {}