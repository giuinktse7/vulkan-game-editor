#include "brush_loader.h"

#include <format>
#include <fstream>
#include <sstream>
#include <stack>

#include "../debug.h"
#include "../item_palette.h"
#include "../logger.h"
#include "../time_point.h"
#include "brush.h"

using json = nlohmann::json;

std::string joinStack(std::stack<std::string> &stack, std::string delimiter)
{
    auto stackCopy = stack;
    std::ostringstream ss;
    while (!stackCopy.empty())
    {
        ss << stackCopy.top();
        stackCopy.pop();
        if (!stackCopy.empty())
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

bool BrushLoader::load(std::filesystem::path path)
{
    TimePoint start;

    if (!std::filesystem::exists(path))
    {
        VME_LOG_ERROR(std::format("Could not find file '{}'", path.string()));
        return false;
    }

    std::ifstream fileStream(path);
    json rootJson;
    fileStream >> rootJson;
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

        auto brushType = parseBrushType(getString(brush, "type"));
        if (!brushType)
        {
            throw json::type_error::create(302, std::format("The type must be one of ['ground', 'doodad', 'wall]."));
        }

        switch (*brushType)
        {
            case BrushType::Ground:
            {
                auto groundBrush = parseGroundBrush(brush);
                VME_LOG_D("ID: " << groundBrush.brushId());
                Brush::addGroundBrush(std::move(groundBrush));
                break;
            }

            default:
                break;
        }
    }
    stackTrace.pop();
}

GroundBrush BrushLoader::parseGroundBrush(const json &brush)
{
    auto id = brush.at("id").get<std::string>();
    auto name = brush.at("name").get<std::string>();

    auto lookId = getInt(brush, "lookId");
    auto zOrder = getInt(brush, "zOrder");

    auto items = brush.at("items");

    std::vector<WeightedItemId> weightedIds;

    for (auto &item : items)
    {
        int id = getInt(item, "id");
        int chance = getInt(item, "chance");

        weightedIds.emplace_back(id, chance);
    }

    auto groundBrush = GroundBrush(id, std::move(weightedIds));
    groundBrush.setIconServerId(lookId);
    groundBrush.setName(name);
    return groundBrush;
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

        Tileset tileset(tilesetId);
        tileset.setName(tilesetName);

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
                        tileset.addRawBrush(idObject.get<int>());
                    }
                    else if (idObject.is_array() && idObject.size() == 2)
                    {
                        uint32_t from = idObject[0].get<int>();
                        uint32_t to = idObject[1].get<int>();

                        for (uint32_t id = from; id <= to; ++id)
                        {
                            tileset.addRawBrush(id);
                        }
                    }
                    else
                    {
                        throw json::type_error::create(302, std::format("Invalid value in serverIds: {}. The values in the serverIds array must be server IDs or arrays of size two as [from_server_id, to_server_id]. For example: 'serverIds: [100, [103, 105]]' will yield ids [100, 103, 104, 105].", idObject.dump(4)));
                    }
                }
            }
        }

        palette->addTileset(std::move(tileset));

        stackTrace.pop();
    }

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
}
