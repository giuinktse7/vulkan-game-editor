#include "welcome_view.h"

#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>
#include <QQmlContext>

#include "../../const.h"
#include "../../error.h"
#include "../../file.h"

WelcomeView::WelcomeView(QObject *parent)
    : QObject(parent)
{
}

WelcomeView::WelcomeView(MainApplication *app, QObject *parent)
    : app(app), QObject(parent)
{
}

void WelcomeView::show()
{
    viewModel.recentFiles.addFile("C:/dev/C++/vulkan-map-editor/vulkan-map-editor/map1.otbm");
    viewModel.recentFiles.addFile("C:/dev/C++/vulkan-map-editor/vulkan-map-editor/some_other_map.otbm");

    view = new QQuickView();
    view->engine()->rootContext()->setContextProperty("_viewModel", this);
    view->engine()->rootContext()->setContextProperty("_recentFilesModel", &this->viewModel.recentFiles);
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
    // TODO
    viewModel.recentFiles.writeToDisk();
    return;
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
    // TODO
    if (File::exists(std::filesystem::path(APP_FOLDER_PATH) / RECENT_FILES_FILE))
    {
    }
    else
    {
        File::createDirectory(std::filesystem::path(APP_FOLDER_PATH));
    }
}
