#include "item_palette_store.h"

void ItemPaletteStore::setTileset(std::shared_ptr<Tileset> &&tileset)
{
    _model.setTileset(std::move(tileset));
}
