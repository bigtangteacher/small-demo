#ifndef MAPSPLICE_H
#define MAPSPLICE_H

#include <QObject>
#include <QImage>
#include <QList>
#include <QPair>
#include "mapread.h" // 包含 MapRead 的完整定义

class MapSplice : public QObject
{
    Q_OBJECT  // 添加 Q_OBJECT 宏

public:
    explicit MapSplice(QObject *parent = nullptr);
    QImage spliceImages(const QList<QPair<double, double>>& coordinates);

private:
    mapread *mapRead;
};

#endif // MAPSPLICE_H
