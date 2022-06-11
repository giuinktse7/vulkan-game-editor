#include "main.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <utility>

#include <QFile>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QLoggingCategory>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QStyleFactory>

#include "config.h"
#include "file.h"
#include "graphics/appearances.h"
#include "gui/main_application.h"
#include "gui/map_tab_widget.h"
#include "gui/map_view_widget.h"
#include "gui/vulkan_window.h"
#include "item_wrapper.h"
#include "items.h"
#include "load_map.h"
#include "logger.h"
#include "observable_item.h"
#include "qt/logging.h"
#include "random.h"
#include "time_util.h"
#include "util.h"

#include "brushes/brush.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <numeric>
#include <regex>

#include "lua/luascript_interface.h"

#include "brushes/brush_loader.h"
#include "item_palette.h"

int main(int argc, char *argv[])
{
    ItemPalettes::createPalette("Default", "default");

    Random::global().setSeed(123);
    TimePoint::setApplicationStartTimePoint();

    // if (!LuaScriptInterface::initialize())
    // {
    //     return EXIT_FAILURE;
    // }

    // LuaScriptInterface::get()->test();

    /* return 0; */

    // QQuickWindow::setSceneGraphBackend(QSGRendererInterface::VulkanRhi);

    MainApplication app(argc, argv);

    // std::string version = "12.70.10889";
    std::string version = "12.81";
    std::string clientPath = std::format("data/clients/{}", version);

    auto configResult = Config::create(version);
    if (configResult.isErr())
    {
        VME_LOG(configResult.unwrapErr().show());
        return EXIT_FAILURE;
    }

    QQuickStyle::setStyle("Fusion");

    Config config = configResult.unwrap();
    config.loadOrTerminate();

    // testApplyAtlasTemplate();

    BrushLoader brushLoader;
    brushLoader.load(std::format("{}/palettes/palettes.json", clientPath));
    brushLoader.load(std::format("{}/palettes/borders.json", clientPath));
    brushLoader.load(std::format("{}/palettes/grounds.json", clientPath));
    brushLoader.load(std::format("{}/palettes/walls.json", clientPath));
    brushLoader.load(std::format("{}/palettes/doodads.json", clientPath));
    brushLoader.load(std::format("{}/palettes/mountains.json", clientPath));
    brushLoader.load(std::format("{}/palettes/creatures.json", clientPath));
    brushLoader.load(std::format("{}/palettes/tilesets.json", clientPath));

    // TemporaryTest::loadAllTexturesIntoMemory();
    // return 0;

    app.initializeUI();

    // app.mainWindow.addMapTab("C:/Users/giuin/Desktop/Untitled-1.otbm");
    // app.mainWindow.addMapTab(TemporaryTest::makeTestMap1());
    // app.mainWindow.addMapTab(TemporaryTest::makeTestMap2());

    // QFontDatabase database;

    // const QStringList fontFamilies = database.families();
    // for (auto family : fontFamilies)
    // {
    //     VME_LOG_D(family);
    // }

    // VME_LOG_D("???: " << database.hasFamily("Source Sans Pro"));

    // Appearances::dumpSpriteFiles("D:\\Programs\\Tibia\\packages\\TibiaExternal\\assets", "./spritedump");
    // return 0;

    // std::vector<uint32_t> bounacOutfitIds{1290, 1301, 1317, 1316, 1338, 1339};
    // MainUtils::printOutfitAtlases(bounacOutfitIds);

    // ItemType *t = Items::items.getItemTypeByServerId(7759);
    // ItemType *t = Items::items.getItemTypeByServerId(5901);
    // auto atlases = t->atlases();
    // for (const auto id : t->appearance->getSpriteInfo().spriteIds)
    // {
    //     VME_LOG_D(id);
    // }
    // VME_LOG(t->getFirstTextureAtlas()->sourceFile);

    // QApplication::setOverrideCursor(QtUtil::itemPixmap(1987));

    // Enables QML binding debug logs
    // QLoggingCategory::setFilterRules(QStringLiteral("qt.qml.binding.removal.info=true"));

    // for (int serverId = 0; serverId < 40000; ++serverId)
    // {
    //     if (Items::items.validItemType(serverId))
    //     {
    //         auto itemtype = Items::items.getItemTypeByServerId(serverId);
    //         if (itemtype->isFluidContainer())
    //         {
    //             VME_LOG_D("\n" << serverId << ": " << itemtype->name() << ":");
    //             for (int fg = 0; fg < itemtype->appearance->frameGroupCount(); ++fg)
    //             {
    //                 const std::string delimiter = ", ";
    //                 auto ids = itemtype->appearance->getSpriteInfo(fg).spriteIds;
    //                 if (ids.size() == 1)
    //                     continue;
    //                 const auto zero = std::to_string(ids[0]);
    //                 std::string result = std::accumulate(std::next(ids.begin()), ids.end(),
    //                                                      zero,
    //                                                      [delimiter](std::string a, uint32_t b) {
    //                                                          return a + delimiter + std::to_string(b);
    //                                                      });

    //                 VME_LOG_D(result << " : " << std::to_string(ids.size()));
    //             }
    //         }
    //     }
    // }

    // TemporaryTest::loadAllTexturesIntoMemory();

    // int rat = 21;
    // checkCreature(rat);

    // Creatures::creatureType(rat)->checkTransparency(Direction::North);
    // Creatures::creatureType(rat)->checkTransparency(Direction::East);

    // int cyclops = 22;
    // checkCreature(cyclops);

    return app.run();
}

// void testApplyAtlasTemplate()
// {
//     auto nomadType = Creatures::addCreatureType("test", "Test", 146);
//     Creature nomad(*nomadType);

//     Outfit::Look look;
//     look.type = 146;
//     look.addon = 0;
//     look.setHead(77);
//     look.setBody(125);
//     look.setLegs(22);
//     look.setFeet(77);

//     int spriteOffset = 0;

//     auto spriteIndex = nomadType->getSpriteIndex(0, Direction::North);
//     auto spriteId = nomadType->frameGroup(0).getSpriteId(spriteIndex);

//     // TODO Assumes that template is always at index + 1. Maybe not correct.
//     auto templateSpriteId = nomadType->frameGroup(0).getSpriteId(spriteIndex + 1);

//     TextureAtlas *targetAtlas = nomadType->appearance->getTextureAtlas(spriteId);
//     TextureAtlas *templateAtlas = nomadType->appearance->getTextureAtlas(spriteId);

//     auto const [spriteX, spriteY] = offsetFromSpriteId(atlas, spriteId);
//     auto const [templateX, templateY] = offsetFromSpriteId(atlas, spriteId + 1);

//     auto &pixels = atlas->getOrCreateTexture().mutablePixelsTEST();

//     applyTemplate(spriteX, spriteY, templateX, templateY, atlas->spriteWidth, atlas->spriteHeight, atlas->width, pixels, Outfit(look));
// }

std::pair<int, int> offsetFromSpriteId(TextureAtlas *atlas, uint32_t spriteId)
{
    int spriteIndex = spriteId - atlas->firstSpriteId;

    auto topLeftX = atlas->spriteWidth * (spriteIndex % atlas->columns);
    auto topLeftY = atlas->spriteHeight * (spriteIndex / atlas->rows);

    return {topLeftX, topLeftY};
}

void TemporaryTest::loadAllTexturesIntoMemory()
{
    TimePoint start;

    for (uint16_t id = 100; id < Items::items.size(); ++id)
    {
        ItemType *t = Items::items.getItemTypeByServerId(id);

        if (!Items::items.validItemType(id))
            continue;

        const TextureInfo &info = t->getTextureInfo();
        info.atlas->getOrCreateTexture();
    }

    for (uint16_t looktype = 1; looktype < 1400; ++looktype)
    {
        auto creatureType = Creatures::creatureType(looktype);
        if (creatureType)
        {
            creatureType->getTextureInfo(0, Direction::North).atlas->getOrCreateTexture();
        }
    }
    VME_LOG("loadTextures() ms: " << start.elapsedMillis());
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>TemporaryTest>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>>

std::shared_ptr<Map> TemporaryTest::makeTestMap1()
{
    std::shared_ptr<Map> map = std::make_shared<Map>();
    auto &rand = Random::global();

    for (int x = 0; x < 200; ++x)
    {
        for (int y = 0; y < 200; ++y)
        {
            map->addItem(Position(x, y, 7), rand.nextInt<uint16_t>(4526, 4542));
        }
    }

    for (int i = 0; i < 10; ++i)
    {
        map->addItem(Position(rand.nextInt<uint16_t>(1, 10), rand.nextInt<uint16_t>(1, 10), 7), 2767 + rand.nextInt<uint16_t>(0, 2));
    }

    return map;
}

std::shared_ptr<Map> TemporaryTest::makeTestMap2()
{
    std::shared_ptr<Map> map = std::make_shared<Map>();

    // Add some creatures
    // {
    //     VME_LOG_D("Creature ids: ");
    //     auto addCreature = [&map](Position &pos, uint32_t outfitId) {
    //         auto newCreature = Creature::fromOutfitId(outfitId);
    //         if (newCreature)
    //         {
    //             auto &creature = newCreature.value();
    //             creature.setDirection(Direction::South);
    //             map->getTile(pos)->setCreature(std::move(creature));
    //             VME_LOG_D(outfitId);
    //         }
    //         else
    //         {
    //             VME_LOG_D("No creature (" << outfitId << ")");
    //         }
    //     };

    //     int outfitId = 2000;
    //     auto stop = [&outfitId] { return outfitId < 800; };
    //     for (int y = 2; y < 70; y += 3)
    //     {
    //         if (stop())
    //             break;

    //         for (int x = 2; x < 70; x += 3)
    //         {
    //             Position pos(x, y, 7);
    //             map->addItem(pos, 103);

    //             while (!(Creatures::creatureType(outfitId) || stop()))
    //                 --outfitId;

    //             if (stop())
    //                 break;

    //             addCreature(pos, outfitId);
    //             --outfitId;
    //         }
    //     }
    // }

    {
        std::array<uint16_t, 9> sandWaterBorders{7951, 7945, 7952, 7946, 4608, 7944, 7953, 7943, 7954};

        Position pos(10, 10, 7);
        map->addItem(pos, sandWaterBorders[0]);

        pos.move(1, 0, 0);
        map->addItem(pos, sandWaterBorders[1]);

        pos.move(1, 0, 0);
        map->addItem(pos, sandWaterBorders[2]);

        pos.move(-2, 1, 0);
        map->addItem(pos, sandWaterBorders[3]);

        pos.move(1, 0, 0);
        map->addItem(pos, sandWaterBorders[4]);

        pos.move(1, 0, 0);
        map->addItem(pos, sandWaterBorders[5]);

        pos.move(-2, 1, 0);
        map->addItem(pos, sandWaterBorders[6]);

        pos.move(1, 0, 0);
        map->addItem(pos, sandWaterBorders[7]);

        pos.move(1, 0, 0);
        map->addItem(pos, sandWaterBorders[8]);
    }

    // uint32_t i = 37733;

    // for (int y = 0; y < 1000; ++y)
    // {
    //     for (int x = 1; x < 40; ++x)
    //     {
    //         if (i >= 39768)
    //         {
    //             VME_LOG_D("Added all " << (39768 - 37733) << " items.");
    //             return map;
    //         }
    //         if (Items::items.getItemTypeByServerId(i)->isCorpse())
    //             --x;
    //         else
    //             map->addItem(Position(x, y, 7), i);

    //         ++i;
    //     }
    // }

    return map;
}

void MainUtils::printOutfitAtlases(std::vector<uint32_t> outfitIds)
{
    std::unordered_set<const TextureAtlas *> atlases;
    for (const auto id : outfitIds)
    {
        auto &type = *Creatures::creatureType(id);
        auto &groups = type.frameGroups();
        for (const FrameGroup &frameGroup : groups)
        {
            for (const auto spriteId : frameGroup.spriteInfo.spriteIds)
            {
                atlases.emplace(Appearances::getTextureAtlas(spriteId));
            }
        }
    }

    VME_LOG("Looktype atlases:");
    for (const auto atlas : atlases)
    {
        VME_LOG(atlas->sourceFile);
    }
}

void TemporaryTest::testOctree()
{
    VME_LOG_D("octree:");
    vme::MapSize mapSize = {2048, 2048, 16};
    // constexpr vme::MapSize mapSize = {4096, 4096, 16};

    vme::octree::Tree tree = vme::octree::Tree::create(mapSize);

    auto &rand = Random::global();
    rand.setSeed(123);

    // Positions that were bugged at one point(could include in tests maybe) : tree.add(Position(712, 428, 10));
    // tree.add(Position(718, 491, 12));
    // tree.add(Position(67, 473, 8));
    // tree.add(Position(47, 89, 7));

    // tree.add(Position(960, 2752, 8));
    // tree.add(Position(3001, 2773, 8));

    tree.add(Position(35, 35, 7));
    // tree.remove(Position(35, 35, 7));
    tree.add(Position(64, 32, 7));

    // auto chunk = vme::octree::ChunkSize;
    // int chunksX = 1;
    // int chunksY = 1;
    // int chunksZ = 2;
    // int positions = chunksX * chunksY * chunksZ * (chunk.width * chunk.height * chunk.depth);

    // VME_LOG("Adding from " << Position(0, 0, 0) << " to " << Position(chunksX * chunk.width, chunksY * chunk.height, chunksZ * chunk.depth) << " (" << positions << " positions).");
    // for (int x = 0; x < chunksX; ++x)
    //     for (int y = 0; y < chunksY; ++y)
    //         for (int z = 0; z < chunksZ; ++z)
    //             addChunk(Position(x * 64, y * 64, z * 8), tree);

    int count = 0;

    TimePoint start;
    // for (const auto pos : tree)
    //     ++count;

    {
        // These two together created a crash (they shared the same leaf node)
        // tree.add(Position(979, 2778, 9));
        // tree.add(Position(3001, 2773, 8));
    }

    // for (int i = 0; i < 10000; ++i)
    // {
    //     auto x = rand.nextInt<int>(0, 4000);
    //     auto y = rand.nextInt<int>(0, 4000);
    //     auto z = rand.nextInt<int>(0, 16);

    //     Position pos(x, y, z);
    //     // VME_LOG_D("(" << i << "): " << pos);
    //     // VME_LOG_D("(" << i << "): "
    //     //   << "Position(" << x << "," << y << "," << z << ")");
    //     // VME_LOG_D("Adding " << pos);
    //     tree.add(pos);
    // }

    // {

    // auto b = Position(3638, 873, 5);
    // tree.add(b);

    // auto a = Position(3644, 797, 0);
    // tree.add(a);

    // VME_LOG_D(tree.boundingBox());

    // tree.remove(b);

    // VME_LOG_D(tree.boundingBox());

    // }

    // {
    //     auto a = Position(628, 1354, 7);
    //     tree.add(a);
    //     auto b = Position(2518, 1604, 11);
    //     tree.add(b);

    //     auto c = Position(2507, 1788, 14);
    //     tree.add(c);
    // }

    // auto pos = Position(47, 89, 7);
    // tree.add(pos);

    // VME_LOG_D("Next:");
    // auto pos2 = Position(1027, 287, 9);
    // tree.add(pos2);

    // VME_LOG_D("pos2: " << std::boolalpha << tree.contains(pos2));
    // tree.remove(pos2);
    // VME_LOG_D("pos2 still here? " << std::boolalpha << tree.contains(pos2));
    VME_LOG_D("Count: " << count);
    VME_LOG("Done in " << start.elapsedMicros() << " us. (" << start.elapsedMillis() << " ms)");

    TimePoint clearStart;
    tree.clear();

    VME_LOG("Clear in " << clearStart.elapsedMicros() << " us. Size: " << tree.size() << ", contains: " << tree.contains(Position(10, 10, 7)));
}

/**
 * Generates items.xml-compliant XML rows for item names based on a file with the structure:
 * [<Client ID>] <item name>
 * [<Client ID>] <item name>
 * [<Client ID>] <item name>
 * ...
 */
void generateItemXmlNames(std::string inputPath, std::string outputPath)
{
    std::regex regexPattern("\\[([0-9]+)\\] (.+)");

    std::ifstream file(inputPath);

    std::stringstream s("", std::ios_base::app | std::ios_base::out);

    int prevId = -1;
    int consecutive = 0;
    std::string prevName;

    auto flush = [&prevId, &consecutive, &prevName, &s]() {
        if (prevId == -1)
        {
            return;
        }
        if (consecutive == 0)
        {
            s << std::format("<item id=\"{}\" name=\"{}\" />", prevId, prevName) << std::endl;
        }
        else
        {
            int from = prevId - consecutive;
            int to = prevId;
            s << std::format("<item fromid=\"{}\" toid=\"{}\" name=\"{}\" />", from, to, prevName) << std::endl;
        }
    };

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty())
            continue;

        std::smatch match;
        std::regex_search(line, match, regexPattern);
        int clientId = std::stoi(match[1]);
        std::string itemName = match[2];

        if (itemName.starts_with("a "))
        {
            itemName.erase(0, 2);
        }
        else if (itemName.starts_with("an "))
        {
            itemName.erase(0, 3);
        }

        auto itemType = Items::items.getItemTypeByClientId(clientId);
        if (itemType && itemType->name().empty())
        {
            uint32_t serverId = itemType->id;
            if (prevName == itemName && prevId != -1 && prevId == serverId - 1)
            {
                ++consecutive;
            }
            else
            {
                flush();
                consecutive = 0;
            }

            prevId = serverId;
            prevName = itemName;
        }
        else
        {
            flush();
            consecutive = 0;
            prevId = -1;
        }
    }

    flush();

    std::fstream outFile;
    outFile.open(outputPath, std::ios::out);
    if (outFile)
    {
        outFile << s.str();
        outFile.close();
    }
}