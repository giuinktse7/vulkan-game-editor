#include "camera.h"

#include "logger.h"

constexpr int MinZoomStep = 0;
constexpr int MaxZoomStep = 20;

Camera::Camera()
		: floor(7),
			zoomStep(10),
			zoomChanged(false),
			_zoomFactor(1.0f),
			_position(0.0, 0.0)
{
}

void Camera::updateZoom(ScreenPosition cursorPos)
{
	if (!zoomChanged)
		return;

	zoomChanged = false;
	int cursorX = cursorPos.x;
	int cursorY = cursorPos.y;

	float n = 0.1f;
	float newZoomFactor = n * exp(log(1 / n) / 10 * zoomStep);

	WorldPosition newPos(this->_position);

	newPos.x += cursorX / _zoomFactor;
	newPos.x -= cursorX / newZoomFactor;

	newPos.y += cursorY / _zoomFactor;
	newPos.y -= cursorY / newZoomFactor;

	this->setPosition(newPos);

	_zoomFactor = newZoomFactor;
}

void Camera::setPosition(WorldPosition position)
{
	this->_position = WorldPosition(std::max(position.x, 0L), std::max(position.y, 0L));
}

void Camera::translate(WorldPosition delta)
{
	setPosition(this->_position + delta);
}

void Camera::translateZ(int z)
{
	floor = static_cast<float>(std::clamp(static_cast<int>(floor + z), 0, 15));
}

void Camera::zoomIn()
{
	setZoomStep(this->zoomStep + 1);
}

void Camera::zoomOut()
{
	setZoomStep(this->zoomStep - 1);
}

void Camera::resetZoom()
{
	setZoomStep(10);
}

void Camera::setZoomStep(int zoomStep)
{
	if (this->zoomStep != zoomStep)
	{
		this->zoomStep = std::clamp(zoomStep, MinZoomStep, MaxZoomStep);
		zoomChanged = true;
	}
}