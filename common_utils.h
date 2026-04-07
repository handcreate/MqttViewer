#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include <QColor>
#include <QRandomGenerator>

inline QColor randomColor()
{
    // return QColor(
    //     QRandomGenerator::global()->bounded(256),
    //     QRandomGenerator::global()->bounded(256),
    //     QRandomGenerator::global()->bounded(256)
    //     );
    return QColor::fromHsv(
        QRandomGenerator::global()->bounded(360),  // 色相
        200 + QRandomGenerator::global()->bounded(56), // 饱和度
        200 + QRandomGenerator::global()->bounded(56)  // 明度
        );
}

#endif // COMMON_UTILS_H
