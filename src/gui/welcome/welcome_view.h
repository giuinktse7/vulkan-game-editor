#pragma once

#include <QObject>
#include <QQuickView>
#include <QVariant>

#include <string>

#include "../main_application.h"
#include "welcome_view_model.h"

class WelcomeView : public QObject
{
    Q_OBJECT
  public:
    WelcomeView(QObject *parent = nullptr);
    WelcomeView(MainApplication *app, QObject *parent = nullptr);

    Q_INVOKABLE void openMainWindow();
    Q_INVOKABLE void selectMapFile();

    void show();
    void close();

  private:
    void readRecentFiles();

    MainApplication *app;
    QQuickView *view = nullptr;

    WelcomeViewModel viewModel;
};