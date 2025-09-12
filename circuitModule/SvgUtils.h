#ifndef SVGUTILS_H
#define SVGUTILS_H

#include <QVector>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QTransform>
#include <QColor>
#include "pugixml.hpp"

namespace utils {

// 将 SVG path 的 d 解析为“本地坐标”的折线点列（不做任何 QTransform）。
// 支持 M/m L/l H/h V/v Z/z；Q/q、C/c 以 curveSegments 份近似；A/a 目前不支持（可后续扩展）。
QVector<QPointF> parseSvgPathToPolyline(const QString& d, int curveSegments = 12);

// 解析 transform="matrix(a b c d e f)" 为 QTransform；无法解析则返回单位矩阵。
QTransform parseTransformMatrix(const pugi::xml_node& node);

// 解析 points 属性（如 "x1,y1 x2,y2 ..."）为点列。
QVector<QPointF> parsePointsAttr(const QString& pointsStr);

// 线段最短距离（pt 到线段 ab 的距离）。
double pointToSegmentDistance(const QPointF& pt, const QPointF& a, const QPointF& b);

// 解析颜色字符串（如 "#rrggbb"、"red"）并应用不透明度（0..1）。
QColor parseColor(const char* s, double opacity = 1.0);

// 计算节点（本身是 path 或包含若干 path 子节点）在叠加额外变换后的包围盒。
// extraTransform 一般为文档到像素的整体变换；内部会与节点自身的 transform 相乘。
bool computePathBoundingRect(const pugi::xml_node& node, const QTransform& extraTransform, QRectF& outRect);

void simplifyPolyline(QVector<QPointF>& pts, qreal tol);

}

#endif // SVGUTILS_H
