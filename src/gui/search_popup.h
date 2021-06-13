#pragma once

#include <QAbstractItemDelegate>
#include <QAbstractListModel>
#include <QObject>
#include <QPixmap>
#include <QQuickView>
#include <QSortFilterProxyModel>
#include <QWidget>

#include <functional>
#include <optional>

#include "../brushes/brush.h"

class MainWindow;
class Brush;
class QHideEvent;
class SearchResultModel;
class ItemTypeImageProvider;

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
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

  private:
    std::function<bool(Brush *)> acceptAll = [](Brush *) { return true; };
    std::function<bool(Brush *)> predicate = acceptAll;

    SearchResultModel *_searchModel = nullptr;
    ItemTypeImageProvider *thingImageProvider;

    std::optional<BrushType> parseBrushType(QString raw);
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

    ItemTypeImageProvider *thingImageProvider;
};

class SearchPopupView : public QQuickView
{
    Q_OBJECT

  signals:
    void requestClose();

  public:
    SearchPopupView(QUrl filepath, MainWindow *mainWindow);

    Q_INVOKABLE void searchEvent(QString searchTerm);
    Q_INVOKABLE void setHeight(int height);
    Q_INVOKABLE void brushSelected(int index);

    QWidget *wrapInWidget(QWidget *parent = nullptr);

    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent();

    void focus();

    void reloadSource();

    void search(std::string searchTerm);

    QUrl _filepath;
    MainWindow *mainWindow;
    QWidget *_wrapperWidget = nullptr;

    ItemTypeImageProvider *thingImageProvider;

  protected:
    void hideEvent(QHideEvent *e) override;

  private:
    SearchResultModel searchResultModel;
    FilteredSearchModel filteredSearchModel;
    QObject *child(const char *name);
};

/**
 * This extra wrapper is used to center the search widget in the window.
 * TODO: Would be nice if we could get rid of this wrapper that is used only to center the widget. 
 */
class SearchPopupWidget : public QWidget
{
  public:
    SearchPopupWidget(MainWindow *mainWindow);

    void keyPressEvent(QKeyEvent *event) override;

    SearchPopupView *popupView() const;

  private:
    SearchPopupView *searchPopupView;
    MainWindow *mainWindow;
};

class SearchWrapperEventFilter : public QObject
{
  public:
    SearchWrapperEventFilter(SearchPopupView *parent);

    bool eventFilter(QObject *obj, QEvent *event) override;

  private:
    SearchPopupView *searchPopupView;
};
