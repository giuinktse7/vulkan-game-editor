#include "camera.h"

#include "logger.h"

constexpr int MinZoomStep = 0;
constexpr int MaxZoomStep = 20;
constexpr int DefaultZoomStep = (MaxZoomStep - MinZoomStep) / 2;

Camera::Camera()
		: floor(7),
			zoomStep(DefaultZoomStep),
			_zoomFactor(1.0f),
			_position(0.0, 0.0)
{
}

void Camera::updateZoom(ScreenPosition zoomOrigin)
{
	auto [originX, originY] = zoomOrigin;

	float n = 0.1f;
	float newZoomFactor = n * exp(log(1 / n) / 10 * zoomStep);

	WorldPosition newPos(this->_position);

	newPos.x += originX / _zoomFactor;
	newPos.x -= originX / newZoomFactor;

	newPos.y += originY / _zoomFactor;
	newPos.y -= originY / newZoomFactor;

	this->setPosition(newPos);

	_zoomFactor = newZoomFactor;
}

void Camera::setPosition(WorldPosition position) noexcept
{
	this->_position = WorldPosition(std::max(position.x, 0), std::max(position.y, 0));
}

void Camera::translate(WorldPosition delta)
{
	setPosition(this->_position + delta);
}

void Camera::translateZ(int z)
{
	floor = static_cast<float>(std::clamp(static_cast<int>(floor + z), 0, MAP_LAYERS - 1));
}

void Camera::zoomIn(ScreenPosition zoomOrigin)
{
	setZoomStep(this->zoomStep + 1, zoomOrigin);
}

void Camera::zoomOut(ScreenPosition zoomOrigin)
{
	setZoomStep(this->zoomStep - 1, zoomOrigin);
}

void Camera::resetZoom(ScreenPosition zoomOrigin)
{
	setZoomStep(DefaultZoomStep, zoomOrigin);
}

void Camera::setZoomStep(int zoomStep, ScreenPosition zoomOrigin)
{
	int inZoomStep = std::clamp(zoomStep, MinZoomStep, MaxZoomStep);
	if (this->zoomStep != inZoomStep)
	{
		this->zoomStep = inZoomStep;
		updateZoom(zoomOrigin);
	}
}