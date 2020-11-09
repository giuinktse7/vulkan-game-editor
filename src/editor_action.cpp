#include "editor_action.h"

#include "debug.h"
#include "selection.h"

bool MouseAction::Select::isMoving() const
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

void MouseAction::Select::setMoveOrigin(const Position &origin)
{
  moveOrigin = origin;
  moveDelta = PositionConstants::Zero;
}

void MouseAction::Select::reset()
{
  moveOrigin.reset();
  moveDelta.reset();
}