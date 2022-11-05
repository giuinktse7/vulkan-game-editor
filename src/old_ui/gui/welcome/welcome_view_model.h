#pragma once

#include <QAbstractListModel>
#include <QObject>

#include <chrono>
#include <ctime>
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
        return files.size();
    }
    virtual QVariant data(const QModelIndex &index, int role) const;

    QHash<int, QByteArray> roleNames() const;

    Q_INVOKABLE void onItemClicked(const int index);

    void addFile(std::string path);
    void writeToDisk();
    void readFromDisk();

    struct RecentFile
    {
        RecentFile(std::string path);
        RecentFile(std::string path, std::chrono::system_clock::time_point timestamp);
        std::string path;
        std::chrono::system_clock::time_point timestamp;
    };

  private:
    std::vector<RecentFile> files;
};

class WelcomeViewModel
{
  public:
    RecentFilesModel recentFiles;
};