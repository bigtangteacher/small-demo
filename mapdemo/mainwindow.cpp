#include "mainwindow.h"
#include <QRandomGenerator>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPainter>
#include <QPixmap>
#include <QRect>
#include <cmath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      currentIndex(0),
      currenZoomLevel(13),
      imageLoaderThread(new ImageLoaderThread(this)),
      currentGroupIndex(0),
      currentCoordinateIndex(0),
      timer(nullptr),
      wheelCounter(0),
      zoomLabel(nullptr)  // 初始化 zoomLabel
      ,
      previousZoomLevel(0),
      lastCenterTiles(4)                //控制大小
{
    this->resize(512, 512);
   this->setFixedSize(1024, 1024);  // 固定窗口大小

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // 使用 QVBoxLayout 布局
    layout = new QVBoxLayout(centralWidget);

    // 创建一个 QLabel 用于显示合成后的图片
    imageLabel = new QLabel;
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    imageLabel->setScaledContents(true);  // 确保图片始终缩放到 label 的大小
    layout->addWidget(imageLabel);

    // 创建标签用于显示文件信息
    infoLabel = new QLabel;
    infoLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(infoLabel);

    // 设置坐标文件路径
    QString coordinatesFilePath = "D:/mapdata/coordinates.txt";

    centerTiles.resize(4);

    imageLoaderThread->setCoordinatesFilePath(coordinatesFilePath);
    imageLoaderThread->setZoomLevel(currenZoomLevel);
    connect(imageLoaderThread, &ImageLoaderThread::imagesReady, this, &MainWindow::displayImages);
    imageLoaderThread->start();


    // 创建一个 QLabel 用于显示放大的像素区域
    zoomLabel = new QLabel(this);
    zoomLabel->setAlignment(Qt::AlignCenter);
    zoomLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    zoomLabel->setFixedSize(50, 50);  // 设置固定大小
    zoomLabel->move(10, 10);  // 移动到左上角
    zoomLabel->hide();  // 默认隐藏



    //安装事件过滤器
    installEventFilter(this);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Wheel) {
        QWheelEvent *wheelEvent = static_cast<QWheelEvent*>(event);

        if (wheelEvent->angleDelta().y() > 0) {
            // 向上滚动，增加计数器
            isZoomIn = true;
            wheelCounter++;
        } else {
            // 向下滚动，减少计数器
            isZoomIn = false;
            wheelCounter--;
        }

        // 每滑动 3 次更新缩放级别
        const int threshold = 3;
        if (qAbs(wheelCounter) >= threshold) {
            if (wheelCounter > 0 ) {
                // 向上滚动，放大
                currenZoomLevel++;

            } else {
                // 向下滚动，缩小
                currenZoomLevel--;
            }

            // 重置滚轮计数器
            wheelCounter = 0;

            // 重新计算中心位置的瓦片坐标
            QVector<QPair<int, int>> newTiles;

            for (const auto& tile : centerTiles) {
                 qDebug() << "adjustedtile is new:" << centerTiles;
                QPair<int, int> adjustedTile = tile;
                qDebug() << "adjustedtile is:" << adjustedTile;
                if (isZoomIn) {
                    adjustedTile.first *= 2;
                    adjustedTile.second *= 2;
                } else {
                    adjustedTile.first /= 2;
                    adjustedTile.second /= 2;
                }
                if (adjustedTile.first > 400000 && currenZoomLevel >= 18)
                    adjustedTile.first /= 2;
                if (adjustedTile.second > 150000 && currenZoomLevel >= 18)
                    adjustedTile.second /= 2;
                newTiles.append(adjustedTile);
            }
            qDebug() << "current new tiles" << newTiles.size();
            qDebug() << "newTiles zuobiao"<<newTiles;
            // 更新中心瓦片坐标
            updateCenterTiles(newTiles);

            // 使用 QTimer::singleShot 延迟执行 handleZoom 方法
            QTimer::singleShot(100, this, [this, newTiles] {
                // 在这里检查线程是否已经在运行，如果在运行，则先退出
                if (imageLoaderThread->isRunning()) {
                    imageLoaderThread->quit();
                    imageLoaderThread->wait();
                }
                for(int i = 0 ; i < 4; i ++)
                {
                    const auto& tile = newTiles[i];
                    qDebug() << "Before adjust:" << tile;
                    startImageLoading(tile.first + 1,tile.second + 1);
                }
            });
        } else {
            // 处理局部放大或缩小
            if (isZoomIn) {
                imageLabel->setPixmap(imageLabel->pixmap()->scaled(imageLabel->pixmap()->size() * 1.2, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            } else {
                imageLabel->setPixmap(imageLabel->pixmap()->scaled(imageLabel->pixmap()->size() / 1.2, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
        }

        return true;  // 表示事件已被处理
    }

    return QMainWindow::eventFilter(obj, event);
}

MainWindow::~MainWindow()
{
    if (imageLoaderThread) {
        imageLoaderThread->quit();
        imageLoaderThread->wait();
        delete imageLoaderThread;
    }
    if (timer) {
        timer->stop();
        delete timer;
    }
    if (zoomLabel) {
        delete zoomLabel;
    }
}

void MainWindow::displayImages(const QList<QPair<QString, mapread::coordinate>>& images)
{
    this->images = images;

    qDebug() << "size" << this->images.size();
    if (images.isEmpty()) {
        imageLabel->setText("No images found.");
        infoLabel->setText("No coordinates available.");
        return;
    }

    // 选择中心点
    bool allTilesValid = true;
    for(const auto& tile : lastCenterTiles)
    {
        if(tile.first == 0 && tile.second == 0)
        {
            allTilesValid = false;
            break;
        }
    }


    if (allTilesValid) {
        qDebug() << "lastCenterTiles" << lastCenterTiles;
        qDebug() << "CenterTiles" << centerTiles;
        // 使用上次的中心点
        for (const auto& tile : lastCenterTiles) {
            QPair<int, int> adjustedTile = tile;
            if (isZoomIn) {
                adjustedTile.first *= 2;
                adjustedTile.second *= 2;
            } else {
                adjustedTile.first /= 2;
                adjustedTile.second /= 2;
            }
            if (adjustedTile.first > 400000 && currenZoomLevel >= 18)
                adjustedTile.first /= 2;
            if (adjustedTile.second > 150000 && currenZoomLevel >= 18)
                adjustedTile.second /= 2;


            centerTiles.append(adjustedTile);
        }
    } else {
        bool isNonZeroInCenterTiles = false;
        for (QVector<QPair<int ,int >>::const_iterator it = centerTiles.begin(); it != centerTiles.end() ; it++) {
            if(it->first == 0 && it->second == 0)
            {
                isNonZeroInCenterTiles = true;
                break;
            }

        }
        //暂时随机生成图片，若是没有图片
        for(int i = 0 ;  i < centerTiles.size() ; i ++){
            int randomIndex = QRandomGenerator::global()->bounded(images.size());
            const auto& startPoint = images[randomIndex];
            QPair<int, int> newTile = qMakePair(startPoint.second.curLongitude + 1, startPoint.second.curLatitude + 1);  // 给横纵坐标都加1

            //确保新的瓦片不在原本centertile中
            if (!centerTiles.contains(newTile)) {
                centerTiles[i] = newTile;
            }
        }
    }
    // 获取起点图片的尺寸
    QImage firstImage(images.first().first);
    int width = firstImage.width();
    int height = firstImage.height();

    // 创建一个大的 QImage 来容纳4x4的小图片
    QImage compositeImage(width * 4, height * 4, QImage::Format_ARGB32);
    compositeImage.fill(Qt::transparent);

    QPainter painter(&compositeImage);

    // 定义一个辅助函数来查找并绘制图片
    bool hasCenterImages[4] = {false, false, false, false};  // 用于跟踪每个中心瓦片是否找到对应的图片
    auto drawImageAtPosition = [&](int dx, int dy, const QPair<int, int>& centerTile, int index) -> bool {
        // 遍历 images 列表寻找匹配项
        for (const auto& image : images) {
            if (image.second.curLongitude == centerTile.first + dx - 2 &&
                image.second.curLatitude == centerTile.second + dy - 2) {
                // 如果找到了匹配的图片
                QImage img(image.first);
                painter.drawImage(dx * width, dy * height, img);
                painter.drawText(dx * width + 5, dy * height + 15, QString("Coordinates: (%1, %2)").arg(image.second.curLongitude).arg(image.second.curLatitude));
                if (dx == 2 && dy == 2) {
                    hasCenterImages[index] = true;  // 更新 hasCenterImages
                }
                return true;  // 返回成功
            }

        }

        // 如果没有找到匹配的图片，则打印出缺少图片的瓦片坐标
        int missingTileX = centerTile.first + dx - 2;
        int missingTileY = centerTile.second + dy - 2;

        //查找需要填充的图片所在路径
        QString imagePath = QString("D:/mapdata/mapabc-沈阳/satellite/%1/%2/%3").arg(this->currenZoomLevel).arg(missingTileX).arg(missingTileY);
        //事先查询文件是否存在
        QImage img(imagePath);

        if (!img.isNull()) {
               // 图片存在，使用它
               qDebug() << "绘图" << imagePath;
               painter.drawImage(dx * width, dy * height, img);
               painter.drawText(dx * width + 5, dy * height + 15, QString("Coordinates: (%1, %2)").arg(missingTileX).arg(missingTileY));
               return true;  // 返回成功
           } else {
               // 图片不存在，打印出缺失图片的瓦片坐标
               qDebug() << "Missing image at tile coordinates:" << missingTileX << missingTileY;
                qDebug() << imagePath;
               // 绘制红色图片
               QImage redImage(width, height, QImage::Format_RGB32);
               redImage.fill(Qt::red);  // 使用红色填充图片
               painter.drawImage(dx * width, dy * height, redImage);
               painter.drawText(dx * width + 5, dy * height + 15, QString("Coordinates: (%1, %2)").arg(missingTileX).arg(missingTileY));

               return false;  // 没有找到匹配的图片，返回失败
           }
    };

    // 绘制4x4的图片
    for (int i = 0; i < 4; ++i) {
        for (int dx = 0; dx < 4; ++dx) {
            for (int dy = 0; dy < 4; ++dy) {
                drawImageAtPosition(dx, dy, centerTiles[i], i);
            }
        }
    }

    painter.end();

    // 显示合成后的图片
    QPixmap pixmap = QPixmap::fromImage(compositeImage);
    imageLabel->setPixmap(pixmap);




    // 更新 lastCenterTile
    lastCenterTiles = centerTiles;
}


void MainWindow::startImageLoading(int centerTileX, int centerTileY)
{
    qDebug() << "startImageLoading called with zoom level: " << currenZoomLevel;
    qDebug() << "currentZoomlevel" << currenZoomLevel << "previousZoomlevel" << previousZoomLevel;

    QString coordinatesFilePath = "D:/mapdata/coordinates.txt";
    imageLoaderThread->setCoordinatesFilePath(coordinatesFilePath);
    imageLoaderThread->setIsZoomIn(!isZoomIn);  // 设置 isZoomIn 标志

    imageLoaderThread->setZoomLevel(currenZoomLevel);

    // 更新 previousZoomLevel
    previousZoomLevel = currenZoomLevel;

    // 设置中心瓦片坐标，并加1
    imageLoaderThread->setCenterTileXY(centerTileX + 1, centerTileY + 1);
    imageLoaderThread->start();
}
