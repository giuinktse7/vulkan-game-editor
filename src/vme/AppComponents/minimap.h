#pragma once

#include <memory>

#include <QImage>
#include <QSize>
#include <QTimer>

#include "core/time_util.h"

class MapView;

class Minimap : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QImage image MEMBER _image READ image NOTIFY minimapChanged)
    Q_PROPERTY(bool open MEMBER _open READ open WRITE setOpen NOTIFY openChanged)

  public:
    void setMapView(std::weak_ptr<MapView> mapView);

    Q_INVOKABLE void setWidth(int width);
    Q_INVOKABLE void setHeight(int height);

    void resize(int width, int height);

    bool open()
    {
        return _open;
    }
    void setOpen(bool open);

    explicit Minimap(QObject *parent = nullptr);

    QImage image() const;
    void update();

  signals:
    void minimapChanged();
    void openChanged();

  private:
    QImage createMinimap();

    std::weak_ptr<MapView> _mapView;

    uint32_t refreshCooldownMs = 16;

    QImage _image;
    QSize _size;
    bool _open;

    // Used to avoid refreshing the minimap too often
    TimePoint lastRefresh;
    QTimer delayedRefresh;
};