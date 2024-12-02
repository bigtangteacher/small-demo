#include "imageloader.h"

void ImageLoader::run()
{
    QList<QPair<QString, mapread::coordinate>> matchedImages;
    reader->loadImagesAsync(directoryPath, matchedImages);

    // 将结果保存到共享的列表中
    *result = matchedImages;

    // 使用 QMetaObject::invokeMethod 确保在主线程中更新 UI
    QMetaObject::invokeMethod(mainWindow, [this]() {
        mainWindow->displayImages(*result);
    }, Qt::QueuedConnection);
}
