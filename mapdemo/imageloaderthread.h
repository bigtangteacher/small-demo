// imageloaderthread.h
#ifndef IMAGELOADERTHREAD_H
#define IMAGELOADERTHREAD_H

#include <QThread>
#include <QList>
#include <QPair>
#include <QString>
#include <QMutex>
#include <QWaitCondition>
#include "mapread.h"

class ImageLoaderThread : public QThread
{
    Q_OBJECT

public:
    explicit ImageLoaderThread(QObject *parent = nullptr);
    void setCoordinatesFilePath(const QString &path);
    void setZoomLevel(int zoomLevel);
    void setCurrentLevel(int level);
    void setCenterTileXY(int x, int y) { centerTileX = x; centerTileY = y; }
    void setIsZoomIn(bool zoomIn);
    void setIsInterrupted(bool interrupted);
    void requestStop();
     QWaitCondition condition;
protected:
    void run() override;

signals:
    void imagesReady(const QList<QPair<QString, mapread::coordinate>> &images);

private:
    QString coordinatesFilePath;
    int zoomLevel;
    int previousZoomLevel;
    mapread mapReader;
    int centerTileX;
    int centerTileY;
    bool hasBeenCalled;
    bool isZoomIn;
    bool isInterrupted;
    QMutex mutex;
     bool stopRequested;  // 新增成员变量

};

#endif // IMAGELOADERTHREAD_H
