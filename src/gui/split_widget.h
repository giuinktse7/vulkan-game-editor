#pragma once

#include <QSplitter>
#include <QSplitterHandle>

class Splitter : public QSplitter
{
public:
  Splitter(Qt::Orientation orientation = Qt::Orientation::Horizontal, QWidget *parent = nullptr);

protected:
  QSplitterHandle *createHandle() override;
};

class SplitterHandle : public QSplitterHandle
{
public:
  SplitterHandle(Qt::Orientation o, QSplitter *parent = nullptr);

protected:
  void resizeEvent(QResizeEvent *event) override;
  void paintEvent(QPaintEvent *event) override;
};