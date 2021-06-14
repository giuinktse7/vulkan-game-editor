#include "search_popup.h"

#include <QGraphicsDropShadowEffect>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlProperty>
#include <QQuickItem>
#include <QWidget>

#include "mainwindow.h"
#include "qt_util.h"

SearchPopupView::SearchPopupView(QUrl filepath, MainWindow *mainWindow)
    : _filepath(filepath), mainWindow(mainWindow), _wrapperWidget(nullptr)
{
    installEventFilter(new SearchWrapperEventFilter(this));
    filteredSearchModel.setSourceModel(&searchResultModel);

    QVariantMap properties;
    properties.insert("searchResults", QVariant::fromValue(&filteredSearchModel));

    setInitialProperties(properties);

    qmlRegisterSingletonInstance("Vme.context", 1, 0, "C_SearchPopupView", this);
    engine()->addImageProvider(QLatin1String("itemTypes"), new ItemTypeImageProvider);
    engine()->addImageProvider(QLatin1String("creatureLooktypes"), new CreatureImageProvider);

    setSource(filepath);
    setResizeMode(ResizeMode::SizeRootObjectToView);

    QmlApplicationContext *applicationContext = new QmlApplicationContext();
    engine()->rootContext()->setContextProperty("applicationContext", applicationContext);
}

void SearchPopupView::focus()
{
    _wrapperWidget->setFocus();
    QMetaObject::invokeMethod(rootObject(), "focusSearchTextInput");
}

QWidget *SearchPopupView::wrapInWidget(QWidget *parent)
{
    DEBUG_ASSERT(_wrapperWidget == nullptr, "There is already a wrapper for this window.");

    _wrapperWidget = QWidget::createWindowContainer(this, parent);
    _wrapperWidget->setObjectName("SearchPopupView wrapper");

    return _wrapperWidget;
}

void SearchPopupView::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
    {
        emit requestClose();
    }

    QQuickView::keyPressEvent(event);
}

void SearchPopupView::closeEvent()
{
    emit requestClose();
}

void SearchPopupView::hideEvent(QHideEvent *e)
{
    QMetaObject::invokeMethod(rootObject(), "resetSearchText");
    QQuickView::hideEvent(e);
}

void SearchPopupView::search(std::string searchTerm)
{
    auto results = Brush::search(searchTerm);
    searchResultModel.setSearchResults(std::move(results));
}

SearchWrapperEventFilter::SearchWrapperEventFilter(SearchPopupView *parent)
    : QObject(static_cast<QObject *>(parent)), searchPopupView(parent) {}

bool SearchWrapperEventFilter::eventFilter(QObject *obj, QEvent *event)
{
    switch (event->type())
    {
        case QEvent::FocusOut:
            searchPopupView->closeEvent();
            break;
        default:
            break;
    }

    return QObject::eventFilter(obj, event);
}

SearchPopupWidget::SearchPopupWidget(MainWindow *mainWindow)
    : mainWindow(mainWindow)
{
    searchPopupView = new SearchPopupView(QUrl("qrc:/vme/qml/SearchPopupView.qml"), mainWindow);

    auto wrapperWidget = searchPopupView->wrapInWidget();

    wrapperWidget->setFixedWidth(mainWindow->width() * 0.6);
    wrapperWidget->setMinimumHeight(300);
    // wrapperWidget->setMaximumHeight(mainWindow->height() * 0.6);
    // searchPopupWidget->setFixedWidth(300);
    // searchPopupWidget->setFixedHeight(300);
    QVBoxLayout *searchLayout = new QVBoxLayout(this);
    searchLayout->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    searchLayout->setContentsMargins(0, 30, 0, 0);
    searchLayout->addWidget(wrapperWidget);
    setLayout(searchLayout);
}

void SearchPopupView::searchEvent(QString searchTerm)
{
    this->search(searchTerm.toStdString());
}

void SearchPopupView::setHeight(int height)
{
    _wrapperWidget->setFixedHeight(height);
}

void SearchPopupView::brushSelected(int index)
{
    auto modelIndex = filteredSearchModel.index(index, 0);
    auto indexInUnFilteredList = filteredSearchModel.data(modelIndex, to_underlying(SearchResultModel::Role::VectorIndex)).toInt();

    mainWindow->selectBrush(searchResultModel.brushAtIndex(indexInUnFilteredList));
    emit requestClose();
}

inline QObject *SearchPopupView::child(const char *name)
{
    return rootObject()->findChild<QObject *>(name);
}

SearchPopupView *SearchPopupWidget::popupView() const
{
    return searchPopupView;
}

void SearchPopupWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
    {
        searchPopupView->closeEvent();
    }
}

void SearchPopupView::reloadSource()
{
    VME_LOG_D("SearchPopupView source reloaded.");
    engine()->clearComponentCache();
    setSource(QUrl::fromLocalFile("../../resources/qml/SearchPopupView.qml"));
}

SearchResultModel::SearchResultModel(QObject *parent)
    : QAbstractListModel(parent), thingImageProvider(new ItemTypeImageProvider) {}

int SearchResultModel::rowCount(const QModelIndex &parent) const
{
    return _searchResults ? _searchResults->matches->size() : 0;
}

Brush *SearchResultModel::brushAtIndex(size_t index) const
{
    return _searchResults ? _searchResults->matches->at(index) : nullptr;
}

void SearchResultModel::clear()
{
    if (_searchResults)
    {
        beginResetModel();
        _searchResults.reset();
        endResetModel();
    }
}

void SearchResultModel::setSearchResults(BrushSearchResult &&brushes)
{
    beginResetModel();
    _searchResults = std::move(brushes);
    endResetModel();
}

QVariant SearchResultModel::data(const QModelIndex &index, int role) const
{
    if (index.row() < 0 || index.row() >= _searchResults->matches->size())
        return QVariant();

    Brush *brush = _searchResults->matches->at(index.row());

    if (role == to_underlying(Role::DisplayId))
    {
        if (brush->type() == BrushType::Raw)
        {
            auto displayId = brush->getDisplayId();
            return QString::fromStdString(displayId);
        }
        else
        {
            return QString();
        }
    }
    else if (role == to_underlying(Role::Name))
    {
        return QString::fromStdString(brush->name());
    }
    else if (role == to_underlying(Role::VectorIndex))
    {
        return index.row();
    }
    else if (role == to_underlying(Role::ResourceString))
    {
        return QtUtil::resourcePath(brush);
    }

    return QVariant();
}

FilteredSearchModel::FilteredSearchModel(QObject *parent)
    : QSortFilterProxyModel(parent) {}

void FilteredSearchModel::setSourceModel(QAbstractItemModel *model)
{
    _searchModel = dynamic_cast<SearchResultModel *>(model);
    connect(this, &SearchResultModel::modelReset, [this]() {
        emit searchModelChanged();
    });
    QSortFilterProxyModel::setSourceModel(model);
}

void FilteredSearchModel::setFilter(QString s)
{
    if (s == "raw")
    {
        setPredicate([](Brush *brush) { return brush->type() == BrushType::Raw; });
    }
    else if (s == "ground")
    {
        setPredicate([](Brush *brush) { return brush->type() == BrushType::Ground; });
    }
    else if (s == "doodad")
    {
        setPredicate([](Brush *brush) { return brush->type() == BrushType::Doodad; });
    }
    else if (s == "creature")
    {
        setPredicate([](Brush *brush) { return brush->type() == BrushType::Creature; });
    }
    else
    {
        VME_LOG_ERROR("FilteredSearchModel::setFilter Unknown filter type: " << s.toStdString());
        reset();
    }
}

void FilteredSearchModel::setPredicate(std::function<bool(Brush *)> predicate)
{
    this->predicate = predicate;
    invalidateFilter();
}

void FilteredSearchModel::resetFilter()
{
    reset();
}

QHash<int, QByteArray> SearchResultModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[to_underlying(Role::VectorIndex)] = "vectorIndex";
    roles[to_underlying(Role::DisplayId)] = "displayId";
    roles[to_underlying(Role::ResourceString)] = "resourceString";
    roles[to_underlying(Role::Name)] = "name";

    return roles;
}

void FilteredSearchModel::reset()
{
    predicate = acceptAll;
    invalidateFilter();
}

bool FilteredSearchModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    return predicate(static_cast<SearchResultModel *>(sourceModel())->brushAtIndex(sourceRow));
}

int FilteredSearchModel::totalCount() const
{
    const auto &results = _searchModel->searchResults();
    return results ? results->matches->size() : 0;
}

int FilteredSearchModel::rawCount() const
{
    const auto &results = _searchModel->searchResults();
    return results ? results->rawCount : 0;
}

int FilteredSearchModel::groundCount() const
{
    const auto &results = _searchModel->searchResults();
    return results ? results->groundCount : 0;
}

int FilteredSearchModel::doodadCount() const
{
    const auto &results = _searchModel->searchResults();
    return results ? results->doodadCount : 0;
}

int FilteredSearchModel::creatureCount() const
{
    const auto &results = _searchModel->searchResults();
    return results ? results->creatureCount : 0;
}

std::optional<BrushType> FilteredSearchModel::parseBrushType(QString rawBrushType)
{
    if (rawBrushType == "raw")
    {
        return BrushType::Raw;
    }
    else if (rawBrushType == "ground")
    {
        return BrushType::Ground;
    }
    else if (rawBrushType == "doodad")
    {
        return BrushType::Doodad;
    }
    else if (rawBrushType == "creature")
    {
        return BrushType::Creature;
    }
    else
    {
        return std::nullopt;
    }
}
