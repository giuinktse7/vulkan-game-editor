#include "main.h"

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QGuiApplication>
#include <QIODevice>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QVariant>

#include <iostream>

#include "core/brushes/border_brush.h"
#include "core/brushes/brush.h"
#include "core/brushes/brush_loader.h"
#include "core/brushes/creature_brush.h"
#include "core/brushes/doodad_brush.h"
#include "core/brushes/ground_brush.h"
#include "core/brushes/mountain_brush.h"
#include "core/brushes/wall_brush.h"
#include "core/config.h"
#include "core/item_palette.h"
#include "core/items.h"
#include "core/random.h"
#include "core/time_util.h"
#include "debug_util.h"
#include "gui_thing_image.h"

#include <QtQml/qqmlextensionplugin.h>
Q_IMPORT_QML_PLUGIN(AppComponentsPlugin)

class FileWatcher : QObject
{
  public:
    FileWatcher()
    {
    }

    void onFileChanged(std::function<void(const QString &)> f)
    {
        this->f = f;
    }

    void watchFile(const QString &file)
    {
        _watcher.addPath(file);

        if (this->f)
        {
            connect(&_watcher, &QFileSystemWatcher::fileChanged, this, *f);
        }
    }

  private:
    std::optional<std::function<void(const QString &)>> f;
    QFileSystemWatcher _watcher;
};

std::unique_ptr<FileWatcher> watcher;

MainApp::MainApp(int argc, char **argv)
    : app(argc, argv), dataModel(std::make_unique<AppDataModel>())
{
}

int MainApp::start()
{
    createDefaultPalettes();

    // Use Vulkan
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Vulkan);
    // QQuickStyle::setStyle("Fusion");

    rootView = std::make_unique<QQuickView>();
    QQmlEngine *engine = rootView->engine();

    // The QQmlEngine takes ownership of the image provider
    engine->addImageProvider(QLatin1String("itemTypes"), new ItemTypeImageProvider);
    engine->addImageProvider(QLatin1String("creatureLooktypes"), new CreatureImageProvider);

    qmlRegisterSingletonInstance("VME.dataModel", 1, 0, "AppDataModel", dataModel.get());

    QVariantMap properties;
    // properties.insert("tilesetModel", QVariant::fromValue(dataModel->itemPaletteStore().tilesetModel()));

    rootView->setInitialProperties(properties);

    rootView->setResizeMode(QQuickView::SizeRootObjectToView);
    engine->addImportPath(QStringLiteral(":/"));

    const QUrl url(u"qrc:/app/qml/main.qml"_qs);
    rootView->setSource(url);
    rootView->show();

    // watcher = std::make_unique<FileWatcher>();

    // watcher->onFileChanged([this](const QString &file) {
    //     qDebug() << file;

    //     if (QFileInfo(file).baseName() == "main")
    //     {
    //         qDebug() << "Main changed.";
    //         rootView->engine()->clearComponentCache();
    //         this->rootView->setSource(QUrl::fromLocalFile(file));
    //     }
    // });

    // watcher->watchFile("qml/main.qml");

    // QObject::connect(watcher->watcher(), QFileSystemWatcher::fileChanged)

    return app.exec();
}

int main(int argc, char **argv)
{
    // qInstallMessageHandler(pipeQtMessagesToStd);
    // showQmlResources();

    Random::global().setSeed(123);
    TimePoint::setApplicationStartTimePoint();
    std::string version = "12.87.12091";

    auto configResult = Config::create(version);
    if (configResult.isErr())
    {
        VME_LOG(configResult.unwrapErr().show());
        return EXIT_FAILURE;
    }

    Config config = configResult.unwrap();
    config.loadOrTerminate();

    std::string clientPath = std::format("data/clients/{}", version);

    BrushLoader brushLoader;
    brushLoader.load(std::format("{}/palettes/palettes.json", clientPath));
    brushLoader.load(std::format("{}/palettes/borders.json", clientPath));
    brushLoader.load(std::format("{}/palettes/grounds.json", clientPath));
    brushLoader.load(std::format("{}/palettes/walls.json", clientPath));
    brushLoader.load(std::format("{}/palettes/doodads.json", clientPath));
    brushLoader.load(std::format("{}/palettes/mountains.json", clientPath));
    brushLoader.load(std::format("{}/palettes/creatures.json", clientPath));
    brushLoader.load(std::format("{}/palettes/tilesets.json", clientPath));

    MainApp app(argc, argv);

    return app.start();
}

// TODO Load palettes from JSON
void MainApp::createDefaultPalettes()
{

    // Raw palette
    {
        ItemPalette &rawPalette = *ItemPalettes::getOrCreateById("raw");

        auto &bottomTileset = rawPalette.addTileset(Tileset("bottom_items", "Bottom items"));
        // auto &groundTileset = rawPalette.addTileset(Tileset("grounds", "Grounds"));
        auto &borderTileset = rawPalette.addTileset(Tileset("borders", "Borders"));
        auto &unsightTileset = rawPalette.addTileset(Tileset("sight_blocking", "Sight-blocking"));
        auto &doorTileset = rawPalette.addTileset(Tileset("doors", "Doors"));
        auto &archwayTileset = rawPalette.addTileset(Tileset("archways", "Archways"));
        auto &containerTileset = rawPalette.addTileset(Tileset("containers", "Containers"));
        auto &hangableTileset = rawPalette.addTileset(Tileset("hangables", "Hangables"));
        auto &pickuableTileset = rawPalette.addTileset(Tileset("pickupables", "Pickupables"));
        auto &equipmentTileset = rawPalette.addTileset(Tileset("equipment", "Equipment"));
        auto &lightSourceTileset = rawPalette.addTileset(Tileset("light_source", "Light source"));
        auto &interiorTileset = rawPalette.addTileset(Tileset("interior_wrap_unwrap", "Interior / Wrap / Unwrap"));
        auto &otherTileset = rawPalette.addTileset(Tileset("other", "Other"));
        auto &allTileset = rawPalette.addTileset(Tileset("all", "All"));

        int from = 100;
        int to = 45000;

#ifdef _DEBUG_VME
        from = 100;
        to = 5000;
#endif

        for (int i = from; i < to; ++i)
        {
            ItemType *itemType = Items::items.getItemTypeByServerId(i);
            if (!itemType || !itemType->isValid())
                continue;

            if (itemType->hasFlag(AppearanceFlag::Corpse) || itemType->hasFlag(AppearanceFlag::PlayerCorpse))
            {
                // Skip corpses
                continue;
            }

            allTileset.addRawBrush(i);

            if (itemType->hasFlag(AppearanceFlag::Bottom))
            {
                bottomTileset.addRawBrush(i);
            }
            // else if (itemType->hasFlag(AppearanceFlag::Ground))
            // {
            //     groundTileset.addRawBrush(i);
            // }
            else if (itemType->hasFlag(AppearanceFlag::Border))
            {
                borderTileset.addRawBrush(i);
            }
            else if (itemType->hasFlag(AppearanceFlag::Top))
            {
                archwayTileset.addRawBrush(i);
            }
            else if (itemType->isDoor())
            {
                doorTileset.addRawBrush(i);
            }
            else if (itemType->hasFlag(AppearanceFlag::Unsight))
            {
                unsightTileset.addRawBrush(i);
            }
            else if (itemType->isContainer())
            {
                containerTileset.addRawBrush(i);
            }
            else if (itemType->hasFlag(AppearanceFlag::Wrap) || itemType->hasFlag(AppearanceFlag::Unwrap) || itemType->isBed())
            {
                interiorTileset.addRawBrush(i);
            }
            else if (itemType->hasFlag(AppearanceFlag::Take))
            {
                if (itemType->hasFlag(AppearanceFlag::Clothes))
                {
                    equipmentTileset.addRawBrush(i);
                }
                else
                {
                    pickuableTileset.addRawBrush(i);
                }
            }
            else if (itemType->hasFlag(AppearanceFlag::Light))
            {
                lightSourceTileset.addRawBrush(i);
            }
            else if (itemType->hasFlag(AppearanceFlag::Hang))
            {
                hangableTileset.addRawBrush(i);
            }
            else
            {
                if (!itemType->hasFlag(AppearanceFlag::Ground))
                {
                    otherTileset.addRawBrush(i);
                }
            }

            // GuiImageCache::cachePixmapForServerId(i);
        }
    }

    // Terrain palette
    {
        ItemPalette &terrainPalette = *ItemPalettes::getById("terrain");

        // Grounds
        {
            auto &groundTileset = terrainPalette.addTileset(Tileset("grounds", "Grounds"));
            auto &brushes = Brush::getGroundBrushes();
            for (auto &brush : brushes)
            {
                groundTileset.addBrush(brush.second.get());
            }

            auto &mountainBrushes = Brush::getMountainBrushes();
            for (auto &brush : mountainBrushes)
            {
                groundTileset.addBrush(brush.second.get());
            }
        }

        // Borders
        {
            auto &borderTileset = terrainPalette.addTileset(Tileset("borders", "Borders"));
            auto &brushes = Brush::getBorderBrushes();
            for (auto &brush : brushes)
            {
                borderTileset.addBrush(brush.second.get());
            }
        }

        // Walls
        {
            auto &wallTileset = terrainPalette.addTileset(Tileset("walls", "Walls"));
            auto &brushes = Brush::getWallBrushes();
            for (auto &brush : brushes)
            {
                wallTileset.addBrush(brush.second.get());
            }
        }

        // Doodads
        {
            auto &brushes = Brush::getDoodadBrushes();
            if (brushes.size() > 0)
            {
                auto &doodadTileset = terrainPalette.addTileset(Tileset("doodads", "Doodads"));
                for (auto &brush : brushes)
                {
                    doodadTileset.addBrush(brush.second.get());
                }
            }
        }
    }

    // Creature palette
    {
        ItemPalette &creaturePalette = *ItemPalettes::getById("creature");

        auto &otherTileset = creaturePalette.addTileset(Tileset("other", "Other"));

        const auto addTestCreatureBrush = [&otherTileset](std::string id, std::string name, int looktype) {
            otherTileset.addBrush(Brush::addCreatureBrush(CreatureBrush(Creatures::addCreatureType(id, name, looktype))));
        };

        otherTileset.addBrush(Brush::addCreatureBrush(CreatureBrush(Creatures::addCreatureType(
            "colorful_nomad",
            "Colorful Nomad",
            // Outfit(150, 116, 68, 68, 68, Outfit::Addon::First | Outfit::Addon::Second, 370)))));
            Outfit(150, 116, 68, 68, 68, Outfit::Addon::None, 370)))));
        // Outfit(150, 116, 68, 68, 68, Outfit::Addon::None)))));

        otherTileset.addBrush(Brush::addCreatureBrush(CreatureBrush(Creatures::addCreatureType(
            "colorful_nomad_2",
            "Colorful Nomad 2",
            // Outfit(150, 116, 68, 68, 68, Outfit::Addon::First | Outfit::Addon::Second, 370)))));
            Outfit(150, 116, 68, 68, 68, Outfit::Addon::None)))));

        otherTileset.addBrush(Brush::addCreatureBrush(CreatureBrush(Creatures::addCreatureType(
            "nomad",
            "Nomad",
            Outfit(146, 114, 20, 22, 2)))));

        addTestCreatureBrush("rat", "Rat", 21);
        addTestCreatureBrush("bear", "Bear", 16);
        addTestCreatureBrush("cyclops", "Cyclops", 22);
        addTestCreatureBrush("goblin", "Goblin", 61);
        addTestCreatureBrush("goblin_scavenger", "Goblin Scavenger", 297);
        addTestCreatureBrush("goblin_assassin", "Goblin Assassin", 296);
        addTestCreatureBrush("squirrel", "Squirrel", 274);
        addTestCreatureBrush("cobra", "Cobra", 81);

        otherTileset.addBrush(Brush::addCreatureBrush(CreatureBrush(Creatures::addCreatureType("mimic", "Mimic", Outfit::fromServerId(1740)))));
    }
}
