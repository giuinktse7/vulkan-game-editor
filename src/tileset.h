#pragma once

#include <string>
#include <vector>

#include "brushes/brush.h"
#include "util.h"

class ItemPalette;
class RawBrush;

class Tileset
{
  public:
    Tileset(std::string id);
    Tileset(std::string id, std::string name);

    // Non-copyable
    Tileset(const Tileset &other) = delete;
    Tileset operator&=(const Tileset &other) = delete;

    Tileset(Tileset &&other) = default;
    Tileset &operator=(Tileset &&other) = default;

    void addRawBrush(uint32_t serverId);
    void addBrush(Brush *brush);

    const std::string &name() const noexcept;
    const std::string &id() const noexcept;

    int indexOf(Brush *brush) const;

    Brush *get(size_t index) const;

    ItemPalette *palette() const noexcept;

    size_t size() const noexcept;

    void setPalette(ItemPalette *palette) noexcept;

    void setName(std::string name);

  private:
    bool hasBrush(Brush *brush) const;

    std::string _id;
    std::string _name;
    std::vector<Brush *> brushes;
    vme_unordered_map<Brush *, size_t> brushToIndexMap;

    ItemPalette *_palette;
};