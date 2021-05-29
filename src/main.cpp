#include "main.h"

#include <fstream>
#include <iostream>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>

#include <QFile>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QLoggingCategory>
#include <QQuickWindow>
#include <QStyleFactory>

#include "config.h"
#include "graphics/appearances.h"
#include "gui/main_application.h"
#include "gui/map_tab_widget.h"
#include "gui/map_view_widget.h"
#include "gui/vulkan_window.h"
#include "item_wrapper.h"
#include "items.h"
#include "load_map.h"
#include "logger.h"
#include "minimap.h"
#include "observable_item.h"
#include "qt/logging.h"
#include "random.h"
#include "time_point.h"
#include "util.h"

#include "brushes/brush.h"
#include <fstream>
#include <nlohmann/json.hpp>
#include <numeric>

#include "lua/luascript_interface.h"

int main(int argc, char *argv[])
{
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

    auto configResult = Config::create("12.60.10411");
    if (configResult.isErr())
    {
        VME_LOG(configResult.unwrapErr().show());
        return EXIT_FAILURE;
    }

    Config config = configResult.unwrap();
    config.loadOrTerminate();

    // TemporaryTest::loadAllTexturesIntoMemory();

    std::filesystem::path mapPath = "C:/Users/giuin/Desktop/Untitled-1.otbm";

    std::variant<Map, std::string> result = LoadMap::loadMap(mapPath);
    if (!std::holds_alternative<Map>(result))
    {
        VME_LOG("Map load error: " << std::get<std::string>(result));
        return EXIT_FAILURE;
    }

    Map loadedMap = std::move(std::get<Map>(result));
    std::shared_ptr<Map> sharedMap = std::make_shared<Map>(std::move(loadedMap));

    app.initializeUI();
    app.mainWindow.addMapTab(sharedMap);
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

    return app.run();
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
    //             creature.setDirection(Creature::Direction::South);
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

void TemporaryTest::addChunk(Position from, vme::octree::Tree &tree)
{
    auto chunk = vme::octree::ChunkSize;
    auto to = Position(from.x + chunk.width - 1, from.y + chunk.height - 1, static_cast<Position::z_type>(from.z + chunk.depth - 1));
    VME_LOG("addChunk: " << from << " to " << to);

    for (const auto pos : MapArea(from, to))
        tree.add(pos);
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