#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <stack>
#include <string>

#include "../const.h"
#include "../tileset.h"
#include "../util.h"

class BorderBrush;
class GroundBrush;
class WallBrush;
class DoodadBrush;
class MountainBrush;
struct BorderRuleAction;

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

    void parseBorderRules(BorderBrush &brush, const nlohmann::json &ruleJson);
    std::unique_ptr<BorderRuleAction> parseBorderRuleAction(const nlohmann::json &actionJson);

    BorderBrush parseBorderBrush(const nlohmann::json &borderJson);
    WallBrush parseWallBrush(const nlohmann::json &wallJson);
    std::optional<DoodadBrush> parseDoodadBrush(const nlohmann::json &doodadJson);
    std::optional<MountainBrush> parseMountainBrush(const nlohmann::json &mountainJson);

    void logError(std::string message);

    static std::array<uint32_t, 12> parseBorderIds(const nlohmann::json &borderJson);
    static vme_unordered_map<uint32_t, BorderType> parseExtraBorderIds(const nlohmann::json &extrasJson);

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