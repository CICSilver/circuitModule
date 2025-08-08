#ifndef INTERACTIVESVGMAPITEM_H
#define INTERACTIVESVGMAPITEM_H

#include <QGraphicsItem>
#include <QPixmap>
#include <QVector>
#include <QPointF>
#include <QString>
#include <QSvgRenderer>
#include "pugixml.hpp"

struct MapLine
{
	QVector<QPointF> points;
	QString type; // "virtual"/"logic"/"optical"
};

struct Plate
{
	QRectF rect;
	QList<PlateCircle> circles;	// 压板的两个圆
};

struct PlateCircle {
	QPointF center;
	qreal   radius;
	QColor  stroke;
	int     strokeWidth;
	QColor  fill;
	qreal   fillOpacity; // 0~1
};

static QVector<double> extractNumbers(const QString& d) {
	QVector<double> out;
	QString num; num.reserve(d.size());
	for (int i = 0; i < d.size(); ++i) {
		const QChar c = d[i];
		if (c.isDigit() || c == '-' || c == '.') {
			num.append(c);
		}
		else {
			if (!num.isEmpty()) { out.append(num.toDouble()); num.clear(); }
		}
	}
	if (!num.isEmpty()) out.append(num.toDouble());
	return out;
}

static QColor parseColor(const char* s, double opacity = 1.0) {
	if (!s || !*s) return Qt::transparent;
	QColor c(QString::fromLatin1(s));
	if (!c.isValid()) return Qt::transparent;
	c.setAlphaF(qBound(0.0, opacity, 1.0));
	return c;
}


class InteractiveSvgMapItem : public QGraphicsItem
{
public:
	InteractiveSvgMapItem(const QString& svgPath);

	QRectF boundingRect() const;
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

	void setHighlightedLine(int idx);

protected:
	void hoverMoveEvent(QGraphicsSceneHoverEvent* event);

private:
	void parseSvgAndInit(const QString& svgPath);
	double pointToSegmentDistance(const QPointF& pt, const QPointF& a, const QPointF& b);
	QVector<QPointF> parsePointsAttr(const QString& pointsStr);
    void drawPlateIcon(QPainter* painter, const QPointF& center) const;
	// 解析svg中圆的贝塞尔近似描述
	QVector<PlateCircle> parsePlateCircles(const pugi::xml_node& plateCircleNode);
	// 绘制压板的圆
	void drawPlateCircles(QPainter* p, const QVector<PlateCircle>& cs);



	QPixmap m_bgPixmap;
	QVector<MapLine> m_allLines;
    QVector<Plate> m_allPlates;
	int m_highlightedLineIdx;
};

#endif // INTERACTIVESVGMAPITEM_H
