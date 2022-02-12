#include "welcome_view.h"

#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QQmlContext>

#include "../const.h"
#include "../error.h"
#include "../file.h"

WelcomeView::WelcomeView(MainApplication *app, QObject *parent)
    : app(app), QObject(parent)
{
}

void WelcomeView::show()
{
    recentFilesModel.addFile("D:/dev/C++/vulkan-map-editor/vulkan-map-editor/map1.otbm");
    recentFilesModel.addFile("D:/dev/C++/vulkan-map-editor/vulkan-map-editor/some_other_map.otbm");

    view = new QQuickView();
    view->engine()->rootContext()->setContextProperty("_viewModel", this);
    view->engine()->rootContext()->setContextProperty("_recentFilesModel", &this->recentFilesModel);
    view->setSource(QUrl("qrc:/vme/qml/vme/WelcomeView.qml"));
    if (view->status() == QQuickView::Error)
        throw std::exception("Could not load QML welcome view 'qrc:/vme/qml/vme/WelcomeView.qml'");

    QSurfaceFormat format;
    format.setSwapInterval(0);
    format.setRenderableType(QSurfaceFormat::OpenGL);
    view->setFormat(format);

    // view->setResizeMode(QQuickView::SizeRootObjectToView);

    view->show();
}

void WelcomeView::openMainWindow()
{
    // app->showMainWindow();
    readRecentFiles();
}

void WelcomeView::selectMapFile()
{
    auto qFileName = QFileDialog::getOpenFileName(nullptr, tr("Open Image"), "C:/Users/giuin/Desktop", tr("OTBM Files (*.otbm)"));
    std::string mapPath = qFileName.toStdString();

    // No file selected
    if (qFileName.isEmpty())
    {
        return;
    }

    if (File::exists(mapPath))
    {
        try
        {
            app->mainWindow.addMapTab(mapPath);
        }
        catch (const MapLoadError &error)
        {
            QMessageBox messageBox;
            messageBox.setText(error.what());
            messageBox.exec();
            return;
        }

        app->showMainWindow();
    }
    else
    {
        QMessageBox messageBox;
        messageBox.setText("Could not find the file " + qFileName);
        messageBox.exec();
        return;
    }
}

void WelcomeView::close()
{
    view->close();
}

void WelcomeView::readRecentFiles()
{
    if (File::exists(std::filesystem::path(APP_FOLDER_PATH) / RECENT_FILES_FILE))
    {
        VME_LOG_D("We have it");
    }
    else
    {
        VME_LOG_D("We don't have it");
        File::createDirectory(std::filesystem::path(APP_FOLDER_PATH));
    }
}

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
        return QVariant(QString::fromStdString(backing[index.row()].path));
    }
    return QVariant();
}

void RecentFilesModel::onItemClicked(const int i)
{
    if (i < 0 || i >= backing.size())
    {
        return;
    }

    File file = backing[i];
    VME_LOG_D("Clicked " << file.path);
}

void RecentFilesModel::addFile(std::string path)
{
    backing.emplace_back(path);
}

RecentFilesModel::File::File(std::string path)
    : path(path) {}
