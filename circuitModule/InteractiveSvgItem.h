#ifndef INTERACTIVESVGMAPITEM_H
#define INTERACTIVESVGMAPITEM_H

#include <QGraphicsItem>
#include <QPixmap>
#include <QVector>
#include <QPointF>
#include <QString>
#include <QSvgRenderer>

// ·ṹ
struct MapLine
{
	QVector<QPointF> points;
	QString type; // "virtual"/"logic"/"optical"
};

struct Plate
{
        QRectF rect;
        QPointF iconCenter;
};

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
	

	QPixmap m_bgPixmap;
	QVector<MapLine> m_allLines;
        QVector<Plate> m_allPlates;
	int m_highlightedLineIdx;
};

#endif // INTERACTIVESVGMAPITEM_H
