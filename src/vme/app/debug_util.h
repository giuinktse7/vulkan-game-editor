#pragma once

#include <QDebug>

void pipeQtMessagesToStd(QtMsgType type, const QMessageLogContext &context, const QString &msg);

void showQmlResources();