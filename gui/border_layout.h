#pragma once

#include <vector>

#include <QLayout>
#include <QRect>

class BorderLayout : public QLayout
{
public:
  enum class Position
  {
    West,
    North,
    South,
    East,
    Center
  };

  explicit BorderLayout(QWidget *parent, const QMargins &margins = QMargins(), int spacing = -1);
  BorderLayout(int spacing = 0);
  ~BorderLayout();

  void setGeometry(const QRect &rect) override;
  void addItem(QLayoutItem *item) override;
  void addWidget(QWidget *widget, Position position);
  QLayoutItem *takeAt(int index) override;

  Qt::Orientations expandingDirections() const override;
  bool hasHeightForWidth() const override;
  int count() const override;
  QLayoutItem *itemAt(int index) const override;
  QSize minimumSize() const override;
  QSize sizeHint() const override;

  void add(QLayoutItem *item, Position position);

private:
  struct ItemWrapper
  {
    ItemWrapper(QLayoutItem *item, Position position)
    {
      this->item = item;
      this->position = position;
    }

    QLayoutItem *item;
    Position position;
  };

  enum class SizeType
  {
    MinimumSize,
    SizeHint
  };
  QSize calculateSize(SizeType sizeType) const;

  std::vector<ItemWrapper> items;
};
