#pragma once

#include "main_application.h"
#include <QAbstractListModel>
#include <QObject>
#include <QQuickView>
#include <QVariant>

#include <string>

class RecentFilesModel : public QAbstractListModel
{
    Q_OBJECT
  public:
    enum Roles
    {
        PathRole = Qt::UserRole + 1,
    };

    explicit RecentFilesModel(QObject *parent = 0);

    virtual int rowCount(const QModelIndex &) const
    {
        return backing.size();
    }
    virtual QVariant data(const QModelIndex &index, int role) const;

    QHash<int, QByteArray> roleNames() const;

    Q_INVOKABLE void onItemClicked(const int index);

    void addFile(std::string path);

    struct File
    {
        File(std::string path);
        std::string path;
    };

  private:
    std::vector<File> backing;
};

class WelcomeView : public QObject
{
    Q_OBJECT
  public:
    WelcomeView(QObject *parent = nullptr);
    WelcomeView(MainApplication *app, QObject *parent = nullptr);

    Q_INVOKABLE void openMainWindow();
    Q_INVOKABLE void selectMapFile();

    void show();
    void close();

  private:
    void readRecentFiles();

    MainApplication *app;
    QQuickView *view;

    RecentFilesModel recentFilesModel;
};