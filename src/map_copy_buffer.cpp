#include "map_copy_buffer.h"

#include <limits>

#include "map_view.h"

void MapCopyBuffer::clear()
{
    bufferMap.clear();
    _tileCount = 0;
    _itemCount = 0;

    topLeft = PositionConstants::Zero;
    bottomRight = PositionConstants::Zero;
}

size_t MapCopyBuffer::tileCount() const noexcept
{
    return _tileCount;
}

bool MapCopyBuffer::copySelection(const MapView &mapView)
{
    const Selection &selection = mapView.selection();
    if (selection.empty())
    {
        return false;
    }

    clear();

    topLeft = *selection.getCorner(0, 0, 0);
    bottomRight = *selection.getCorner(1, 1, 1);

    for (const auto &pos : selection.allPositions())
    {
        ++_tileCount;

        auto mapTile = mapView.getTile(pos);
        DEBUG_ASSERT(mapTile != nullptr, "No tile at " << pos);

        Tile copiedTile = mapTile->deepCopy(true);
        _itemCount += copiedTile.itemCount();
        bufferMap.insertTile(std::move(copiedTile));
    }

    auto tileInflection = _tileCount == 1 ? "tile" : "tiles";
    auto itemInflection = _itemCount == 1 ? "item" : "items";
    VME_LOG_D("Copied " << _tileCount << " " << tileInflection << " (" << _itemCount << " " << itemInflection << ").");

    return true;
}

bool MapCopyBuffer::empty() const noexcept
{
    return _tileCount == 0;
}

const Map &MapCopyBuffer::getBufferMap() const
{
    return bufferMap;
}
