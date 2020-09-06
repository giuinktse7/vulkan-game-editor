#include "camera.h"

#include "Logger.h"
#include "position.h"

void Camera::updateZoom(ScreenPosition cursorPos)
{
	if (!zoomChanged)
		return;

	zoomChanged = false;
	float cursorX = static_cast<float>(cursorPos.x);
	float cursorY = static_cast<float>(cursorPos.y);

	float n = 0.1f;
	float zoomFactor = n * exp(log(1 / n) / 10 * zoomStep);

	glm::vec2 newPos{this->position};

	newPos.x += cursorX / this->zoomFactor;
	newPos.x -= cursorX / zoomFactor;

	newPos.y += cursorY / this->zoomFactor;
	newPos.y -= cursorY / zoomFactor;

	this->setPosition(newPos);

	this->zoomFactor = zoomFactor;
}

void Camera::setPosition(glm::vec2 position)
{
	this->position = glm::vec2(std::max(position.x, 0.0f), std::max(position.y, 0.0f));
}

void Camera::translate(glm::vec2 delta)
{
	setPosition(this->position + delta);
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
		this->zoomStep = std::clamp(zoomStep, 0, zoomSteps);
		zoomChanged = true;
	}
}