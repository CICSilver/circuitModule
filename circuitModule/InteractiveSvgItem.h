#ifndef INTERACTIVESVGMAPITEM_H
#define INTERACTIVESVGMAPITEM_H

#include <QGraphicsItem>
#include <QPixmap>
#include <QVector>
#include <QPointF>
#include <QString>
#include <QSvgRenderer>

// 线路结构体
struct MapLine
{
	QVector<QPointF> points;
	QString type; // 可用来区分"virtual"/"logic"/"optical"等
};

class InteractiveSvgMapItem : public QGraphicsItem
{
public:
	InteractiveSvgMapItem(const QString& svgPath);

	QRectF boundingRect() const;
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

	// 交互接口
	void setHighlightedLine(int idx);

protected:
	void hoverMoveEvent(QGraphicsSceneHoverEvent* event);

private:
	void parseSvgAndInit(const QString& svgPath);
	double pointToSegmentDistance(const QPointF& pt, const QPointF& a, const QPointF& b);
	// 解析 polyline 的 points 属性为点集合
	QVector<QPointF> parsePointsAttr(const QString& pointsStr);
	

	QPixmap m_bgPixmap;
	QVector<MapLine> m_allLines;
	int m_highlightedLineIdx;
};

#endif // INTERACTIVESVGMAPITEM_H
