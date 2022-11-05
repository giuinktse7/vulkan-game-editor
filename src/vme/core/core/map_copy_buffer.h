#pragma once

#include "map.h"

class MapCopyBuffer
{
  public:
    /**
     * @return true if anything was copied, or false otherwise.
     */
    bool copySelection(const MapView &mapView);

    /**
     * @return true if anything was cut, or false otherwise.
     */
    bool cutSelection(const MapView &mapView);

    /**
     * @return true if anything was pasted, or false otherwise.
     */
    bool paste(const MapView &mapView);

    void clear();

    Position getPosition() const;
    size_t tileCount() const noexcept;
    const Map &getBufferMap() const;

    bool empty() const noexcept;

    Position topLeft;
    Position bottomRight;

  private:
    Map bufferMap;

    int _tileCount;
    int _itemCount;
};