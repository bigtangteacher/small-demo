#include "mapread.h"

mapread::mapread()    : QObject()
{
    requiredCoordinates.clear();
    filesWithCoordinates.clear();
    loadedImages.clear();
    zoomLevel = 18;
}

void mapread::readDirectory(const QString &directoryPath)
{
    QSet<QString> requiredSubDirs;

    // 根据 requiredCoordinates 生成所需的子文件夹集合
    for (const auto &coord : requiredCoordinates) {
        QString subDir = QString::number(coord.curLongitude);
        requiredSubDirs.insert(subDir);
    }

    QQueue<QString> directories;
    directories.enqueue(directoryPath);

    while (!directories.isEmpty())
    {
        QString currentDir = directories.dequeue();
        QDir directory(currentDir);

        // 检查目录是否存在
        if (directory.exists())
        {
            // 获取文件夹中的所有文件和子文件夹
            QStringList entries = directory.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);

            // 遍历所有条目
            foreach (const QString &entry, entries)
            {
                QString fullPath = directory.filePath(entry);
                QFileInfo fileInfo(fullPath);

                // 如果是文件，检查是否在所需的瓦片文件范围内
                if (fileInfo.isFile())
                {
                    QString fileName = fileInfo.fileName();
                    QString parentDir = fileInfo.absoluteDir().dirName();
                    coordinate coord = parseCoordinatesFromFileName(fileName, parentDir);

                    // 检查是否在所需的瓦片文件范围内
                    if (isCoordinateRequired(coord)) {
                        filesWithCoordinates[fileInfo.absoluteFilePath()] = coord;
                    }
                }
                // 如果是子文件夹，且在所需的子文件夹集合中，加入队列
                else if (fileInfo.isDir() && requiredSubDirs.contains(entry))
                {
                    directories.enqueue(fullPath);
                }
            }
        } else {
            qDebug() << "Directory does not exist: " << currentDir;
        }
    }
}


void mapread::readCoordinatesFromFile(const QString &coordinatesFilePath, int currentZoomLevel)
{
    this->zoomLevel = currentZoomLevel;

    QFile file(coordinatesFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open coordinates file: " << coordinatesFilePath;
        return;
    }

    QList<QPair<double, double>> coordinates;
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        qDebug() << "this line:" << line;
        QStringList parts = line.split('\t');
        if (parts.size() == 2) {
            bool ok;
            double longitude = parts[0].trimmed().toDouble(&ok);
            if (!ok) {
                qDebug() << "Failed to parse longitude: " << parts[0];
                continue;
            }

            double latitude = parts[1].trimmed().toDouble(&ok);
            if (!ok) {
                qDebug() << "Failed to parse latitude: " << parts[1];
                continue;
            }

            coordinates.append(qMakePair(longitude, latitude));
        }
    }

    file.close();

    if (!coordinates.isEmpty()) {
        setZoomLevel(zoomLevel);

        // 直接计算传入的经纬度对应的瓦片坐标
        requiredCoordinates.clear();
        // 为每个坐标计算周围16x16个瓦片坐标
        for (const auto &coordPair : coordinates) {
            double longitude = coordPair.first;
            double latitude = coordPair.second;

            int tileX = lonToTileX(longitude, zoomLevel);
            int tileY = latToTileY(latitude, zoomLevel);

            // 计算周围的16x16个坐标
            for (int dx = -2; dx <= 2; ++dx) {
                for (int dy = -2; dy <= 2; ++dy) {
                    int currentTileX = tileX + dx;
                    int currentTileY = tileY + dy;
                    requiredCoordinates.append({currentTileX, currentTileY});
                }
            }
        }

        // 读取文件和文件目录
        QString directoryPath = QString("D:/mapdata/mapabc-沈阳/satellite/%1").arg(currentZoomLevel);
        timer.start();
        readDirectory(directoryPath);

        qDebug() << "time consume:" << timer.elapsed();
        this->matchedImages.clear();
        for (const auto &tileCoord : requiredCoordinates) {
            int tileX = tileCoord.curLongitude;
            int tileY = tileCoord.curLatitude;
            findNearByImages(tileX, tileY, this->matchedImages);
        }

        if (!matchedImages.isEmpty()) {
            emit updateImages(this->matchedImages);
        }
    }
}


void mapread::findNearByImages(int tileX, int tileY, QList<QPair<QString, coordinate>> &matchedImages)
{
    int range = 1;

    for (int dx = -range; dx <= range; ++dx) {
        for (int dy = -range; dy <= range; ++dy) {
            int currentTileX = tileX + dx;
            int currentTileY = tileY + dy;

            for (auto it = filesWithCoordinates.begin(); it != filesWithCoordinates.end(); ++it) {
                int fileTileX = it.value().curLongitude;
                int fileTileY = it.value().curLatitude;

                // 检查是否在附近的范围内
                if (fileTileX == currentTileX && fileTileY == currentTileY) {
                    QString imagePath = it.key();
                    qDebug() << "found matching image:" << imagePath << "with" << fileTileX << fileTileY;
                    matchedImages.append(qMakePair(imagePath, it.value()));
                }
            }
        }
    }
}


mapread::coordinate mapread::parseCoordinatesFromFileName(const QString &fileName, const QString &parentDir)
{
    // 提取父文件夹名作为经度瓦片编号
    QFileInfo dirInfo(parentDir);
    int tileX = dirInfo.baseName().toInt();

    // 提取文件名作为纬度瓦片编号
    int tileY = fileName.split('.').first().toInt();

    return coordinate{tileX, tileY};
}


bool mapread::isCoordinateRequired(const coordinate &coord)
{
    for (const auto &reqCoord : requiredCoordinates) {
        if (qAbs(coord.curLongitude - reqCoord.curLongitude) <= 1 && qAbs(coord.curLatitude - reqCoord.curLatitude) <= 1) {
            return true;
        }
    }
    return false;
}

int mapread::lonToTileX(double lon, int z)
{
    return static_cast<int>(floor((lon + 180.0) / 360.0 * pow(2.0, z)));
}

int mapread::latToTileY(double lat, int z)
{
    double n = M_PI * (1 - (log(tan(M_PI / 4 + lat * (M_PI / 180.0) / 2)) / M_PI));
    return static_cast<int>(floor(n * pow(2.0, z - 1) / M_PI - 1));
}

void mapread::setZoomLevel(int level)
{
    this->zoomLevel = level;
    addedPaths.clear();
    matchedImages.clear();
}

int mapread::getZoomLevel()
{
    return zoomLevel;
}

QList<QPair<QString, mapread::coordinate>> mapread::getMatchedImages()
{

    for (const auto &tileCoord : requiredCoordinates) {
        int tileX = tileCoord.curLongitude;
        int tileY = tileCoord.curLatitude;
        findNearByImages(tileX, tileY, this->matchedImages);
    }
    return this->matchedImages;
}


QList<QPair<QString, mapread::coordinate>> mapread::getMatchedImages(int centerTileX, int centerTileY)
{
    while (centerTileX > 400000)
        centerTileX /= 2;
    while (centerTileY > 150000)
        centerTileY /= 2;
    qDebug() << "mapread类坐标" << centerTileX << ',' << centerTileY;

    QList<QPair<QString, coordinate>> matchedImages;

    bool foundAnyMatch = false;

    // 检查 filesWithCoordinates 中是否有与给定的 centerTileX 和 centerTileY 一致的文件
    for (auto it = filesWithCoordinates.begin(); it != filesWithCoordinates.end(); ++it) {
        const mapread::coordinate &coord = it.value();

        if (coord.curLongitude == centerTileX && coord.curLatitude == centerTileY) {
            // 检查文件是否存在
            QFile file(it.key());
            if (file.exists()) {
                qDebug() << ",,,,,";
                matchedImages.append(qMakePair(it.key(), coord));
                foundAnyMatch = true;
                break;
            }
        }
    }

    // 如果没有找到任何匹配的文件，重新读取目录
    if (!foundAnyMatch) {
        qDebug() << "未找到匹配文件，重新读取目录：" << centerTileX << "," << centerTileY;
        refreshFilesWithCoordinates(centerTileX, centerTileY);
        // 再次尝试查找匹配的文件
        return getMatchedImages(centerTileX, centerTileY);
    }

    // 如果找到了匹配的文件，继续查找周围的文件
    for (int dy = -5; dy <= 5; ++dy) {
        for (int dx = -5; dx <= 5; ++dx) {
            int tileX = centerTileX + dx;
            int tileY = centerTileY + dy;

            for (auto it = filesWithCoordinates.begin(); it != filesWithCoordinates.end(); ++it) {
                const mapread::coordinate &coord = it.value();

                // 检查是否在附近的范围内
                if (coord.curLongitude == tileX && coord.curLatitude == tileY) {
                    // 检查文件是否存在
                    QFile file(it.key());
                    if (file.exists()) {
                        matchedImages.append(qMakePair(it.key(), coord));
                        break;
                    }
                }
            }
        }
    }

    // 缓存结果
    imageCache[qMakePair(centerTileX, centerTileY)] = matchedImages;

    return matchedImages;
}


void mapread::refreshFilesWithCoordinates(int centerTileX, int centerTileY)
{

    qDebug() << "my centerTile" << centerTileX << ',' << centerTileY << ',' << zoomLevel;



    // 清空现有的 filesWithCoordinates
    filesWithCoordinates.clear();

    // 生成所需的瓦片坐标集合
    requiredCoordinates.clear();
    for (int dx = -4; dx <= 4; ++dx) {
        for (int dy = -4; dy <= 4; ++dy) {
            int tileX = centerTileX + dx;
            int tileY = centerTileY + dy;
            requiredCoordinates.append({tileX, tileY});
        }
    }

    // 重新读取目录
    QString directoryPath = QString("D:/mapdata/mapabc-沈阳/satellite/%1").arg(zoomLevel);
    readDirectory(directoryPath);

    // 重新查找匹配的文件
    getMatchedImages(centerTileX, centerTileY);
}
