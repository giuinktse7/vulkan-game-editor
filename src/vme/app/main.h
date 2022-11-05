#pragma once

#include <QGuiApplication>
#include <QtQuick/QQuickView>
#include <memory>

class MainApp
{
  public:
    MainApp(int argc, char **argv);

    int start();

  private:
    QGuiApplication app;

    std::unique_ptr<QQuickView> rootView;
};