#ifndef SVGUTILS_H
#define SVGUTILS_H

#include <QVector>
#include <QPointF>
#include <QString>

namespace utils {

// 将 SVG path 的 d 解析为“本地坐标”的折线点列（不做任何 QTransform）。
// 支持 M/m L/l H/h V/v Z/z；Q/q、C/c 以 curveSegments 份近似；A/a 目前不支持（可后续扩展）。
QVector<QPointF> parseSvgPathToPolyline(const QString& d, int curveSegments = 12);

}

#endif // SVGUTILS_H
