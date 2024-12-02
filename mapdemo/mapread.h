#ifndef MAPREAD_H
#define MAPREAD_H

#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QMap>
#include <QQueue>
#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QtConcurrent/QtConcurrentRun>
#include <QTimer>
#include <QtMath>
#include <QElapsedTimer>
#include <QImage>
#include <QPainter>
#include <QMetaType>
#include <QHash>

class mapread : public QObject
{
    Q_OBJECT  // 添加 Q_OBJECT 宏

public:
    struct coordinate
    {
        int curLongitude;
        int curLatitude;

        coordinate() : curLongitude(0), curLatitude(0) {}
        coordinate(int lon, int lat) : curLongitude(lon), curLatitude(lat) {}
    };


    QHash<QPair<int, int>, QList<QPair<QString, coordinate>>> imageCache; // 新增的缓存
    QList<QPair<QString,coordinate>> getAdjacentImages(const coordinate &coord,int zoomLevel);
    QList<QPair<QString, coordinate>> getMatchedImages();
    QList<QPair<QString,coordinate>> getMatchedImages(int centerTileX,int centerTileY);
    int getZoomLevel();
    void setZoomLevel(int level);  // 声明 setZoomLevel 方法
    mapread();
    void readDirectory(const QString &directoryPath);                                           //递归遍历文件，将匹配的瓦片文件放到缓存中，
    void readCoordinatesFromFile(const QString &coordinatesFilePath,int zoomLevel);             //读取经纬度文件，并且根据这些坐标计算出需要的瓦片坐标。
    QMap<QString, coordinate> getFilesWithCoordinates() const;

    QImage getHigherResolutionImage(const coordinate &coord, int higherZoomLevel);


    QList<mapread::coordinate> getSurroundingCoordinates(int tileX,int tileY,int zoomLevel);
    QList<QImage> loadImages(const QList<QPair<double, double>>& coordinates);
    QList<coordinate> requiredCoordinates;
signals:
    void updateImages(const QList<QPair<QString,mapread::coordinate>>& images);
    void zoomIn();
    void zoomOut();

private:
    QMap<QString, coordinate> filesWithCoordinates;


    QList<QPair<QString, coordinate>> matchedImages;
    void loadImages(const QString &directoryPath, const QList<QPair<QString, coordinate>> &matchedImages);
//    QList<QImage> loadedImages;                     //新增成员变量存储加载的图片
    QElapsedTimer timer;
    QSet<QString> addedPaths;                        // 保证不重复添加
    coordinate parseCoordinatesFromFileName(const QString &fileName, const QString &parentDir);
    void refreshFilesWithCoordinates(int centerTileX,int centerTileY);
    coordinate convertToLevel(const coordinate& coord);
    bool isCoordinateRequired(const coordinate& coord);                                          //判断是否需要删除这个瓦片文件，他是否被需要。
    int lonToTileX(double lon,int z);
    int latToTileY(double lat,int z);
    void findNearByImages(int tileX, int tileY, QList<QPair<QString, coordinate>> &matchedImages);
    QList<mapread::coordinate> interpolateCoordinates(const QList<QPair<double,double>> &coordinates, int numPointsPerSegment, int zoomLevel);          //对坐标组进行插值生成更多坐标
    QString mapDirectory;

    QList<QPair<QString, coordinate>> loadedImages;
    int zoomLevel;
};

#endif // MAPREAD_H
