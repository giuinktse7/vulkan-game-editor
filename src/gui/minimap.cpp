#include "minimap.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QResizeEvent>
#include <QShowEvent>

#include "../camera.h"
#include "../map_view.h"
#include "../minimap_colors.h"
#include "mainwindow.h"

MinimapWidget::MinimapWidget(MainWindow *mainWindow)
    : QWidget(mainWindow), mainWindow(mainWindow), imageContainer(new QLabel()), lastRefresh(TimePoint::now())
{
    setWindowFlags(Qt::Tool | Qt::Dialog);
    setWindowTitle("Minimap");
    _size = QSize(200, 200);
    setMinimumSize(100, 100);

    setStyleSheet("background-color: #444;");

    delayedRefresh.setSingleShot(true);
    connect(&delayedRefresh, &QTimer::timeout, [this]() {
        update();
    });

    auto layout = new QHBoxLayout(this);

    layout->setContentsMargins(1, 1, 1, 1);
    layout->setSpacing(0);
    setLayout(layout);

    QSizePolicy po;
    po.setHorizontalPolicy(QSizePolicy::Preferred);
    po.setVerticalPolicy(QSizePolicy::Preferred);
    po.setVerticalStretch(1);
    po.setHorizontalStretch(1);

    imageContainer->setSizePolicy(po);

    layout->addWidget(imageContainer);
}

void MinimapWidget::update()
{
    if (!isVisible())
        return;

    auto elapsedMillis = lastRefresh.elapsedMillis();
    if (elapsedMillis < refreshCooldownMs)
    {
        if (!delayedRefresh.isActive())
        {
            delayedRefresh.start(refreshCooldownMs - elapsedMillis);
        }
        return;
    }

    lastRefresh = TimePoint::now();
    MapView *mapView = mainWindow->currentMapView();
    if (!mapView)
    {
        ABORT_PROGRAM("Tried to update minimap without a mapView set.");
    }

    const Camera &camera = mapView->camera();

    canvas = QImage(QSize(_size), QImage::Format::Format_ARGB32);
    canvas.fill(Qt::black);

    auto viewportMidPoint = camera.position() + Position(camera.viewport().gameWidth() / 2, camera.viewport().gameHeight() / 2, 0);

    Position delta = Position(_size.width() / 2, _size.height() / 2, 0);

    int offsetX = viewportMidPoint.x - delta.x;
    int offsetY = viewportMidPoint.y - delta.y;

    auto from = Position(std::max(0, offsetX), std::max(0, offsetY), viewportMidPoint.z);
    auto to = viewportMidPoint + delta;

    for (auto &tileLocation : mapView->map()->getRegion(from, to))
    {
        if (!tileLocation.hasTile())
            continue;

        Tile *tile = tileLocation.tile();
        uint8_t colorId = tile->minimapColor();
        if (colorId != 0)
        {
            auto color = MinimapColors::colors[tile->minimapColor()];

            Position tilePos = tile->position();
            int x = tilePos.x - offsetX;
            int y = tilePos.y - offsetY;

            canvas.setPixel(x, y, (color.a << 24) | (color.r << 16) | (color.g << 8) | color.b);
        }
    }

    imageContainer->setPixmap(QPixmap::fromImage(canvas));
}

void MinimapWidget::resizeEvent(QResizeEvent *event)
{
    _size = event->size();
    update();
}

void MinimapWidget::showEvent(QShowEvent *event)
{
    resize(_size.width(), _size.height());
    update();
}

void MinimapWidget::toggle()
{
    if (isHidden())
    {
        show();
    }
    else
    {
        hide();
    }
}
