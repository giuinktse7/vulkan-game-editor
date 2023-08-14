#include "refreshable_image.h"

RefreshableImage::RefreshableImage(QQuickItem *parent)
    : QQuickPaintedItem(parent), _image{}
{
}

void RefreshableImage::paint(QPainter *painter)
{
    painter->drawImage(0, 0, _image);
}

void RefreshableImage::setImage(const QImage &image)
{
    _image = image;

    update();
}