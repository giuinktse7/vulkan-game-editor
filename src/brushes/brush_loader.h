#pragma once

#include <filesystem>
#include <nlohmann/json.hpp>
#include <stack>
#include <string>

#include "../tileset.h"
#include "ground_brush.h"

class BrushLoader
{
  public:
    bool load(std::filesystem::path path);

  private:
    void parseBrushes(const nlohmann::json &brushesJson);
    void parseTilesets(const nlohmann::json &tilesetsJson);

    GroundBrush parseGroundBrush(const nlohmann::json &groundJson);
    void parseTileset(const nlohmann::json &tilesetJson);
    void parsePalettes(const nlohmann::json &paletteJson);

    void parseCreatures(const nlohmann::json &creaturesJson);
    void parseCreature(const nlohmann::json &creatureJson);

    std::stack<std::string> stackTrace;
};