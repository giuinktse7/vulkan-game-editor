#include "camera.h"

#include "const.h"
#include "logger.h"
#include "map.h"

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
	this->setWorldPosition(newPosition);

	_viewport.zoom = newZoomFactor;
	fireViewportChange();
}

void Camera::setWorldPosition(WorldPosition position) noexcept
{
	int oldX = _viewport.x;
	int oldY = _viewport.y;

	_viewport.x = std::max(position.x, 0);
	_viewport.y = std::max(position.y, 0);

	if (_viewport.x != oldX || _viewport.y != oldY)
		fireViewportChange();
}

void Camera::translate(WorldPosition delta)
{
	setWorldPosition(WorldPosition(_viewport.x, _viewport.y) + delta);
}

void Camera::translateZ(int z)
{
	int oldFloor = _viewport.z;
	_viewport.z = std::clamp(static_cast<int>(_viewport.z + z), 0, MAP_LAYERS - 1);
	if (_viewport.z != oldFloor)
		fireViewportChange();
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
	return std::ceil<uint32_t>(width / (zoom * MapTileSize));
}

uint32_t Camera::Viewport::gameHeight() const
{
	return std::ceil<uint32_t>(height / (zoom * MapTileSize));
}