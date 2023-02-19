#pragma once

#include "core/signal.h"
#include "tileset_model.h"

class Brush;

class ItemPaletteStore
{

  public:
    // Signals
    template <auto MemberFunction, typename T>
    void onSelectBrush(T *instance);

    TileSetModel *tilesetModel()
    {
        return &_model;
    }

    void setTileset(std::shared_ptr<Tileset> &&tileset);

  private:
    TileSetModel _model;
};

template <auto MemberFunction, typename T>
void ItemPaletteStore::onSelectBrush(T *instance)
{
    _model.onSelectBrush<MemberFunction, T>(instance);
}
