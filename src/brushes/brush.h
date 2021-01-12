#pragma once

#include <memory>
#include <string>

#include "../util.h"

class Tileset;

enum class BrushType
{
    RawItem
};

class Brush
{
  public:
    Brush(std::string name);

    virtual uint32_t iconServerId() const = 0;

    const std::string &name() const noexcept;

    virtual bool erasesItem(uint32_t serverId) const = 0;
    virtual BrushType type() const = 0;

    static Brush *getOrCreateRawBrush(uint32_t serverId);

    void setTileset(Tileset *tileset) noexcept;
    Tileset *tileset() const noexcept;

  private:
    static vme_unordered_map<uint32_t, std::unique_ptr<Brush>> rawBrushes;

    std::string _name;
    Tileset *_tileset = nullptr;
};