#include "brush_loader.h"

#include <format>
#include <fstream>
#include <sstream>
#include <stack>

#include "../debug.h"
#include "../item_palette.h"
#include "../items.h"
#include "../logger.h"
#include "../time_util.h"
#include "border_brush.h"
#include "brush.h"
#include "doodad_brush.h"
#include "ground_brush.h"
#include "mountain_brush.h"
#include "raw_brush.h"
#include "wall_brush.h"

using json = nlohmann::json;

namespace JsonParseMethods
{
    Brush::LazyGround parseLazyGround(const json &json)
    {
        if (json.is_string())
        {
            return Brush::LazyGround(json.get<std::string>());
        }
        else
        {
            throw json::type_error::create(302, "Invalid ground ID.");
        }
    }

    BorderStackBehavior parseBorderStackBehavior(const json &json)
    {
        if (json.is_string())
        {
            std::string stackBehavior = json.get<std::string>();
            switch (string_hash(stackBehavior.c_str()))
            {
                case "default"_sh:
                    return BorderStackBehavior::Default;
                case "clear"_sh:
                    return BorderStackBehavior::Clear;
                case "fullGround"_sh:
                    return BorderStackBehavior::FullGround;
                default:
                    throw json::type_error::create(302, std::format("Invalid stack behavior: {}", stackBehavior));
            }
        }
        else
        {
            throw json::type_error::create(302, "Invalid stack behavior.");
        }
    }

    BorderType parseBorderType(const json &json)
    {
        if (json.is_string())
        {
            std::string rawBorderType = json.get<std::string>();
            switch (string_hash(rawBorderType.c_str()))
            {
                case "n"_sh:
                    return BorderType::North;
                case "e"_sh:
                    return BorderType::East;
                case "s"_sh:
                    return BorderType::South;
                case "w"_sh:
                    return BorderType::West;
                case "csw"_sh:
                    return BorderType::SouthWestCorner;
                case "cse"_sh:
                    return BorderType::SouthEastCorner;
                case "cnw"_sh:
                    return BorderType::NorthWestCorner;
                case "cne"_sh:
                    return BorderType::NorthEastCorner;
                case "dsw"_sh:
                    return BorderType::SouthWestDiagonal;
                case "dse"_sh:
                    return BorderType::SouthEastDiagonal;
                case "dnw"_sh:
                    return BorderType::NorthWestDiagonal;
                case "dne"_sh:
                    return BorderType::NorthEastDiagonal;
                default:
                    throw json::type_error::create(302, "parseBorderType: Invalid BorderType. Expected std::string but got something else.");
            }
        }
        else
        {
            throw json::type_error::create(302, "parseBorderType: Invalid BorderType. Expected std::string but got something else.");
        }
    }
} // namespace JsonParseMethods

using namespace JsonParseMethods;

std::string joinStack(std::stack<std::string> &stack, std::string delimiter)
{
    auto stackCopy = stack;
    std::stack<std::string> reversed;
    while (!stackCopy.empty())
    {
        reversed.push(stackCopy.top());
        stackCopy.pop();
    }
    std::ostringstream ss;
    while (!reversed.empty())
    {
        ss << reversed.top();
        reversed.pop();
        if (!reversed.empty())
            ss << delimiter;
    }

    return ss.str();
}

json asArray(const json &j, std::string key)
{
    if (!j.contains(key))
        return json{};

    json value = j.at(key);
    return value.is_array() ? value : json{};
}

std::string getString(const json &j, std::string key)
{
    return j[key].get<std::string>();
}

int getInt(const json &j, std::string key)
{
    auto value = j[key];
    if (!value.is_number_integer())
    {
        throw json::type_error::create(302, std::format("The value at key '{}' has to be an integer (it was a '{}').", key, std::string(value.type_name())));
    }

    return value.get<int>();
}

int getIntOrElse(const json &j, std::string key, uint32_t fallback)
{
    auto value = j[key];
    return value.is_number_integer() ? value.get<int>() : 0;
}

bool BrushLoader::load(std::filesystem::path path)
{
    TimePoint start;

    if (!std::filesystem::exists(path))
    {
        VME_LOG_ERROR(std::format("Could not find file '{}'", path.string()));
        return false;
    }

    std::ifstream fileStream(path);
    json rootJson = json::parse(fileStream, nullptr, true, true);
    fileStream.close();

    auto topTrace = stackTrace;

    try
    {
        auto palettes = asArray(rootJson, "palettes");
        if (!palettes.is_null())
        {
            parsePalettes(palettes);
        }

        auto brushes = asArray(rootJson, "brushes");
        if (!brushes.is_null())
        {
            parseBrushes(brushes);
        }

        auto tilesets = asArray(rootJson, "tilesets");
        if (!tilesets.is_null())
        {
            parseTilesets(tilesets);
        }

        auto creatures = asArray(rootJson, "creatures");
        if (!creatures.is_null())
        {
            parseCreatures(creatures);
        }
    }
    catch (json::exception &exception)
    {
        std::string msg = joinStack(stackTrace, " -> ");
        VME_LOG_D(msg << ": " << exception.what());
        return false;
    }

    VME_LOG("Loaded brushes in " << start.elapsedMillis() << " ms.");

    return true;
}

void BrushLoader::logError(std::string message)
{
    std::string stack = joinStack(stackTrace, " -> ");
    VME_LOG_ERROR(std::format("{}: {}", stack, message));
}

void BrushLoader::parseBrushes(const nlohmann::json &brushesJson)
{
    stackTrace.emplace("/brushes");
    auto topTrace = stackTrace;

    for (const json &brush : brushesJson)
    {
        stackTrace = topTrace;

        if (!brush.contains("id"))
        {
            throw json::other_error::create(403, std::format("A brush is missing an id (all brushes must have an id). Add an id to this brush: {}", brush.dump(4)));
        }

        stackTrace.emplace(std::format("Brush '{}'", brush.at("id").get<std::string>()));

        auto brushType = Brush::parseBrushType(getString(brush, "type"));
        if (!brushType)
        {
            throw json::type_error::create(302, std::format("The type must be one of ['ground', 'doodad', 'wall', 'border']."));
        }

        switch (*brushType)
        {
            case BrushType::Ground:
            {
                auto groundBrush = parseGroundBrush(brush);
                Brush::addGroundBrush(std::move(groundBrush));
                break;
            }
            case BrushType::Border:
            {
                auto borderBrush = parseBorderBrush(brush);
                Brush::addBorderBrush(std::move(borderBrush));
                break;
            }
            case BrushType::Wall:
            {
                auto wallBrush = parseWallBrush(brush);
                Brush::addWallBrush(std::move(wallBrush));
                break;
            }
            case BrushType::Doodad:
            {
                auto doodadBrush = parseDoodadBrush(brush);
                if (doodadBrush.has_value())
                {
                    Brush::addDoodadBrush(std::move(doodadBrush.value()));
                }
                break;
            }
            case BrushType::Mountain:
            {
                auto mountainBrush = parseMountainBrush(brush);
                if (mountainBrush.has_value())
                {
                    Brush::addMountainBrush(std::move(mountainBrush.value()));
                }
                break;
            }

            default:
                break;
        }

        stackTrace.pop();
    }
    stackTrace.pop();
}

std::optional<DoorType> getDoorType(std::string raw)
{
    switch (string_hash(raw.c_str()))
    {
        case "normal"_sh:
            return DoorType::Normal;
        case "locked"_sh:
            return DoorType::Locked;
        case "quest"_sh:
            return DoorType::Quest;
        case "magic"_sh:
            return DoorType::Magic;
        default:
            return std::nullopt;
    }
}

std::optional<WindowType> getWindowType(std::string raw)
{
    switch (string_hash(raw.c_str()))
    {
        case "normal"_sh:
        case "window"_sh:
            return WindowType::Normal;
        case "hatch"_sh:
        case "hatch_window"_sh:
            return WindowType::Hatch;
        default:
            return std::nullopt;
    }
}

WallBrush BrushLoader::parseWallBrush(const json &brush)
{
    std::string id = brush.at("id").get<std::string>();
    std::string name = brush.at("name").get<std::string>();

    int lookId = getInt(brush, "lookId");

    WallBrush::StraightPart horizontalPart;
    WallBrush::StraightPart verticalPart;
    WallBrush::Part cornerPart;
    WallBrush::Part polePart;

    /*
        Accepted formats:
            - "root": <integer>
            - "root": [<integer>|{"id": <integer>, "chance": <integer>}]
    */
    auto parseItems = [this](const json &root, WallBrush::Part *part) {
        if (root.is_number_integer())
        {
            int id = root.get<int>();
            part->items.emplace_back(id, 1);
        }
        else
        {
            for (auto &item : root)
            {
                if (item.is_number_integer())
                {
                    int id = item.get<int>();
                    part->items.emplace_back(id, 1);
                }

                int id = getInt(item, "id");
                int chance = getInt(item, "chance");

                part->items.emplace_back(id, chance);
            }
        }
    };

    auto parseStraightPart = [this, &parseItems](const json &root, WallBrush::StraightPart &part) {
        std::vector<WeightedItemId> weightedIds;

        stackTrace.emplace("items");
        {
            json items = root.at("items");

            parseItems(items, &part);
        }
        stackTrace.pop();

        stackTrace.emplace("doors");
        {
            json doors = root.at("doors");

            for (auto &door : doors)
            {
                int id = getInt(door, "id");
                std::string rawDoorType = jsonGetOrDefault<std::string>(door, "type", "normal");
                std::optional<DoorType> doorType = getDoorType(rawDoorType);
                if (!doorType)
                {
                    logError(std::format("Incorrect door type: {}. Must be one of ['normal', 'locked', 'quest', 'magic']", rawDoorType));
                    continue;
                }

                bool open = jsonGetOrDefault<bool>(door, "open", false);
                part.doors.emplace_back(id, *doorType, open);
            }
        }
        stackTrace.pop();

        stackTrace.emplace("windows");
        {
            json windows = root.at("windows");

            for (auto &window : windows)
            {
                int id = getInt(window, "id");
                std::string rawWindowType = window["type"].get<std::string>();
                std::optional<WindowType> windowType = getWindowType(rawWindowType);
                if (!windowType)
                {
                    logError(std::format("Incorrect window type: {}. Must be one of ['normal', 'window', 'hatch', 'hatch_window']", rawWindowType));
                    continue;
                }

                bool open = jsonGetOrDefault(window, "open", false);

                part.windows.emplace_back(id, *windowType, open);
            }
        }
        stackTrace.pop();
    };

    stackTrace.emplace("walls");
    {
        json walls = brush.at("walls");

        stackTrace.emplace("horizontal");
        json horizontal = walls.at("horizontal");
        parseStraightPart(horizontal, horizontalPart);
        stackTrace.pop();

        stackTrace.emplace("vertical");
        json vertical = walls.at("vertical");
        parseStraightPart(vertical, verticalPart);
        stackTrace.pop();

        stackTrace.emplace("corner");
        json corner = walls.at("corner");
        parseItems(corner, &cornerPart);
        stackTrace.pop();

        stackTrace.emplace("pole");
        json pole = walls.at("pole");
        parseItems(pole, &polePart);
        stackTrace.pop();
    }
    stackTrace.pop();

    WallBrush wallbrush = WallBrush(id, name, std::move(horizontalPart), std::move(verticalPart), std::move(cornerPart), std::move(polePart));
    wallbrush.setIconServerId(lookId);

    return wallbrush;
}

std::optional<MountainBrush> BrushLoader::parseMountainBrush(const json &brush)
{
    std::string mountainBrushId = brush.at("id").get<std::string>();
    std::string name = brush.at("name").get<std::string>();

    MountainPart::InnerWall innerWall;

    int lookId = getInt(brush, "lookId");

    std::array<uint32_t, 12> outerWalls;

    stackTrace.emplace("items");
    {
        const json &items = brush.at("items");

        if (items.contains("innerWalls"))
        {
            stackTrace.emplace("innerWalls");
            {
                const json &innerWallsJson = items.at("innerWalls");

                innerWall.east = getIntOrElse(innerWallsJson, "e", 0);
                innerWall.south = getIntOrElse(innerWallsJson, "s", 0);
                innerWall.southEast = getIntOrElse(innerWallsJson, "se", 0);
            }
            stackTrace.pop();
        }

        stackTrace.emplace("outerWalls");
        {
            const json &outerWallJson = items.at("outerWalls");

            outerWalls = parseBorderIds(outerWallJson);
        }
        stackTrace.pop();
    }
    stackTrace.pop();

    stackTrace.emplace("ground");
    const json &ground = brush.at("ground");

    auto onFirstGroundUse = [mountainBrushId](const Brush *brush) {
        MountainBrush *mountainBrush = Brush::getMountainBrush(mountainBrushId);
        switch (brush->type())
        {
            case BrushType::Raw:
            {
                uint32_t serverId = static_cast<const RawBrush *>(brush)->serverId();
                Items::items.getItemTypeByServerId(serverId)->addBrush(mountainBrush);
                mountainBrush->addServerId(serverId);
                break;
            }
            case BrushType::Ground:
            {
                for (const uint32_t serverId : static_cast<const GroundBrush *>(brush)->serverIds())
                {
                    Items::items.getItemTypeByServerId(serverId)->addBrush(mountainBrush);
                    mountainBrush->addServerId(serverId);
                }
                break;
            }
            default:
                ABORT_PROGRAM("Invalid brush type. Must be Raw or Ground brush.");
        }
    };

    auto lazyGround = JsonParseMethods::parseLazyGround(ground);
    lazyGround.setOnFirstUse(onFirstGroundUse);
    stackTrace.pop();

    MountainBrush mountainBrush = MountainBrush(mountainBrushId, name, lazyGround, innerWall, BorderData(outerWalls), lookId);

    return mountainBrush;
}

std::optional<DoodadBrush> BrushLoader::parseDoodadBrush(const json &brush)
{
    std::vector<DoodadBrush::DoodadAlternative> alternatives;

    std::string id = brush.at("id").get<std::string>();
    std::string name = brush.at("name").get<std::string>();

    int lookId = getInt(brush, "lookId");

    stackTrace.emplace("alternates");
    {
        const json &alternates = brush.at("alternates");

        if (!alternates.is_array())
        {
            logError(std::format("Skipping doodad brush with id '{}'. Reason: 'alternates' must be an array.", id));
            return std::nullopt;
        }

        for (auto &alternate : alternates)
        {
            std::vector<std::unique_ptr<DoodadBrush::DoodadEntry>> choices;

            const json &doodads = alternate.at("doodads");
            stackTrace.emplace("doodads");

            for (auto &doodad : doodads)
            {
                int chance = getInt(doodad, "chance");

                // Single item
                if (doodad.contains("id"))
                {
                    int itemId = getInt(doodad, "id");
                    choices.emplace_back(std::make_unique<DoodadBrush::DoodadSingle>(itemId, chance));
                }
                else
                { // Composite item
                    stackTrace.emplace("items");
                    const json &items = doodad.at("items");
                    std::vector<DoodadBrush::CompositeTile> tiles;

                    for (auto &item : items)
                    {
                        DoodadBrush::CompositeTile tile{};
                        tile.dx = getInt(item, "x");
                        tile.dy = getInt(item, "y");
                        tile.serverId = getInt(item, "id");

                        tiles.emplace_back(std::move(tile));
                    }
                    stackTrace.pop();

                    choices.emplace_back(std::make_unique<DoodadBrush::DoodadComposite>(std::move(tiles), chance));
                }
            }
            stackTrace.pop();

            alternatives.emplace_back(DoodadBrush::DoodadAlternative(std::move(choices)));
        }
    }
    stackTrace.pop();

    DoodadBrush doodadBrush = DoodadBrush(id, name, std::move(alternatives), lookId);
    if (brush.contains("replaceBehavior"))
    {
        const json &replaceBehavior = brush.at("replaceBehavior");
        if (replaceBehavior.is_string())
        {
            doodadBrush.replaceBehavior = DoodadBrush::parseReplaceBehavior(replaceBehavior.get<std::string>());
        }
    }

    return doodadBrush;
}

GroundBrush BrushLoader::parseGroundBrush(const json &brush)
{
    json id = brush.at("id").get<std::string>();
    json name = brush.at("name").get<std::string>();

    int lookId = getInt(brush, "lookId");
    int zOrder = getInt(brush, "zOrder");

    json items = brush.at("items");

    std::vector<WeightedItemId> weightedIds;

    for (auto &item : items)
    {
        int chance = getInt(item, "chance");

        if (item.contains("ids"))
        {
            for (auto &id : item.at("ids"))
            {
                weightedIds.emplace_back(id, chance);
            }
        }
        else
        {
            int id = getInt(item, "id");

            weightedIds.emplace_back(id, chance);
        }
    }

    auto groundBrush = GroundBrush(id, std::move(weightedIds), zOrder);
    groundBrush.setIconServerId(lookId);
    groundBrush.setName(name);

    if (brush.contains("borders"))
    {
        json borders = asArray(brush, "borders");

        stackTrace.emplace("borders");

        for (auto &border : borders)
        {
            std::string rawAlign = border.at("align").get<std::string>();
            BorderAlign align;

            switch (string_hash(rawAlign.c_str()))
            {
                case "inner"_sh:
                    align = BorderAlign::Inner;
                    break;
                case "outer"_sh:
                    align = BorderAlign::Outer;
                    break;
                default:
                    throw json::type_error::create(302, std::format("Invalid value for 'align': '{}'. Expected 'inner' or 'outer'.", rawAlign));
            }

            std::string borderId = border.at("id").get<std::string>();

            BorderBrush *brush = Brush::getBorderBrush(borderId);
            if (!brush)
            {
                throw json::type_error::create(302, std::format("There is no border brush with id '{}'", borderId));
            }

            // TODO make it possible for a ground to have more than one border. The borders should be chosen based on
            // what other ground(s) our ground is adjacent to.

            GroundBorder groundBorder{};
            groundBorder.brush = brush;
            groundBorder.align = align;

            if (border.contains("to"))
            {
                std::string toGroundId = border.at("to").get<std::string>();
                if (toGroundId == "none")
                {
                    groundBorder.to = Brush::LazyGround(nullptr);
                }
                else
                {
                    groundBorder.to = Brush::LazyGround(toGroundId);
                }
            }

            groundBrush.addBorder(groundBorder);
        }

        stackTrace.pop();
    }

    return groundBrush;
}

vme_unordered_map<uint32_t, BorderType> BrushLoader::parseExtraBorderIds(const json &extrasJson)
{
    vme_unordered_map<uint32_t, BorderType> data;
    const auto parse = [&data](BorderType borderType, const json &json) {
        if (json.is_number_integer())
        {
            data.try_emplace(json.get<int>(), borderType);
        }
        else if (json.is_array())
        {
            for (const auto &value : json)
            {
                data.try_emplace(value.get<int>(), borderType);
            }
        }
    };

    if (extrasJson.contains("straight"))
    {
        const json &straight = extrasJson.at("straight");
        if (straight.contains("n"))
        {
            parse(BorderType::North, straight.at("n"));
        }
        if (straight.contains("e"))
        {
            parse(BorderType::East, straight.at("e"));
        }
        if (straight.contains("s"))
        {
            parse(BorderType::South, straight.at("s"));
        }
        if (straight.contains("w"))
        {
            parse(BorderType::West, straight.at("w"));
        }
    }

    if (extrasJson.contains("corner"))
    {
        const json &corner = extrasJson.at("corner");
        if (corner.contains("nw"))
        {
            parse(BorderType::NorthWestCorner, corner.at("nw"));
        }
        if (corner.contains("ne"))
        {
            parse(BorderType::NorthEastCorner, corner.at("ne"));
        }
        if (corner.contains("sw"))
        {
            parse(BorderType::SouthWestCorner, corner.at("sw"));
        }
        if (corner.contains("se"))
        {
            parse(BorderType::SouthEastCorner, corner.at("se"));
        }
    }

    if (extrasJson.contains("diagonal"))
    {
        const json &diagonal = extrasJson.at("diagonal");
        if (diagonal.contains("nw"))
        {
            parse(BorderType::NorthWestDiagonal, diagonal.at("nw"));
        }
        if (diagonal.contains("ne"))
        {
            parse(BorderType::NorthEastDiagonal, diagonal.at("ne"));
        }
        if (diagonal.contains("sw"))
        {
            parse(BorderType::SouthWestDiagonal, diagonal.at("sw"));
        }
        if (diagonal.contains("se"))
        {
            parse(BorderType::SouthEastDiagonal, diagonal.at("se"));
        }
    }

    return data;
}

std::array<uint32_t, 12> BrushLoader::parseBorderIds(const json &borderJson)
{
    const json &straight = borderJson.at("straight");
    const json &corner = borderJson.at("corner");
    const json &diagonal = borderJson.at("diagonal");

    std::array<uint32_t, 12> borderIds;

    auto setBorderId = [&borderIds](BorderType borderType, uint32_t serverId) {
        // -1 because first value in BorderType is BorderType::None
        int index = to_underlying(borderType) - 1;
        borderIds[index] = serverId;
    };

    setBorderId(BorderType::North, getIntOrElse(straight, "n", 0));
    setBorderId(BorderType::East, getIntOrElse(straight, "e", 0));
    setBorderId(BorderType::South, getIntOrElse(straight, "s", 0));
    setBorderId(BorderType::West, getIntOrElse(straight, "w", 0));

    setBorderId(BorderType::NorthWestCorner, getIntOrElse(corner, "nw", 0));
    setBorderId(BorderType::NorthEastCorner, getIntOrElse(corner, "ne", 0));
    setBorderId(BorderType::SouthEastCorner, getIntOrElse(corner, "se", 0));
    setBorderId(BorderType::SouthWestCorner, getIntOrElse(corner, "sw", 0));

    setBorderId(BorderType::NorthWestDiagonal, getIntOrElse(diagonal, "nw", 0));
    setBorderId(BorderType::NorthEastDiagonal, getIntOrElse(diagonal, "ne", 0));
    setBorderId(BorderType::SouthEastDiagonal, getIntOrElse(diagonal, "se", 0));
    setBorderId(BorderType::SouthWestDiagonal, getIntOrElse(diagonal, "sw", 0));

    return borderIds;
}

std::unique_ptr<BorderRuleAction> BrushLoader::parseBorderRuleAction(const nlohmann::json &actionJson)
{
    std::string type = actionJson.at("type").get<std::string>();

    switch (string_hash(type.c_str()))
    {
        case "replace"_sh:
        {
            bool replaceSelf = actionJson.at("replaceSelf").get<bool>();
            uint32_t serverId = actionJson.at("serverId").get<int>();

            ReplaceAction action;
            action.serverId = 4657;
            action.replaceSelf = false;

            return std::make_unique<ReplaceAction>(replaceSelf, serverId);
        }
        case "setFull"_sh:
        {
            bool setSelf = actionJson.at("setSelf").get<bool>();

            return std::make_unique<SetFullAction>(setSelf);
        }
        default:
            throw json::other_error::create(403, std::format("Invalid border rule action type: '{}'. Allowed values: [\"replace\", \"setFull\"]", type));
    }
}

void BrushLoader::parseBorderRules(BorderBrush &brush, const json &rules)
{
    for (const auto &ruleJson : rules)
    {
        WhenBorderRule rule;

        const json &whenBorder = ruleJson.at("whenBorder");
        stackTrace.emplace("whenBorder");
        {
            rule.borderId = whenBorder.at("borderId").get<std::string>();
            if (whenBorder.contains("cases"))
            {
                for (const json &jsonCase : whenBorder.at("cases"))
                {
                    WhenBorderRule::Case ruleCase;
                    stackTrace.emplace("case");
                    {
                        ruleCase.selfEdge = parseBorderType(jsonCase.at("selfEdge"));
                        ruleCase.borderEdge = parseBorderType(jsonCase.at("borderEdge"));

                        const json &actions = jsonCase.at("actions");
                        stackTrace.emplace("actions");
                        for (const json &action : actions)
                        {
                            ruleCase.action = parseBorderRuleAction(action);
                            // TODO Allow more than one action
                            break;
                        }
                        stackTrace.pop();
                    }
                    stackTrace.pop();

                    rule.cases.emplace_back(std::move(ruleCase));
                }
            }

            if (whenBorder.contains("actions"))
            {
                const json &actions = whenBorder.at("actions");
                stackTrace.emplace("actions");
                for (const json &action : actions)
                {
                    rule.actions.emplace_back(parseBorderRuleAction(action));
                }
                stackTrace.pop();
            }
        }
        stackTrace.pop();

        brush.rules.emplace_back(std::move(rule));
    }
}

BorderBrush BrushLoader::parseBorderBrush(const nlohmann::json &brush)
{
    json id = brush.at("id").get<std::string>();
    json name = brush.at("name").get<std::string>();

    auto lookId = getInt(brush, "lookId");

    const json &items = brush.at("items");

    const json &straight = items.at("straight");
    const json &corner = items.at("corner");
    const json &diagonal = items.at("diagonal");

    std::array<uint32_t, 12> borderIds = parseBorderIds(items);

    auto borderBrush = BorderBrush(id, name, borderIds);
    borderBrush.setIconServerId(lookId);

    if (brush.contains("centerGroundId"))
    {
        std::string groundId = brush.at("centerGroundId").get<std::string>();
        borderBrush.setCenterGroundId(groundId);
    }

    if (brush.contains("stackBehavior"))
    {
        std::string rawStackBehavior = brush.at("stackBehavior").get<std::string>();
        borderBrush.setStackBehavior(JsonParseMethods::parseBorderStackBehavior(rawStackBehavior));
    }

    if (brush.contains("rules"))
    {
        stackTrace.emplace("rules");
        parseBorderRules(borderBrush, brush.at("rules"));
        stackTrace.pop();
    }

    if (brush.contains("extraItems"))
    {
        stackTrace.emplace("extraItems");
        borderBrush.getBorderData().setExtraBorderIds(parseExtraBorderIds(brush.at("extraItems")));
        stackTrace.pop();
    }

    return borderBrush;
}

void BrushLoader::parseTilesets(const nlohmann::json &tilesetsJson)
{
    stackTrace.emplace("/tilesets");
    auto topTrace = stackTrace;

    for (const json &tileset : tilesetsJson)
    {
        stackTrace = topTrace;

        if (!tileset.contains("id"))
        {
            throw json::other_error::create(403, std::format("A tileset is missing an id (all tilesets must have an id). Add an id to this tileset: {}", tileset.dump(4)));
        }

        parseTileset(tileset);
    }
    stackTrace.pop();
}

void BrushLoader::parseTileset(const nlohmann::json &tilesetJson)
{
    auto tilesetId = tilesetJson.at("id").get<std::string>();
    auto tilesetName = tilesetJson.at("name").get<std::string>();

    stackTrace.emplace(std::format("Tileset '{}'", tilesetId));

    auto palettes = tilesetJson.at("palettes");
    for (const auto &paletteJson : palettes)
    {
        auto paletteId = paletteJson.at("id").get<std::string>();
        stackTrace.emplace(std::format("Palette '{}'", paletteId));

        auto palette = ItemPalettes::getById(paletteId);
        if (!palette)
        {
            VME_LOG_ERROR(std::format("There is no palette with id '{}'.", tilesetId));
            stackTrace.pop();
            continue;
        }

        auto tileset = std::make_unique<Tileset>(tilesetId);
        tileset->setName(tilesetName);

        auto brushes = paletteJson.at("brushes");

        for (auto &brush : brushes)
        {
            std::string brushType = brush.at("type").get<std::string>();
            if (brushType == "raw")
            {
                for (const auto &idObject : brush.at("serverIds"))
                {
                    if (idObject.is_number_integer())
                    {
                        tileset->addRawBrush(idObject.get<int>());
                    }
                    else if (idObject.is_array() && idObject.size() == 2)
                    {
                        uint32_t from = idObject[0].get<int>();
                        uint32_t to = idObject[1].get<int>();

                        for (uint32_t id = from; id <= to; ++id)
                        {
                            tileset->addRawBrush(id);
                        }
                    }
                    else
                    {
                        VME_LOG_ERROR(std::format("Invalid value in serverIds: {}. The values in the serverIds array must be server IDs or arrays of size two as [from_server_id, to_server_id]. For example: 'serverIds: [100, [103, 105]]' will yield ids [100, 103, 104, 105].", idObject.dump(4)));
                        continue;
                    }
                }
            }
        }

        palette->addTileset(std::move(tileset));

        stackTrace.pop();
    }

    stackTrace.pop();
}

void BrushLoader::parseCreatures(const nlohmann::json &creaturesJson)
{
    stackTrace.emplace("/creatures");
    auto topTrace = stackTrace;

    for (const json &creature : creaturesJson)
    {
        stackTrace = topTrace;

        if (!creature.contains("id"))
        {
            throw json::other_error::create(403, std::format("A creature is missing an id (all creatures must have an id). Add an id to this creature: {}", creature.dump(4)));
        }

        parseCreature(creature);
    }
    stackTrace.pop();
}

void BrushLoader::parseCreature(const nlohmann::json &creatureJson)
{
    auto id = getString(creatureJson, "id");
    stackTrace.emplace(std::format("Creature '{}'", id));

    if (!creatureJson.contains("name") || !creatureJson.at("name").is_string())
    {
        throw json::other_error::create(403, std::format("A creature is missing a name (all creatures must have a name). Add a name to this creature: {}", creatureJson.dump(4)));
    }

    if (!creatureJson.contains("type") || !creatureJson.at("type").is_string())
    {
        throw json::other_error::create(403, std::format("A creature is missing a type (either 'monster' or 'npc'). Add a type to this creature: {}", creatureJson.dump(4)));
    }

    if (!creatureJson.contains("looktype") || !creatureJson.at("looktype").is_number_integer())
    {
        throw json::other_error::create(403, std::format("A creature is missing a looktype. Add a looktype to this creature: {}", creatureJson.dump(4)));
    }

    auto name = getString(creatureJson, "name");
    auto type = getString(creatureJson, "type");
    int looktype = getInt(creatureJson, "looktype");

    stackTrace.pop();
}

void BrushLoader::parsePalettes(const nlohmann::json &paletteJson)
{
    stackTrace.emplace("/palettes");

    for (const json &palette : paletteJson)
    {
        if (!palette.contains("id"))
        {
            throw json::other_error::create(403, std::format("A palette is missing an id (all palettes must have an id). Add an id to this palette: {}", palette.dump(4)));
        }

        if (!palette.contains("name"))
        {
            throw json::other_error::create(403, std::format("A palette is missing a name (all palettes must have a name). Add a name to this palette: {}", palette.dump(4)));
        }

        auto id = palette.at("id").get<std::string>();
        auto name = palette.at("name").get<std::string>();

        ItemPalettes::createPalette(id, name);
    }

    stackTrace.pop();
}
