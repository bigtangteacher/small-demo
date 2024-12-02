#include "mapsplice.h"
#include <QDebug>
#include <QPainter>


mapsplice::mapsplice(QObject *parent)
    : QObject(parent),mapread(new mapread())
{

}
