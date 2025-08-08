#include "InteractiveSvgItem.h"
#include <QPainter>
#include <QGraphicsSceneHoverEvent>
#include <QFile>
#include <QDebug>
#include "pugixml.hpp"
#include "svgmodel.h"
#include <QMap>

InteractiveSvgMapItem::InteractiveSvgMapItem(const QString& svgPath)
	: m_highlightedLineIdx(-1)
{
	setAcceptHoverEvents(true);
	parseSvgAndInit(svgPath);
}

QRectF InteractiveSvgMapItem::boundingRect() const
{
	return QRectF(0, 0, m_bgPixmap.width(), m_bgPixmap.height());
}

void InteractiveSvgMapItem::paint(QPainter* painter, const QStyleOptionGraphicsItem*, QWidget*)
{
	painter->drawPixmap(0, 0, m_bgPixmap);
	for (int i = 0; i < m_allLines.size(); ++i) {
		const MapLine& line = m_allLines[i];
		QPen pen;
		if (i == m_highlightedLineIdx) {
			pen.setColor(Qt::green);
			pen.setWidth(6);
		}
		else {
			pen.setColor(Qt::green);
			pen.setWidth(2);
		}
		painter->setPen(pen);
		for (int j = 1; j < line.points.size(); ++j)
			painter->drawLine(line.points[j - 1], line.points[j]);
	}
        foreach (const Plate& plate, m_allPlates) {
                QPen pen(Qt::white);
                pen.setWidth(2);
                pen.setStyle(Qt::DashDotLine);
                QVector<qreal> dashPattern; dashPattern << 3 << 3;
                pen.setDashPattern(dashPattern);
                painter->setPen(pen);
                painter->setBrush(QColor(0x23,0x3f,0x4f));
                painter->drawRect(plate.rect);
                painter->setBrush(Qt::NoBrush);
                drawPlateIcon(painter, plate.iconCenter);
        }
}

// ø·
void InteractiveSvgMapItem::setHighlightedLine(int idx)
{
	if (m_highlightedLineIdx != idx) {
		m_highlightedLineIdx = idx;
		update();
	}
}

void InteractiveSvgMapItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
	QPointF pos = event->pos();
	int closest = -1;
	double minDist = 99999.0;
	for (int i = 0; i < m_allLines.size(); ++i) {
		const QVector<QPointF>& pts = m_allLines[i].points;
		for (int j = 1; j < pts.size(); ++j) {
			QLineF seg(pts[j - 1], pts[j]);
			double dist = seg.p1() == seg.p2() ? QLineF(pos, seg.p1()).length() : pointToSegmentDistance(pos, seg.p1(), seg.p2());
			if (dist < minDist) {
				minDist = dist;
				closest = i;
			}
		}
	}
	if (minDist < 15.0) 
		setHighlightedLine(closest);
	else
		setHighlightedLine(-1);
}

void InteractiveSvgMapItem::parseSvgAndInit(const QString& svgPath)
{
	QSvgRenderer renderer(svgPath);
	QRectF viewBox = renderer.viewBoxF();
	QSize imageSize = viewBox.isValid() ?
		QSize(int(viewBox.width()), int(viewBox.height()))
		: QSize(2000, 2000);
	QPixmap pixmap(imageSize);
	pixmap.fill(Qt::black);
	QPainter painter(&pixmap);
	renderer.render(&painter);
	painter.end();
	m_bgPixmap = pixmap;

	QFile file(svgPath);
	if (!file.open(QIODevice::ReadOnly)) {
		qWarning("SVG文件打开失败");
		return;
	}
	QByteArray svgData = file.readAll();
	pugi::xml_document doc;
	if (!doc.load_buffer(svgData.data(), svgData.size())) {
		qWarning("SVG加载失败");
		return;
	}

	static const char* types[] = { "virtual", "logic", "optical" };
	for (int t = 0; t < 3; ++t) {
		QString xPath = QString("//g[@type='%1']/polyline").arg(types[t]);
		pugi::xpath_node_set lines = doc.select_nodes(xPath.toLocal8Bit());
		for (int i = 0; i < lines.size(); ++i) {
			pugi::xml_node polyline = lines[i].node();
			QString points = polyline.attribute("points").as_string();
			MapLine line;
			line.points = parsePointsAttr(points);
			line.type = types[t];
			m_allLines.append(line);
		}
	}
    QMap<QString, Plate> plateMap;
    pugi::xpath_node_set rectNodes = doc.select_nodes("//g[@type='plate-rect']/path");
    for (int i = 0; i < rectNodes.size(); ++i) {
            pugi::xml_node rect = rectNodes[i].node();
            pugi::xml_node parent = rect.parent();
            QString key = QString(parent.attribute("plate-desc").as_string()) + "|" + parent.attribute("plate-ref").as_string();
            double x = rect.attribute("x").as_double();
            double y = rect.attribute("y").as_double();
            double w = rect.attribute("width").as_double();
            double h = rect.attribute("height").as_double();
            Plate &p = plateMap[key];
            p.rect = QRectF(x, y, w, h);
    }
    pugi::xpath_node_set iconNodes = doc.select_nodes("//g[@type='plate-icon']");
    for (int i = 0; i < iconNodes.size(); ++i) {
            pugi::xml_node g = iconNodes[i].node();
            QString key = QString(g.attribute("plate-desc").as_string()) + "|" + g.attribute("plate-ref").as_string();
            pugi::xml_node circle = g.child("circle");
            if (circle) {
                    double cx = circle.attribute("cx").as_double();
                    double cy = circle.attribute("cy").as_double();
                    plateMap[key].iconCenter = QPointF(cx, cy);
            }
    }
	m_allPlates = plateMap.values().toVector();
}

double InteractiveSvgMapItem::pointToSegmentDistance(const QPointF& pt, const QPointF& a, const QPointF& b)
{
	double x = pt.x(), y = pt.y();
	double x1 = a.x(), y1 = a.y();
	double x2 = b.x(), y2 = b.y();
	double dx = x2 - x1;
	double dy = y2 - y1;

	if (dx == 0 && dy == 0)
		return std::sqrt((x - x1) * (x - x1) + (y - y1) * (y - y1));

	double t = ((x - x1) * dx + (y - y1) * dy) / (dx * dx + dy * dy);
	if (t < 0)
		return std::sqrt((x - x1) * (x - x1) + (y - y1) * (y - y1));
	else if (t > 1)
		return std::sqrt((x - x2) * (x - x2) + (y - y2) * (y - y2));
	double projx = x1 + t * dx;
	double projy = y1 + t * dy;
	return std::sqrt((x - projx) * (x - projx) + (y - projy) * (y - projy));
}

QVector<QPointF> InteractiveSvgMapItem::parsePointsAttr(const QString& pointsStr)
{
	QVector<QPointF> pts;
	QStringList ptList = pointsStr.split(' ', QString::SkipEmptyParts);
	for (int i = 0; i < ptList.size(); ++i) {
		QStringList xy = ptList[i].split(',');
		if (xy.size() == 2)
			pts.append(QPointF(xy[0].toDouble(), xy[1].toDouble()));
	}
	return pts;
}

void InteractiveSvgMapItem::drawPlateIcon(QPainter* painter, const QPointF& center) const
{
        painter->save();
        QPen pen(QColor(0x00ff00));
        pen.setWidth(2);
        painter->setPen(pen);
        int distance = PLATE_WIDTH - PLATE_CIRCLE_RADIUS * 4;
        painter->drawEllipse(center, PLATE_CIRCLE_RADIUS, PLATE_CIRCLE_RADIUS);
        painter->drawEllipse(center + QPointF(distance, 0), PLATE_CIRCLE_RADIUS, PLATE_CIRCLE_RADIUS);
        painter->save();
        painter->translate(center);
        int rectHeight = PLATE_CIRCLE_RADIUS * 2;
        QPointF p1(0, -rectHeight / 2), p2(distance, -rectHeight / 2);
        QPointF p3(distance, rectHeight / 2), p4(0, rectHeight / 2);
        painter->drawLine(p1, p2);
        painter->drawLine(p3, p4);
        painter->restore();
        painter->restore();
}
