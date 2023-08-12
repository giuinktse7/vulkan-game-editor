#include "search_result_model.h"

#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlProperty>
#include <QQuickItem>

#include "gui_thing_image.h"
#include "ui_resources.h"

// void SearchPopupView::brushSelected(int index)
// {
//     auto modelIndex = filteredSearchModel.index(index, 0);
//     auto indexInUnFilteredList = filteredSearchModel.data(modelIndex, to_underlying(SearchResultModel::Role::VectorIndex)).toInt();

//     mainWindow->selectBrush(searchResultModel.brushAtIndex(indexInUnFilteredList));
//     emit requestClose();
// }

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
        return UIResource::resourcePath(brush);
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
    std::optional<BrushType> brushType = Brush::parseBrushType(s.toStdString());
    if (!brushType)
    {
        VME_LOG_ERROR("FilteredSearchModel::setFilter Unknown filter type: " << s.toStdString());
        reset();
        return;
    }

    setPredicate([brushType](Brush *brush) { return brush->type() == brushType; });
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
    return Brush::parseBrushType(rawBrushType.toStdString());
}
