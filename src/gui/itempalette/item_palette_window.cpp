#include "item_palette_window.h"

#include <QComboBox>
#include <QVBoxLayout>

#include "../../editor_action.h"
#include "../../items.h"
#include "../../logger.h"
#include "../../qt/logging.h"
#include "../main_application.h"

#include "item_palette_model.h"

using TilesetModel = ItemPaletteUI::TilesetModel;

ItemPaletteWindow::ItemPaletteWindow(EditorAction *editorAction, QWidget *parent)
    : QWidget(parent),
      paletteDropdown(new QComboBox(this)),
      paletteWidget(new PaletteWidget(this)),
      editorAction(editorAction)
{
    auto layout = new QVBoxLayout();
    setLayout(layout);

    setContentsMargins(0, 0, 0, 0);

    layout->addWidget(paletteDropdown);
    layout->addWidget(paletteWidget);

    connect(paletteDropdown, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) {
        selectPalette(paletteDropdown->currentText());
    });

    connect(paletteWidget->tilesetListView(), &QListView::clicked, this, &ItemPaletteWindow::onTilesetViewItemClicked);
}

void ItemPaletteWindow::onTilesetViewItemClicked(QModelIndex index)
{
    Brush *brush = paletteWidget->tilesetListView()->brushAtIndex(index);
    emit brushSelectionEvent(brush);
}

void ItemPaletteWindow::addPalette(ItemPalette *itemPalette)
{
    bool firstPalette = itemPalettes.empty();
    itemPalettes.emplace_back(itemPalette);

    paletteDropdown->addItem(QString::fromStdString(itemPalette->name()));

    if (firstPalette)
    {
        selectPalette(itemPalette);
    }
}

void ItemPaletteWindow::selectPalette(ItemPalette *itemPalette)
{
    selectPalette(itemPalette->name());
}

void ItemPaletteWindow::selectPalette(const std::string &paletteName)
{
    selectPalette(QString::fromStdString(paletteName));
}

void ItemPaletteWindow::selectPalette(const QString &paletteName)
{
    int index = paletteDropdown->findText(paletteName);

    if (index == -1)
    {
        VME_LOG_ERROR("ItemPaletteWindow did not have a palette with name " << paletteName << "(in paletteDropdown).");
        return;
    }

    std::string stdName = paletteName.toStdString();

    auto found = std::find_if(
        itemPalettes.begin(),
        itemPalettes.end(),
        [&stdName](ItemPalette *p) { return p->name() == stdName; });

    if (found == itemPalettes.end())
    {
        VME_LOG_ERROR("ItemPaletteWindow did not have a palette with name " << paletteName << "(in itemPalettes).");
        return;
    }

    paletteDropdown->setCurrentIndex(index);
    paletteWidget->setPalette(*found);
}

void ItemPaletteWindow::selectPalette(ItemPalette *itemPalette, Tileset *tileset)
{
    int dropdownIndex = paletteDropdown->findText(QString::fromStdString(itemPalette->name()));
    if (dropdownIndex == -1)
    {
        VME_LOG_ERROR("ItemPaletteWindow did not have a palette with name " << itemPalette->name() << "(in paletteDropdown).");
        return;
    }

    auto found = std::find_if(
        itemPalettes.begin(),
        itemPalettes.end(),
        [itemPalette](ItemPalette *p) { return p == itemPalette; });

    if (found == itemPalettes.end())
    {
        VME_LOG_ERROR("ItemPaletteWindow did not have a palette with name " << itemPalette->name() << "(in itemPalettes).");
        return;
    }

    paletteDropdown->setCurrentIndex(dropdownIndex);
    paletteWidget->setPalette(itemPalette, tileset);
}

bool ItemPaletteWindow::selectBrush(Brush *brush)
{
    Tileset *tileset = brush->tileset();
    if (!tileset)
        return false;

    ItemPalette *palette = tileset->palette();
    if (!palette)
        return false;

    selectPalette(palette, tileset);
    paletteWidget->selectBrush(brush);
    // TODO scroll to the brush in the TilesetListView
    editorAction->setBrush(brush);

    return true;
}

TilesetListView::TilesetListView(QWidget *parent)
    : QListView(parent)
{
    // setAlternatingRowColors(true);
    setFlow(QListView::Flow::LeftToRight);
    setWrapping(true);
    setResizeMode(QListView::ResizeMode::Adjust);
    setItemDelegate(new ItemPaletteUI::ItemDelegate(this));
    setSpacing(1);

    setSelectionBehavior(QAbstractItemView::SelectRows);

    _model = new TilesetModel(this);
    setModel(_model);

    installEventFilter(new TilesetListEventFilter(this));

    auto modelPtr = _model;
    connect(selectionModel(), &QItemSelectionModel::selectionChanged, [modelPtr](const QItemSelection &selected, const QItemSelection &deselected) {
        auto selectionIndices = selected.indexes();
        if (!selectionIndices.empty())
        {
            modelPtr->highlightIndex(selectionIndices.front());
        }

        auto deselectionIndices = selected.indexes();
        if (!deselectionIndices.empty())
        {
            modelPtr->setData(deselectionIndices.front(), -1, TilesetModel::HighlightRole);
        }
    });

    // setSelectionRectVisible(true);

    QPalette stylePalette = palette();
    stylePalette.setColor(QPalette::Base, "#000000");
    setPalette(stylePalette);
}

void TilesetListView::setTileset(Tileset *tileset)
{
    _model->setTileset(tileset);
}

Tileset *TilesetListView::tileset() const noexcept
{
    return _model->tileset();
}

Brush *TilesetListView::brushAtIndex(QModelIndex index) const
{
    return _model->brushAtIndex(index.row());
}

void TilesetListView::clear() noexcept
{
    _model->clear();
}

void TilesetListView::selectAndScrollTo(int index)
{
    QModelIndex modelIndex = _model->index(index);
    scrollTo(modelIndex);
    setCurrentIndex(modelIndex);
}

//>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>PaletteWidget>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>
//>>>>>>>>>>>>>>>>>>>>>>>>>>

PaletteWidget::PaletteWidget(QWidget *parent)
    : QWidget(parent),
      _tilesetDropdown(new QComboBox(this)),
      _tilesetListView(new TilesetListView(this))
{
    setContentsMargins(0, 0, 0, 0);

    auto layout = new QVBoxLayout();
    layout->setContentsMargins(0, 0, 0, 0);
    setLayout(layout);

    layout->addWidget(_tilesetDropdown);
    layout->addWidget(_tilesetListView);

    connect(_tilesetDropdown, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) {
        selectTileset(_tilesetDropdown->currentText());
    });
}

void PaletteWidget::selectTileset(const std::string &tilesetName)
{
    auto tileset = _palette->tileset(tilesetName);
    _tilesetListView->setTileset(tileset);
}

void PaletteWidget::selectTileset(const QString &tilesetName)
{
    selectTileset(tilesetName.toStdString());
}

TilesetListView *PaletteWidget::tilesetListView() const noexcept
{
    return _tilesetListView;
}

ItemPalette *PaletteWidget::palette() const
{
    return _palette;
}

void PaletteWidget::selectBrush(Brush *brush)
{
    Tileset *currentTileset = tileset();
    Tileset *brushTileset = brush->tileset();
    DEBUG_ASSERT(brushTileset != nullptr && currentTileset == brushTileset, "The brush is not part of the current tileset.");

    ItemPalette *brushPalette = brushTileset->palette();
    DEBUG_ASSERT(brushPalette != nullptr && brushPalette == _palette, "The brush palette is not the current palette.");

    int index = currentTileset->indexOf(brush);

    if (index == -1)
    {
        ABORT_PROGRAM("The brush was not present in the current tileset.");
    }

    _tilesetListView->selectAndScrollTo(index);
}

Tileset *PaletteWidget::tileset() const noexcept
{
    return _tilesetListView->tileset();
}

void PaletteWidget::setPalette(ItemPalette *palette, Tileset *tileset)
{
    // TODO Improve the control flow & readability of this function.

    if (palette == _palette)
    {
        if (tileset == _tilesetListView->tileset())
        {
            return;
        }

        if (tileset == nullptr)
        {
            _tilesetListView->setTileset(palette->tileset(0));
        }
        else
        {
            _tilesetListView->setTileset(tileset);

            int index = _tilesetDropdown->findText(QString::fromStdString(tileset->name()));
            DEBUG_ASSERT(index != -1, "The tileset was not present in the dropdown.");
            _tilesetDropdown->setCurrentIndex(index);
        }
    }
    else
    {
        _palette = palette;

        _tilesetDropdown->clear();

        if (palette->empty())
        {
            _tilesetListView->clear();
        }
        else
        {
            for (const auto &tileset : palette->tilesets())
            {
                _tilesetDropdown->addItem(QString::fromStdString(tileset->name()));
            }
            if (tileset == nullptr)
            {
                _tilesetListView->setTileset(palette->tileset(0));
                _tilesetDropdown->setCurrentIndex(0);
            }
            else
            {
                _tilesetListView->setTileset(tileset);

                int index = _tilesetDropdown->findText(QString::fromStdString(tileset->name()));
                DEBUG_ASSERT(index != -1, "The tileset was not present in the dropdown.");
                _tilesetDropdown->setCurrentIndex(index);
            }
        }
    }
}

bool TilesetListEventFilter::eventFilter(QObject *object, QEvent *event)
{
    switch (event->type())
    {
        case QEvent::KeyPress:
        {
            // VME_LOG_D("Focused widget: " << QApplication::focusWidget());
            auto keyEvent = static_cast<QKeyEvent *>(event);
            auto key = keyEvent->key();
            switch (key)
            {
                case Qt::Key_I:
                case Qt::Key_Space:
                {
                    auto widget = QtUtil::qtApp()->widgetAt(QCursor::pos());
                    auto vulkanWindow = QtUtil::associatedVulkanWindow(widget);
                    if (vulkanWindow)
                    {
                        QApplication::sendEvent(vulkanWindow, event);
                    }

                    return false;
                    break;
                }
            }
            break;
        }
        default:
            break;
    }

    return QObject::eventFilter(object, event);
}