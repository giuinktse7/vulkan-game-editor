#pragma once

#include <QAbstractItemDelegate>
#include <QAbstractListModel>
#include <QObject>
#include <QPixmap>
#include <QQuickView>
#include <QWidget>

class MainWindow;
class Brush;
class QHideEvent;

class SearchResultModel : public QAbstractListModel
{
    Q_OBJECT
  public:
    enum class Role
    {
        ServerId = Qt::UserRole + 1,
    };

    SearchResultModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    void setSearchResults(std::unique_ptr<std::vector<Brush *>> &&brushes);

    void clear();

    Brush *brushAtIndex(size_t index) const;

  protected:
    QHash<int, QByteArray> roleNames() const override;

  private:
    std::unique_ptr<std::vector<Brush *>> _searchResults;
};

class SearchPopupView : public QQuickView
{
    Q_OBJECT

  signals:
    void requestClose();

  public:
    SearchPopupView(QUrl filepath, MainWindow *mainWindow);

    Q_INVOKABLE void searchEvent(QString searchTerm);

    QWidget *wrapInWidget(QWidget *parent = nullptr);

    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent();

    void focus();

    void reloadSource();

    void search(std::string searchTerm);

    QUrl _filepath;
    MainWindow *mainWindow;
    QWidget *_wrapperWidget = nullptr;

  protected:
    void hideEvent(QHideEvent *e) override;

  private:
    SearchResultModel searchResultModel;

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