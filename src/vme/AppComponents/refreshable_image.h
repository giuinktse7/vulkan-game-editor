#pragma once

#include <QImage>
#include <QPainter>
#include <QQuickPaintedItem>

class RefreshableImage : public QQuickPaintedItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QImage image MEMBER _image WRITE setImage)

  public:
    explicit RefreshableImage(QQuickItem *parent = nullptr);
    void setImage(const QImage &image);
    void paint(QPainter *painter) override;

  private:
    QImage _image;
};