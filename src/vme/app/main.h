#pragma once

#include <QGuiApplication>
#include <QtQuick/QQuickView>
#include <memory>

#include "app_data_model.h"
#include "minimap.h"
#include "tileset_model.h"

class MainApp
{
  public:
    MainApp(int argc, char **argv);

    int start();

  private:
    void createDefaultPalettes();

    QGuiApplication app;

    std::unique_ptr<QQuickView> rootView;

    // TODO Structure tileset model usage, this should not be stored in main!
    std::unique_ptr<TileSetModel> tilesetModel;

    std::unique_ptr<AppDataModel> dataModel;
    std::shared_ptr<Minimap> minimap;
};