#include "minimap.h"

#include "core/map_view.h"
#include "core/minimap_colors.h"

Minimap::Minimap(QObject *parent)
    : QObject(parent), _size(200, 200), lastRefresh(TimePoint::now())
{
    delayedRefresh.setSingleShot(true);
    connect(&delayedRefresh, &QTimer::timeout, [this]() {
        update();
    });

    _image = createMinimap();
}

void Minimap::update()
{
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

    _image = createMinimap();
    emit minimapChanged();
}

QImage Minimap::image() const
{
    return _image;
}

void Minimap::setWidth(int width)
{
    if (_size.width() == width)
    {
        return;
    }

    _size.setWidth(width);
    update();
}

void Minimap::setHeight(int height)
{
    if (_size.height() == height)
    {
        return;
    }

    _size.setHeight(height);
    update();
}

QImage Minimap::createMinimap()
{
    if (auto mapView = _mapView.lock())
    {
        const Camera &camera = mapView->camera();

        QImage canvas = QImage(_size, QImage::Format::Format_ARGB32);
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

        return canvas;
    }
    else
    {
        QImage blankImage(_size.width(), _size.height(), QImage::Format::Format_ARGB32);
        blankImage.fill(QColor(0, 0, 0, 255));

        return blankImage;
    }
}

void Minimap::resize(int width, int height)
{
    if (width != _size.width() || height != _size.height())
    {
        _size = QSize(width, height);
        update();
    }
}

void Minimap::setMapView(std::weak_ptr<MapView> mapView)
{
    _mapView = mapView;
    update();
}