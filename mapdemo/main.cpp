#include <QApplication>
#include "mainwindow.h"
#include "mapread.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    qRegisterMetaType<QList<QPair<QString,mapread::coordinate>>>("QList<QPair<QString, mapread::coordinate>>");
    MainWindow mainWindow;
    mainWindow.show();
    return app.exec();
}
