#pragma once

#include <QImage>
#include <QTimer>
#include <QWidget>

#include <optional>

#include "../time_point.h"

class QResizeEvent;
class QShowEvent;
class QImage;
class QLabel;
class MapView;
class MainWindow;

class MinimapWidget : public QWidget
{
  public:
    MinimapWidget(MainWindow *mainWindow);

    void update();
    void toggle();

  protected:
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

  private:
    uint32_t refreshCooldownMs = 16;
    QSize _size;

    QImage canvas;
    QLabel *imageContainer;

    MainWindow *mainWindow;

    // Used to avoid refreshing the minimap too often
    TimePoint lastRefresh;
    QTimer delayedRefresh;
};