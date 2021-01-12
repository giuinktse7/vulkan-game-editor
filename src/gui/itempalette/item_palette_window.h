#pragma once

#include <QListView>

#include <vector>

#include "../../item_palette.h"
#include "../qt_util.h"

class QComboBox;
class PaletteWidget;
class EditorAction;

namespace ItemPaletteUI
{
    class TilesetModel;
} // namespace ItemPaletteUI

class TilesetListView : public QListView
{
  public:
    TilesetListView(QWidget *parent = nullptr);

    Brush *brushAtIndex(QModelIndex index) const;

    void setTileset(Tileset *tileset);
    void scrollTo(int index);

    Tileset *tileset() const noexcept;

    void clear() noexcept;

  private:
    ItemPaletteUI::TilesetModel *_model;
};

class ItemPaletteWindow : public QWidget
{
    Q_OBJECT
  public:
    ItemPaletteWindow(EditorAction *editorAction, QWidget *parent = nullptr);

  signals:
    void brushSelectionEvent(Brush *brush);

  public:
    void addPalette(ItemPalette *itemPalette);

    void selectPalette(const std::string &paletteName);
    void selectPalette(const QString &paletteName);
    ItemPalette *selectedPalette() const;

    bool selectBrush(Brush *brush);

  private:
    void selectPalette(ItemPalette *itemPalette);
    void selectPalette(ItemPalette *itemPalette, Tileset *tileset);
    void onTilesetViewItemClicked(QModelIndex index);

    std::vector<ItemPalette *> itemPalettes;

    PaletteWidget *paletteWidget;
    QComboBox *paletteDropdown;

    EditorAction *editorAction;
};

class PaletteWidget : public QWidget
{
  public:
    PaletteWidget(QWidget *parent = nullptr);

    TilesetListView *tilesetListView() const noexcept;

    ItemPalette *palette() const;
    void setPalette(ItemPalette *palette, Tileset *tileset = nullptr);

    void selectTileset(const std::string &tilesetName);
    void selectTileset(const QString &tilesetName);
    Tileset *tileset() const noexcept;

    void selectBrush(Brush *brush);

  private:
    // UI
    QComboBox *_tilesetDropdown = nullptr;
    TilesetListView *_tilesetListView = nullptr;

    ItemPalette *_palette = nullptr;
};

class TilesetListEventFilter : public QtUtil::EventFilter
{
  public:
    TilesetListEventFilter(QObject *parent)
        : QtUtil::EventFilter(parent) {}

    bool eventFilter(QObject *obj, QEvent *event) override;
};