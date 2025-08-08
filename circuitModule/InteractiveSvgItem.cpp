#include "InteractiveSvgItem.h"
#include <QPainter>
#include <QGraphicsSceneHoverEvent>
#include <QFile>
#include <QDebug>
#include "pugixml.hpp"

// 构造函数
InteractiveSvgMapItem::InteractiveSvgMapItem(const QString& svgPath)
	: m_highlightedLineIdx(-1)
{
	setAcceptHoverEvents(true);
	parseSvgAndInit(svgPath);
}

// 返回画布边界
QRectF InteractiveSvgMapItem::boundingRect() const
{
	return QRectF(0, 0, m_bgPixmap.width(), m_bgPixmap.height());
}

// 主绘制函数
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
}

// 设置高亮的线路
void InteractiveSvgMapItem::setHighlightedLine(int idx)
{
	if (m_highlightedLineIdx != idx) {
		m_highlightedLineIdx = idx;
		update();
	}
}

// 鼠标悬停事件：自动高亮最近线路（你可以自己定制更复杂的逻辑）
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
	if (minDist < 15.0)  // 只有靠近线才高亮
		setHighlightedLine(closest);
	else
		setHighlightedLine(-1);
}

// 解析SVG并初始化底图与结构体
void InteractiveSvgMapItem::parseSvgAndInit(const QString& svgPath)
{
	// 1. 先用 QSvgRenderer 渲染底图为Pixmap
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

	// 2. 用 pugixml 解析polyline
	QFile file(svgPath);
	if (!file.open(QIODevice::ReadOnly)) {
		qWarning("SVG文件无法打开！");
		return;
	}
	QByteArray svgData = file.readAll();
	pugi::xml_document doc;
	if (!doc.load_buffer(svgData.data(), svgData.size())) {
		qWarning("SVG解析失败！");
		return;
	}

	// 绘制链路部分
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
	// 你也可以添加对 path、text等的支持
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
