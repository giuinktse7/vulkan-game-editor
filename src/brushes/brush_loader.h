#pragma once

#include <filesystem>
#include <nlohmann/json.hpp>
#include <optional>
#include <stack>
#include <string>

#include "../tileset.h"

class BorderBrush;
class GroundBrush;
class WallBrush;
class DoodadBrush;
class MountainBrush;
struct LazyGroundBrush;

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

    BorderBrush parseBorderBrush(const nlohmann::json &borderJson);
    WallBrush parseWallBrush(const nlohmann::json &wallJson);
    std::optional<DoodadBrush> parseDoodadBrush(const nlohmann::json &doodadJson);
    std::optional<MountainBrush> parseMountainBrush(const nlohmann::json &mountainJson);

    void logError(std::string message);

    static LazyGroundBrush fromJson(const nlohmann::json &json);

    std::stack<std::string> stackTrace;
};

template <typename T>
inline T jsonGetOrDefault(const nlohmann::json &j, std::string key, T defaultValue)
{
    if (j.contains(key))
    {
        return j.at(key).get<T>();
    }
    else
    {
        return defaultValue;
    }
}