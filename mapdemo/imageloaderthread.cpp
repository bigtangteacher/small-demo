// imageloaderthread.cpp
#include "imageloaderthread.h"
#include "mapread.h"
ImageLoaderThread::ImageLoaderThread(QObject *parent)
    : QThread(parent), zoomLevel(18),isInterrupted(false), stopRequested(false)
{
}

void ImageLoaderThread::setCoordinatesFilePath(const QString &path)
{
    coordinatesFilePath = path;
}

void ImageLoaderThread::setZoomLevel(int level)
{
    previousZoomLevel = zoomLevel;
    zoomLevel = qBound(1, level, 18);
}

void ImageLoaderThread::setIsZoomIn(bool zoomIn)
{
    isZoomIn = zoomIn;
}

void ImageLoaderThread::setIsInterrupted(bool interrupted)
{
    this->isInterrupted = interrupted;
}


void ImageLoaderThread::requestStop()
{
    QMutexLocker locker(&mutex);
    stopRequested = true;
    isInterrupted = true;
    condition.wakeAll();
}

void ImageLoaderThread::run()
{

    int adjustedCenterTileX;
    int adjustedCenterTileY;
    QMutexLocker locker(&mutex);
    try {
            if (coordinatesFilePath.isEmpty()) {
                qCritical() << "Coordinates file path is empty.";
                return;
            }

            qDebug() << "Reading coordinates from file: " << coordinatesFilePath << " with zoom level: " << zoomLevel;
            mapReader.readCoordinatesFromFile(coordinatesFilePath, zoomLevel);

            if (centerTileX != 0 && centerTileY != 0) {
                // 有中心瓦片坐标，调用有参的 getMatchedImages
                QList<QPair<QString, mapread::coordinate>> images;
                if (!hasBeenCalled) {
                    // 第一次调用无参的 getMatchedImages
                    images = mapReader.getMatchedImages();
                    hasBeenCalled = true;
                } else {
                    // 之后的调用有参的 getMatchedImages
                        adjustedCenterTileX = isZoomIn ? (centerTileX) : (centerTileX );
                        adjustedCenterTileY = isZoomIn ? (centerTileY) : (centerTileY );
                    while(adjustedCenterTileX > 400000)
                        adjustedCenterTileX /= 2;
                    while(adjustedCenterTileY >= 150000)
                        adjustedCenterTileY /= 2;

                    qDebug() << adjustedCenterTileX << "adjust:   " << adjustedCenterTileY << "zoomlevel:  " << zoomLevel;
                    // 检查是否被中断
                    if (isInterrupted) {
                        // 返回缓存中的图像
                        if (mapReader.imageCache.contains(qMakePair(centerTileX, centerTileY))) {
                            images = mapReader.imageCache[qMakePair(centerTileX, centerTileY)];
                        } else {
                            images = mapReader.getMatchedImages(adjustedCenterTileX, adjustedCenterTileY);
                        }
                        isInterrupted = false; // 重置中断标志
                    } else {
                        images = mapReader.getMatchedImages(adjustedCenterTileX, adjustedCenterTileY);
                    }
                }

                emit imagesReady(images);

            } else {
                // 没有中心瓦片坐标，调用无参的 getMatchedImages
                QList<QPair<QString, mapread::coordinate>> images = mapReader.getMatchedImages();
                emit imagesReady(images);
            }
        } catch (const std::exception &e) {
            qCritical() << "Exception in ImageLoaderThread::run: " << e.what();
        } catch (...) {
            qCritical() << "Unknown exception in ImageLoaderThread::run";
        }
}

