#include "qt_util.h"

#include <optional>

#include <QApplication>
#include <QWheelEvent>

#include "../items.h"

QPixmap QtUtil::itemPixmap(uint16_t serverId)
{
  ItemType *t = Items::items.getItemType(serverId);
  auto &info = t->getTextureInfoUnNormalized();

  const uint8_t *pixelData = info.atlas->getOrCreateTexture().pixels().data();
  QImage img(pixelData, 12 * 32, 12 * 32, 384 * 4, QImage::Format::Format_ARGB32);

  QPixmap pixelMap = QPixmap::fromImage(img);

  QRect textureRegion(info.window.x0, info.window.y0, info.window.x1, info.window.y1);

  return pixelMap.copy(textureRegion).transformed(QTransform().scale(1, -1));
}

MainApplication *QtUtil::qtApp()
{
  return (MainApplication *)(QApplication::instance());
}

std::optional<int> QtUtil::ScrollState::scroll(QWheelEvent *event)
{
  // The relative amount that the wheel was rotated, in eighths of a degree.
  const int deltaY = event->angleDelta().y();
  amountBuffer += deltaY;

  if (std::abs(amountBuffer) < minRotationDelta)
    return {};

  int result = amountBuffer / QtMinimumWheelDelta;
  amountBuffer = amountBuffer % QtMinimumWheelDelta;

  return result;
}