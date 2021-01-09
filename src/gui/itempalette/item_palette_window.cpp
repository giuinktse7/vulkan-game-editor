#include "item_palette_window.h"

#include <QComboBox>
#include <QVBoxLayout>

#include "../../items.h"
#include "../../logger.h"
#include "../qt_util.h"

#include "item_palette_model.h"

using ModelItem = ItemPaletteUI::ModelItem;
using TilesetModel = ItemPaletteUI::TilesetModel;

ItemPaletteWindow::ItemPaletteWindow(QWidget *parent)
    : QWidget(parent)
{

    auto layout = new QVBoxLayout();
    setLayout(layout);

    dropdown = new QComboBox;
    _tilesetListView = new TilesetListView;

    layout->addWidget(dropdown);
    layout->addWidget(_tilesetListView);
}

TilesetListView::TilesetListView(QWidget *parent)
    : QListView(parent)
{
    setAlternatingRowColors(true);
    setFlow(QListView::Flow::LeftToRight);
    setWrapping(true);
    setResizeMode(QListView::ResizeMode::Adjust);
    setItemDelegate(new ItemPaletteUI::ItemDelegate(this));

    _model = new TilesetModel(this);
    setModel(_model);
}

ModelItem TilesetListView::itemAtIndex(QModelIndex index)
{
    QVariant variant = _model->data(index);
    return variant.value<ModelItem>();
}

void TilesetListView::addItem(uint32_t serverId)
{
    _model->addItem(serverId);
}

void TilesetListView::addItems(std::vector<uint32_t> &&serverIds)
{
    _model->addItems(std::move(serverIds));
}

void TilesetListView::addItems(uint32_t from, uint32_t to)
{
    _model->addItems(from, to);
}
