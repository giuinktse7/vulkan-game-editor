#include "debug_util.h"

#include <QDirIterator>
#include <iostream>

void pipeQtMessagesToStd(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    const char *file = context.file ? context.file : "";
    const char *function = context.function ? context.function : "";

    switch (type)
    {
        case QtDebugMsg:
            fprintf(stdout, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
            break;
        case QtInfoMsg:
            fprintf(stdout, "Info: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
            break;
        case QtWarningMsg:
            fprintf(stdout, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
            break;
        case QtCriticalMsg:
            fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
            break;
        case QtFatalMsg:
            fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), file, context.line, function);
            break;
    }
}

void showQmlResources()
{
    QDirIterator it(":", QDirIterator::Subdirectories);
    while (it.hasNext())
    {
        qDebug() << it.next();
    }
}
