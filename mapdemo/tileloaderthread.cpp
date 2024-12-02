#include "tileloaderthread.h"
#include <QImage>
#include <QDebug>

TileLoaderThread::TileLoaderThread(QObject *parent)
    : QThread(parent), x(0), y(0)
{
}


void TileLoaderThread::setImagePath(const QString &path, int _x, int _y)
{
    imagePath = path;
    x = _x;
    y = _y;
}

void TileLoaderThread::run()
{
    QImage image(imagePath);
    if (!image.isNull()) {
        emit tileLoaded(image, x, y);
    } else {
        qDebug() << "Failed to load image: " << imagePath;
    }
}
