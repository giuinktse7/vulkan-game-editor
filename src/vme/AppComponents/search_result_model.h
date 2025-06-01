#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QPixmap>
#include <QQuickView>
#include <QSortFilterProxyModel>

#include <functional>
#include <optional>

#include "core/brushes/brush.h"
#include "core/brushes/brushes.h"

class ItemTypeImageProvider;
class SearchResultModel;

class FilteredSearchModel : public QSortFilterProxyModel
{
    Q_OBJECT

  signals:
    void searchModelChanged();

  public:
    FilteredSearchModel(QObject *parent = 0);

    Q_PROPERTY(int totalCount READ totalCount NOTIFY searchModelChanged)
    Q_PROPERTY(int rawCount READ rawCount NOTIFY searchModelChanged)
    Q_PROPERTY(int groundCount READ groundCount NOTIFY searchModelChanged)
    Q_PROPERTY(int doodadCount READ doodadCount NOTIFY searchModelChanged)
    Q_PROPERTY(int creatureCount READ creatureCount NOTIFY searchModelChanged)

    Q_INVOKABLE void resetFilter();
    Q_INVOKABLE void setFilter(QString brushType);

    int totalCount() const;
    int rawCount() const;
    int groundCount() const;
    int doodadCount() const;
    int creatureCount() const;

    void setPredicate(std::function<bool(Brush *)> predicate);
    void reset();
    void setSourceModel(QAbstractItemModel *model) override;

  protected:
    [[nodiscard]] bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

  private:
    std::function<bool(Brush *)> acceptAll = [](Brush *) { return true; };
    std::function<bool(Brush *)> predicate = acceptAll;

    SearchResultModel *_searchModel = nullptr;
    ItemTypeImageProvider *thingImageProvider = nullptr;

    std::optional<BrushType> parseBrushType(const QString &rawBrushType);
};

class SearchResultModel : public QAbstractListModel
{
  public:
    Q_OBJECT
  public:
    enum class Role
    {
        VectorIndex = Qt::UserRole + 1,
        DisplayId = Qt::UserRole + 2,
        ResourceString = Qt::UserRole + 3,
        Name = Qt::UserRole + 4,
    };

    SearchResultModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void setSearchResults(BrushSearchResult &&brushes);
    const std::optional<BrushSearchResult> &searchResults() const
    {
        return _searchResults;
    }

    void clear();

    Brush *brushAtIndex(size_t index) const;

  protected:
    QHash<int, QByteArray> roleNames() const override;

  private:
    std::optional<BrushSearchResult> _searchResults;

    ItemTypeImageProvider *thingImageProvider = nullptr;
};
