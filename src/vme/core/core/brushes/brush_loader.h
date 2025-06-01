#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <stack>
#include <string>

#include "../const.h"
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

    BorderBrush parseBorderBrush(const nlohmann::json &brush);
    WallBrush parseWallBrush(const nlohmann::json &brush);
    std::optional<DoodadBrush> parseDoodadBrush(const nlohmann::json &brush);
    std::optional<MountainBrush> parseMountainBrush(const nlohmann::json &brush);

    void logError(std::string message);

    // 12 = BORDER_COUNT_FOR_GROUND_TILE. We don't use it explicitly because then we would need to include border_brush.h here.
    static std::array<uint32_t, 12> parseBorderIds(const nlohmann::json &borderJson); // NOLINT
    static vme_unordered_map<uint32_t, BorderType> parseExtraBorderIds(const nlohmann::json &extrasJson);

    std::stack<std::string> stackTrace;
};

template <typename T>
inline T jsonGetOrDefault(const nlohmann::json &j, const std::string &key, T defaultValue)
{
    if (j.contains(key))
    {
        return j.at(key).get<T>();
    }

    return defaultValue;
}