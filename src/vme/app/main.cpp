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

#include <iostream>

#include "core/config.h"
#include "core/item_palette.h"
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
    : app(argc, argv)
{
}

int MainApp::start()
{
    // This example needs Vulkan. It will not run otherwise.
    QQuickWindow::setGraphicsApi(QSGRendererInterface::Vulkan);
    QQuickStyle::setStyle("Fusion");

    rootView = std::make_unique<QQuickView>();
    QQmlEngine *engine = rootView->engine();

    // The QQmlEngine takes ownership of the image provider
    engine->addImageProvider(QLatin1String("itemTypes"), new ItemTypeImageProvider);
    engine->addImageProvider(QLatin1String("creatureLooktypes"), new CreatureImageProvider);

    rootView->setResizeMode(QQuickView::SizeRootObjectToView);
    engine->addImportPath(QStringLiteral(":/"));

    const QUrl url(u"qrc:/app/qml/main.qml"_qs);
    rootView->setSource(url);
    rootView->show();

    watcher = std::make_unique<FileWatcher>();

    watcher->onFileChanged([this](const QString &file) {
        qDebug() << file;

        if (QFileInfo(file).baseName() == "main")
        {
            qDebug() << "Main changed.";
            rootView->engine()->clearComponentCache();
            this->rootView->setSource(QUrl::fromLocalFile(file));
        }
    });

    watcher->watchFile("qml/main.qml");

    // QObject::connect(watcher->watcher(), QFileSystemWatcher::fileChanged)

    // BrushLoader brushLoader;
    // brushLoader.load(std::format("{}/palettes/palettes.json", clientPath));
    // brushLoader.load(std::format("{}/palettes/borders.json", clientPath));
    // brushLoader.load(std::format("{}/palettes/grounds.json", clientPath));
    // brushLoader.load(std::format("{}/palettes/walls.json", clientPath));
    // brushLoader.load(std::format("{}/palettes/doodads.json", clientPath));
    // brushLoader.load(std::format("{}/palettes/mountains.json", clientPath));
    // brushLoader.load(std::format("{}/palettes/creatures.json", clientPath));
    // brushLoader.load(std::format("{}/palettes/tilesets.json", clientPath));

    return app.exec();
}

int main(int argc, char **argv)
{
    // qInstallMessageHandler(pipeQtMessagesToStd);
    showQmlResources();

    ItemPalettes::createPalette("Default", "default");

    Random::global().setSeed(123);
    TimePoint::setApplicationStartTimePoint();

    // std::string version = "12.81";
    std::string version = "12.87.12091";
    std::string clientPath = std::format("data/clients/{}", version);

    auto configResult = Config::create(version);
    if (configResult.isErr())
    {
        VME_LOG(configResult.unwrapErr().show());
        return EXIT_FAILURE;
    }

    Config config = configResult.unwrap();
    config.loadOrTerminate();

    MainApp app(argc, argv);

    return app.start();
}