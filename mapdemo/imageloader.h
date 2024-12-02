#ifndef IMAGELOADER_H
#define IMAGELOADER_H

#include <QRunnable>
#include <QList>
#include <QPair>
#include <QString>
#include "mapread.h"
#include "mainwindow.h"

class ImageLoader : public QRunnable
{
public:
    ImageLoader(MainWindow *mainWindow, mapread *reader, const QString &directoryPath, QList<QPair<QString, mapread::coordinate>> *result)
        : mainWindow(mainWindow), reader(reader), directoryPath(directoryPath), result(result) {}

    void run() override;

private:
    MainWindow *mainWindow;
    mapread *reader;
    QString directoryPath;
    QList<QPair<QString, mapread::coordinate>> *result;
};

#endif // IMAGELOADER_H
