#pragma once

#include <QListView>

class QComboBox;

namespace ItemPaletteUI
{
    class TilesetModel;
    struct ModelItem;
} // namespace ItemPaletteUI

class TilesetListView : public QListView
{
  public:
    TilesetListView(QWidget *parent = nullptr);

    void addItem(uint32_t serverId);
    void addItems(std::vector<uint32_t> &&serverIds);
    void addItems(uint32_t from, uint32_t to);

    ItemPaletteUI::ModelItem itemAtIndex(QModelIndex index);

  private:
    ItemPaletteUI::TilesetModel *_model;
};

class ItemPaletteWindow : public QWidget
{
  public:
    ItemPaletteWindow(QWidget *parent = nullptr);

    TilesetListView *tilesetListView() const noexcept
    {
        return _tilesetListView;
    }

    // void addPalette()

  private:
    TilesetListView *_tilesetListView;
    QComboBox *dropdown;
};
