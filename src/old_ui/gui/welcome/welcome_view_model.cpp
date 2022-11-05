#include "welcome_view_model.h"

#include "date/date.h"
#include <format>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <sstream>

#include "../../const.h"
#include "../../file.h"
#include "../../logger.h"
#include "../../time_util.h"

using JSON = nlohmann::json;

RecentFilesModel::RecentFilesModel(QObject *parent)
    : QAbstractListModel(parent) {}

QHash<int, QByteArray> RecentFilesModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[PathRole] = "path";
    return roles;
}

QVariant RecentFilesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
    {
        return QVariant();
    }
    if (role == PathRole)
    {
        return QVariant(QString::fromStdString(files[index.row()].path));
    }
    return QVariant();
}

void RecentFilesModel::onItemClicked(const int i)
{
    if (i < 0 || i >= files.size())
    {
        return;
    }

    RecentFile file = files[i];
    VME_LOG_D("Clicked " << file.path);
}

void RecentFilesModel::addFile(std::string path)
{
    files.emplace_back(path);
}

RecentFilesModel::RecentFile::RecentFile(std::string path)
    : RecentFile(path, std::chrono::system_clock::now()) {}

RecentFilesModel::RecentFile::RecentFile(std::string path, std::chrono::system_clock::time_point timestamp)
    : path(path), timestamp(timestamp) {}

void RecentFilesModel::writeToDisk()
{
    JSON json;
    json["recentFiles"] = JSON::array();
    auto &recentFiles = json["recentFiles"];

    for (const RecentFile &file : files)
    {
        JSON entry;
        entry["path"] = file.path;
        entry["timestamp"] = TimePoint::toString(file.timestamp);

        recentFiles.emplace_back(std::move(entry));
    }

    File::writeJson(std::filesystem::path(APP_FOLDER_PATH) / RECENT_FILES_FILE, std::move(json));

    // std::ofstream o("pretty.json");
    // o << std::setw(4) << json << std::endl;
}

void RecentFilesModel::readFromDisk()
{
}
