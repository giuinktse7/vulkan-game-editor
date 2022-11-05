#pragma once

#include <QObject>
#include <QtQml/qqmlregistration.h>

class MyType : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(int answer READ answer CONSTANT)
  public:
    int answer() const;
};