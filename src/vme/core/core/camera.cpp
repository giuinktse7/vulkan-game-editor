#include "camera.h"

#include "const.h"
#include "logger.h"

namespace
{
    constexpr int MinZoomStep = 0;
    constexpr int MaxZoomStep = 20;
    constexpr int DefaultZoomStep = (MaxZoomStep - MinZoomStep) / 2;
} // namespace

Camera::Camera()
    : _viewport{0, 0, GROUND_FLOOR, 0, 0, 1.0f},
      _zoomStep(DefaultZoomStep) {}

float Camera::computeZoomFactor() const
{
    float n = 0.1f;
    return n * exp(log(1 / n) / 10 * _zoomStep);
}

void Camera::updateZoom(ScreenPosition zoomOrigin)
{
    float newZoomFactor = computeZoomFactor();

    float scale = (1 / _viewport.zoom) - (1 / newZoomFactor);
    ScreenPosition scaled = zoomOrigin * scale;

    WorldPosition newPosition(_viewport.x + scaled.x, _viewport.y + scaled.y);
    _viewport.zoom = newZoomFactor;

    this->setWorldPosition(newPosition, false);
    fireViewportChange();
}

void Camera::setWorldPosition(WorldPosition position, bool notifyViewportChange) noexcept
{
    int oldX = _viewport.x;
    int oldY = _viewport.y;

    _viewport.x = std::max(position.x, 0);
    _viewport.y = std::max(position.y, 0);

    if (notifyViewportChange && (_viewport.x != oldX || _viewport.y != oldY))
    {
        fireViewportChange();
    }
}

void Camera::translate(WorldPosition delta)
{
    setWorldPosition(WorldPosition(_viewport.x, _viewport.y) + delta);
}

void Camera::translateX(WorldPosition::value_type x)
{
    setWorldPosition(WorldPosition(_viewport.x + x, _viewport.y));
}

void Camera::translateY(WorldPosition::value_type y)
{
    setWorldPosition(WorldPosition(_viewport.x, _viewport.y + y));
}

void Camera::zoomIn(ScreenPosition zoomOrigin)
{
    setZoomStep(this->_zoomStep + 1, zoomOrigin);
}

void Camera::zoomOut(ScreenPosition zoomOrigin)
{
    setZoomStep(this->_zoomStep - 1, zoomOrigin);
}

void Camera::resetZoom(ScreenPosition zoomOrigin)
{
    setZoomStep(DefaultZoomStep, zoomOrigin);
}

void Camera::setZoomStep(int zoomStep, ScreenPosition zoomOrigin)
{
    int inZoomStep = std::clamp(zoomStep, MinZoomStep, MaxZoomStep);
    if (this->_zoomStep != inZoomStep)
    {
        this->_zoomStep = inZoomStep;
        updateZoom(zoomOrigin);
    }
}

uint32_t Camera::Viewport::gameWidth() const
{
    return static_cast<uint32_t>(std::ceil(width / (zoom * MapTileSize)));
}

uint32_t Camera::Viewport::gameHeight() const
{
    return static_cast<uint32_t>(std::ceil(height / (zoom * MapTileSize)));
}

Position Camera::Viewport::midPoint() const
{
    return WorldPosition(x + width / 2, y + height / 2).toPos(z);
}
