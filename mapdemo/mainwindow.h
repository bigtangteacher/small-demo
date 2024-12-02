#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QVBoxLayout>
#include <QPixmap>
#include <QFileInfo>
#include <QTimer>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QList>
#include <QPair>
#include <QImage>
#include <QPainter>
#include <QThread>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QtMath>
#include <QRectF>
#include <QHash>
#include "mapread.h"
#include "imageloaderthread.h"




class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void displayImages(const QList<QPair<QString, mapread::coordinate>>& images);
//    void nextImage();
    void startImageLoading(int centerTileX,int centerTileY);
//    void handleMouseClick();
    void updateCenterTiles(const QVector<QPair<int, int>>& tiles)
    {
        // 仅使用 tiles 中的前4个元素来更新 centerTiles
        if (tiles.size() >= 4) {
            centerTiles = tiles.mid(0, 4);
        } else {
            // 如果 tiles 少于4个元素，则直接使用所有元素
            centerTiles = tiles;
        }

        // 更新 lastCenterTiles
        lastCenterTiles = centerTiles;

        // 输出更新后的 centerTiles 和 lastCenterTiles
        qDebug() << "Updated centerTiles and lastCenterTile to: " << centerTiles;

        // 直接更新 centerLatLng 为瓦片坐标
        centerLatLng.clear();  // 清空旧的中心经纬度
        for (int i = 0; i < centerTiles.size(); ++i) {
            centerLatLng.append(qMakePair(static_cast<double>(centerTiles[i].first), static_cast<double>(centerTiles[i].second)));
        }
    }
protected:
//    void wheelEvent(QWheelEvent *event) override;

private:
    void zoomOutAt(const QPoint &point);
    void zoomInAt(const QPoint &point);
    void reloadTilesAt(const QPoint &point);
    void displayImagesAround(int centerTileX, int centerTileY, int zoomLevel);
    bool eventFilter(QObject *obj, QEvent *event);

    QVector<QPair<int, int>> newTiles;
    QLabel *imageLabel;
    QLabel *infoLabel;
    QVBoxLayout *layout;
    QList<QPair<QString, mapread::coordinate>> images;
    int currentIndex;
    int currenZoomLevel;
    ImageLoaderThread *imageLoaderThread;
    int currentGroupIndex;
    int currentCoordinateIndex;
    QTimer *timer;
    mapread mapReader;
    int wheelCounter;
    QGraphicsView *imageView;  // 新增 QGraphicsView
    QGraphicsScene *scene;     // 新增 QGraphicsScene
    int initialZoomLevel;
    QLabel *zoomLabel;  // 新增的成员变量
    QPoint lastMousePos;
    mapread::coordinate highResCoordinate;
    QString highResImage;
    QVector<QPair<double, double>> centerLatLng;  // 声明 centerLatLng 变量
    QPixmap m_currentPixmap;
    QPair<int, int> lastCenterTile; // 保存上次的中心点
    QVector<QPair<int ,int >> lastCenterTiles;

    QPair<int, int> centerTile;
    QVector<QPair<int,int>> centerTiles;


    bool isZoomIn;
    QPair<int, int> previousCenterTile;  // 保存上一级的中心点


    bool isZoomPending;
    int previousZoomLevel;
};

#endif // MAINWINDOW_H
