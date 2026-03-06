#include "directitems.h"
#include "svgmodel.h"
#include "SvgUtils.h"
#include <QPainter>
#include <QPolygonF>
#include <QApplication>
#include <QPainterPath>
#include <QVector>
#include <QStringList>
#include <QFontMetrics>
#include <qmath.h>


directItemBase::~directItemBase()
{}

static double direct_angle_by_vec(const QPointF& vec)
{
	double direction = atan2(-vec.y(), vec.x());
	double angle = direction * (180.0 / M_PI);
	return angle;
}

static double direct_angle_to_radians(double angle)
{
	return angle * M_PI / 180.0;
}

static void direct_draw_arrow(QPainter* p, const QPointF& endPoint, double angle, const QColor& color, int arrowLen)
{
	QPointF leftArrowPoint = endPoint + QPointF(
		arrowLen * cos(direct_angle_to_radians(angle + 150.0)),
		-arrowLen * sin(direct_angle_to_radians(angle + 150.0)));
	QPointF rightArrowPoint = endPoint + QPointF(
		arrowLen * cos(direct_angle_to_radians(angle - 150.0)),
		-arrowLen * sin(direct_angle_to_radians(angle - 150.0)));
	int offset = 8;
	QPointF offsetPoint = QPointF(
		endPoint.x() + offset * cos(direct_angle_to_radians(angle)),
		endPoint.y() - offset * sin(direct_angle_to_radians(angle)));
	QBrush brush(color, Qt::SolidPattern);
	QPen pen(Qt::NoPen);
	p->setPen(pen);
	p->setBrush(brush);
	QVector<QPointF> points;
	points << leftArrowPoint << offsetPoint << rightArrowPoint << endPoint;
	p->drawPolygon(points.data(), 4);
}


static void direct_draw_arrow_outline(QPainter* p, const QPointF& endPoint, double angle, const QColor& color, int arrowLen, int penWidth)
{
	QPointF leftArrowPoint = endPoint + QPointF(
		arrowLen * cos(direct_angle_to_radians(angle + 150.0)),
		-arrowLen * sin(direct_angle_to_radians(angle + 150.0)));
	QPointF rightArrowPoint = endPoint + QPointF(
		arrowLen * cos(direct_angle_to_radians(angle - 150.0)),
		-arrowLen * sin(direct_angle_to_radians(angle - 150.0)));
	int offset = 8;
	QPointF offsetPoint = QPointF(
		endPoint.x() + offset * cos(direct_angle_to_radians(angle)),
		endPoint.y() - offset * sin(direct_angle_to_radians(angle)));
	QPen pen(color);
	pen.setWidth(penWidth);
	p->setPen(pen);
	p->setBrush(Qt::NoBrush);
	QPolygonF poly;
	poly << leftArrowPoint << offsetPoint << rightArrowPoint << endPoint;
	p->drawPolygon(poly);
}

static void direct_draw_conn_circle(QPainter* p, const QPoint& pt, int radius, bool isCircleUnderPt)
{
	QBrush brush(utils::ColorHelper::Color(utils::ColorHelper::pure_green));
	p->setBrush(brush);
	p->setPen(Qt::NoPen);
	int y = isCircleUnderPt ? pt.y() + radius : pt.y() - radius;
	p->drawEllipse(QPoint(pt.x(), y), radius, radius);
}

static QPoint direct_get_arrow_pt(const QPoint& pt, int arrowLen, int conn_r, double angle, bool isUnderConnpt, int offset)
{
	if (qAbs(angle - 90.0) < 0.001 || qAbs(angle + 90.0) < 0.001) {
		if (isUnderConnpt) {
			return QPoint(pt.x(), pt.y() + 2 * conn_r + (angle > 0 ? 1 : (2 * arrowLen + 5)) + offset);
		} else {
			return QPoint(pt.x(), pt.y() - 2 * conn_r - (angle > 0 ? (2 * arrowLen + 5) : 1) - offset);
		}
	}
	double rad = direct_angle_to_radians(angle);
	int direction = isUnderConnpt ? 1 : -1;
	int totalOffset = 2 * conn_r + arrowLen + offset;
	return QPoint(pt.x() + direction * totalOffset * cos(rad), pt.y() - direction * totalOffset * sin(rad));
}

static void direct_draw_single_port_text(QPainter* painter, const QPoint& connPoint, bool isTopSide, bool isSwitch, const QString& port, int conn_r)
{
	if (port.isEmpty()) return;
	int offset = conn_r * 2 + 10;
	QPen pen;
	pen.setColor(Qt::white);
	QFont font = painter->font();
	font.setPointSize(12);
	painter->setPen(pen);
	painter->setFont(font);
	int portPtY = 0;
	if (isTopSide)
	{
		portPtY = isSwitch ? connPoint.y() - offset : connPoint.y() + offset;
	}
	else
	{
		portPtY = isSwitch ? connPoint.y() + offset : connPoint.y() - offset;
	}
	QFontMetrics fm(painter->font());
	int textWidth = fm.width(port);
	QPoint port_lt_pt(connPoint.x() - textWidth / 2, portPtY - 5);
	QPoint port_rb_pt(connPoint.x() + textWidth / 2, portPtY + fm.height());
	painter->drawText(QRect(port_lt_pt, port_rb_pt), port);
}

static bool direct_is_top_anchor(const QPoint& point, const IedRect* rect)
{
	if (!rect)
	{
		return true;
	}
	int topDistance = qAbs(point.y() - (int)rect->y);
	int bottomDistance = qAbs(point.y() - (int)(rect->y + rect->height));
	return topDistance <= bottomDistance;
}

static void direct_draw_port_text(QPainter* painter, const QPoint& startPoint, const QPoint& endPoint,
	bool startAtTop, bool endAtTop, bool startIsSwitch, bool endIsSwitch,
	const QString& startPort, const QString& endPort, int conn_r)
{
	direct_draw_single_port_text(painter, startPoint, startAtTop, startIsSwitch, startPort, conn_r);
	direct_draw_single_port_text(painter, endPoint, endAtTop, endIsSwitch, endPort, conn_r);
}


IedItem::IedItem(QGraphicsItem* parent)
	: m_extendHeight(0)
	, m_innerGap(0)
	, m_padding(0)
	, m_borderColor(0)
	, m_underColor(0)
	, m_isSwitcher(false)
{
	Q_UNUSED(parent);
	m_itemType = Ied;
}

IedItem::~IedItem()
{
}

void IedItem::setFromIedRect(const IedRect& rect, bool isSwitcher)
{
	prepareGeometryChange();
	m_rect = QRectF(rect.x, rect.y, rect.width, rect.height);
	m_extendHeight = rect.extend_height;
	m_innerGap = rect.inner_gap;
	m_padding = rect.padding;
	m_borderColor = rect.border_color;
	m_underColor = rect.underground_color;
	m_iedName = rect.iedName;
	m_iedDesc = rect.iedDesc;
	m_isSwitcher = isSwitcher;
	update();
}

QRectF IedItem::boundingRect() const
{
	if (m_rect.isNull()) return QRectF();
	if (m_extendHeight > 0 && !m_isSwitcher) {
		qreal w = m_rect.width() + 2 * m_innerGap;
		qreal h = m_rect.height() + m_extendHeight + 2 * m_innerGap;
		QRectF outer(m_rect.x() - m_innerGap, m_rect.y() - m_innerGap, w, h);
		return outer.adjusted(-2.0, -2.0, 2.0, 2.0);
	}
	return m_rect.adjusted(-2.0, -2.0, 2.0, 2.0);
}

void IedItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);
	if (m_rect.isNull()) return;
	QPen pen;
	QBrush brush;
	QColor under = (m_underColor == 0) ? utils::ColorHelper::Color(utils::ColorHelper::ied_underground) : utils::ColorHelper::Color(m_underColor);
	brush.setColor(under);
	if (!m_isSwitcher && m_extendHeight > 0) {
		brush.setColor(utils::ColorHelper::Color(utils::ColorHelper::ied_underground));
		pen.setColor(utils::ColorHelper::Color(utils::ColorHelper::ied_border));
		pen.setStyle(Qt::DashLine);
		painter->setPen(pen);
		painter->setBrush(brush);
		QRectF outer(m_rect.x() - m_innerGap, m_rect.y() - m_innerGap, m_rect.width() + 2 * m_innerGap, m_rect.height() + m_extendHeight + 2 * m_innerGap);
		painter->fillRect(outer, brush);
		painter->setBrush(Qt::NoBrush);
		painter->drawRect(outer);
	}
	pen.setStyle(Qt::SolidLine);
	pen.setColor(utils::ColorHelper::Color(m_borderColor));
	painter->setPen(pen);
	brush.setColor(under);
	if (m_underColor == 0) {
		painter->setBrush(Qt::NoBrush);
		painter->drawRect(m_rect);
	} else {
		painter->fillRect(m_rect, brush);
		if (!m_isSwitcher) {
			painter->setBrush(Qt::NoBrush);
			painter->drawRect(m_rect);
		}
	}
	if (m_isSwitcher) {
		drawText(painter, m_rect, m_iedName, m_iedDesc, 14);
	} else {
		drawText(painter, m_rect, m_iedName, m_iedDesc, 12);
	}
}

void IedItem::drawText(QPainter* painter, const QRectF& rect, const QString& name, const QString& desc, int pointSize)
{
	QPen pen(Qt::white);
	painter->setPen(pen);
	int maxWidth = int(rect.width()) - 2 * m_padding;
	if (maxWidth < 10) maxWidth = 10;
	QStringList lines;
	int lineHeight = 0;
	int totalTextHeight = 0;
	int size = pointSize;
	while (size >= 8) {
		QFont font = painter->font();
		font.setPointSize(size);
		painter->setFont(font);
		QFontMetrics metrics(painter->font());
		lines.clear();
		QString currentLine;
		int currentWidth = 0;
		lines.append(name);
		lines.append("");
		for (int i = 0; i < desc.size(); ++i) {
			QChar ch = desc.at(i);
			int charWidth = metrics.width(ch);
			if (currentWidth + charWidth > maxWidth) {
				lines.append(currentLine);
				currentLine.clear();
				currentWidth = 0;
			}
			currentLine.append(ch);
			currentWidth += charWidth;
		}
		if (!currentLine.isEmpty()) lines.append(currentLine);
		lineHeight = metrics.height();
		totalTextHeight = lines.size() * lineHeight;
		if (totalTextHeight <= int(rect.height()) - 2) {
			break;
		}
		--size;
	}
	int text_y = int(rect.y()) + (int(rect.height()) - totalTextHeight) / 2 + lineHeight - painter->fontMetrics().descent();
	for (int li = 0; li < lines.size(); ++li) {
		int textWidth = painter->fontMetrics().width(lines[li]);
		int text_x = int(rect.x()) + (int(rect.width()) - textWidth) / 2;
		painter->drawText(text_x, text_y, lines[li]);
		text_y += lineHeight;
	}
}

DirectPlateItem::DirectPlateItem(QGraphicsItem* parent)
	: m_closed(true)
	, m_code(0)
{
	Q_UNUSED(parent);
	m_itemType = Plate;
	setAcceptHoverEvents(true);
}

DirectPlateItem::~DirectPlateItem()
{
}

QRectF DirectPlateItem::boundingRect() const
{
	if (m_rect.isNull()) return QRectF();
	return m_rect.adjusted(-3.0, -3.0, 3.0, 3.0);
}

void DirectPlateItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);
	if (m_rect.isNull()) return;
	// draw plate rect
	QPen pen;
	pen.setColor(QColor(255, 255, 255));
	pen.setWidth(2);
	pen.setStyle(Qt::DashDotLine);
	QVector<qreal> dash; dash << 3 << 3;
	pen.setDashPattern(dash);
	painter->setPen(pen);
	painter->setBrush(QColor(0x23, 0x3f, 0x4f));
	painter->drawRect(m_rect);

	// circles and lines
	QColor circleColor = m_closed ? QColor(Qt::green) : QColor(Qt::red);
	QPen cpen(circleColor);
	cpen.setWidth(2);
	painter->setPen(cpen);
	painter->setBrush(Qt::NoBrush);
	painter->drawEllipse(m_circle1, PLATE_CIRCLE_RADIUS, PLATE_CIRCLE_RADIUS);
	painter->drawEllipse(m_circle2, PLATE_CIRCLE_RADIUS, PLATE_CIRCLE_RADIUS);

	if (m_closed) {
		painter->drawLine(m_lineTop);
		painter->drawLine(m_lineBottom);
	}
}

void DirectPlateItem::setFromPlateRect(const PlateRect& rect, const QString& idKey)
{
	prepareGeometryChange();
	m_rect = QRectF(rect.rect);
	m_iedName = rect.iedName;
	m_ref = rect.ref;
	m_desc = rect.desc;
	m_code = rect.code;
	m_idKey = idKey;
	rebuildGeometry();
	update();
}

void DirectPlateItem::setClosed(bool closed)
{
	if (m_closed == closed) return;
	m_closed = closed;
	update();
}

void DirectPlateItem::rebuildGeometry()
{
	if (m_rect.isNull()) {
		m_circle1 = QPointF();
		m_circle2 = QPointF();
		m_lineTop = QLineF();
		m_lineBottom = QLineF();
		return;
	}
	qreal cx = m_rect.x() + m_rect.width() * 0.5 - PLATE_WIDTH / 2.0 + PLATE_CIRCLE_RADIUS + PLATE_GAP;
	qreal cy = m_rect.y() + m_rect.height() * 0.5;
	qreal distance = PLATE_WIDTH - PLATE_CIRCLE_RADIUS * 4;
	m_circle1 = QPointF(cx, cy);
	m_circle2 = QPointF(cx + distance, cy);
	m_lineTop = QLineF(QPointF(cx, cy - PLATE_CIRCLE_RADIUS), QPointF(cx + distance, cy - PLATE_CIRCLE_RADIUS));
	m_lineBottom = QLineF(QPointF(cx + distance, cy + PLATE_CIRCLE_RADIUS), QPointF(cx, cy + PLATE_CIRCLE_RADIUS));
}
LineItem::LineItem(QGraphicsItem* parent)
	: m_color(Qt::green)
	, m_width(1)
{
	Q_UNUSED(parent);
	m_itemType = CircuitLine;
	setAcceptHoverEvents(true);
}

LineItem::~LineItem()
{
}

void LineItem::setPoints(const QVector<QPointF>& pts)
{
	prepareGeometryChange();
	m_points = pts;
	update();
}

void LineItem::setColor(const QColor& color)
{
	m_color = color;
	update();
}

void LineItem::setWidth(int w)
{
	m_width = w;
	update();
}

QRectF LineItem::boundingRect() const
{
	if (m_points.isEmpty()) return QRectF();
	qreal minX = m_points[0].x();
	qreal maxX = m_points[0].x();
	qreal minY = m_points[0].y();
	qreal maxY = m_points[0].y();
	for (int i = 1; i < m_points.size(); ++i) {
		const QPointF& p = m_points[i];
		if (p.x() < minX) minX = p.x();
		if (p.x() > maxX) maxX = p.x();
		if (p.y() < minY) minY = p.y();
		if (p.y() > maxY) maxY = p.y();
	}
	qreal pad = qMax(1, m_width) + 2;
	return QRectF(QPointF(minX - pad, minY - pad), QPointF(maxX + pad, maxY + pad));
}

void LineItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);
	if (m_points.size() < 2) return;
	QPen pen(m_color);
	pen.setWidth(m_width);
	pen.setStyle(Qt::SolidLine);
	painter->setPen(pen);
	for (int i = 1; i < m_points.size(); ++i) {
		painter->drawLine(m_points[i - 1], m_points[i]);
	}
}
DirectVirtualLineItem::DirectVirtualLineItem(QGraphicsItem* parent)
	: m_virtualType(0)
	, m_lineColor(Qt::green)
	, m_arrowColor(Qt::green)
	, m_lineWidth(1)
	, m_valueVisible(true)
	, m_blinking(false)
	, m_blinkOn(true)
	, m_highlighted(false)
	, m_circuitCode(0)
{
	Q_UNUSED(parent);
	m_itemType = CircuitLine;
	setAcceptHoverEvents(true);
}

DirectVirtualLineItem::~DirectVirtualLineItem()
{
}

QRectF DirectVirtualLineItem::boundingRect() const
{
	QRectF rect;
	rect |= QRectF(m_startPoint, m_endPoint).normalized();
	if (!m_startValRect.isNull()) rect |= m_startValRect;
	if (!m_endValRect.isNull()) rect |= m_endValRect;
	if (!m_descRect.isNull()) rect |= m_descRect;
	qreal pad = qMax(1, m_lineWidth) + 8;
	return rect.adjusted(-pad, -pad, pad, pad);
}

void DirectVirtualLineItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);
	if (m_blinking && !m_blinkOn) return;
	QPen pen(m_lineColor);
	int w = m_lineWidth;
	if (m_highlighted) {
		w = m_lineWidth * 3;
		if (w < 2) w = 2;
	}
	pen.setWidth(w);
	painter->setPen(pen);
	painter->drawLine(m_startPoint, m_endPoint);
	if (m_arrowColor.isValid()) {
		painter->save();
		double angle = direct_angle_by_vec(m_endPoint - m_startPoint);
		direct_draw_arrow(painter, m_endPoint, angle, m_arrowColor, ARROW_LEN);
		painter->restore();
	}
	QColor iconColor = m_lineColor;
	painter->setBrush(Qt::NoBrush);
	if (m_virtualType == SV)
		utils::drawSvIcon(painter, m_startIconPt.toPoint(), ICON_LENGTH, iconColor);
	else
		utils::drawGseIcon(painter, m_startIconPt.toPoint(), ICON_LENGTH, iconColor);
	if (m_virtualType == SV)
		utils::drawSvIcon(painter, m_endIconPt.toPoint(), ICON_LENGTH, iconColor);
	else
		utils::drawGseIcon(painter, m_endIconPt.toPoint(), ICON_LENGTH, iconColor);
	QFont font = painter->font();
	font.setPointSize(12);
	painter->setFont(font);
	painter->setPen(Qt::white);
	if (m_valueVisible) {
		if (!m_startValRect.isNull())
			painter->drawText(m_startValRect, Qt::AlignCenter, m_outValStr);
		if (!m_endValRect.isNull())
			painter->drawText(m_endValRect, Qt::AlignCenter, m_inValStr);
	}
	if (!m_descRect.isNull())
		painter->drawText(m_descRect, Qt::AlignVCenter | Qt::AlignLeft, m_descStr);
}

void DirectVirtualLineItem::setFromVirtualLine(const VirtualCircuitLine& line, int vtype)
{
	prepareGeometryChange();
	m_startPoint = line.startPoint;
	m_endPoint = line.endPoint;
	m_startIconPt = line.startIconPt;
	m_endIconPt = line.endIconPt;
	m_startValRect = QRectF(line.startValRect);
	m_endValRect = QRectF(line.endValRect);
	m_descRect = QRectF(line.circuitDescRect);
	m_outValStr = line.valStr;
	m_inValStr = line.valStr;
	m_descStr = line.circuitDesc;
	m_virtualType = vtype;
	update();
}

void DirectVirtualLineItem::setVirtualType(int vtype)
{
	m_virtualType = vtype;
	update();
}

void DirectVirtualLineItem::setValues(const QString& outVal, const QString& inVal)
{
	m_outValStr = outVal;
	m_inValStr = inVal;
	update();
}

void DirectVirtualLineItem::setBlinking(bool blinking)
{
	m_blinking = blinking;
	update();
}

void DirectVirtualLineItem::setBlinkOn(bool on)
{
	m_blinkOn = on;
	update();
}

void DirectVirtualLineItem::setHighlighted(bool highlighted)
{
	m_highlighted = highlighted;
	update();
}

QPainterPath DirectVirtualLineItem::shape() const
{
	QPainterPath path;
	path.moveTo(m_startPoint);
	path.lineTo(m_endPoint);
	QPainterPathStroker stroker;
	qreal w = qMax(2, m_lineWidth + 4);
	stroker.setWidth(w);
	return stroker.createStroke(path);
}

void DirectVirtualLineItem::setCircuitCode(quint64 code)
{
	m_circuitCode = code;
}

quint64 DirectVirtualLineItem::circuitCode() const
{
	return m_circuitCode;
}

int DirectVirtualLineItem::virtualType() const
{
	return m_virtualType;
}

bool DirectVirtualLineItem::isBlinking() const
{
	return m_blinking;
}

void DirectVirtualLineItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
	Q_UNUSED(event);
	if (QApplication::mouseButtons() & Qt::LeftButton) return;
	setHighlighted(true);
}

void DirectVirtualLineItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
	Q_UNUSED(event);
	if (QApplication::mouseButtons() & Qt::LeftButton) return;
	setHighlighted(false);
}

void DirectVirtualLineItem::setLineColor(const QColor& color)
{
	m_lineColor = color;
	update();
}

void DirectVirtualLineItem::setArrowColor(const QColor& color)
{
	m_arrowColor = color;
	update();
}

void DirectVirtualLineItem::setValueVisible(bool visible)
{
	m_valueVisible = visible;
	update();
}

DirectOpticalLineItem::DirectOpticalLineItem(QGraphicsItem* parent)
	: m_startIsSwitch(false)
	, m_endIsSwitch(false)
	, m_startAtTop(true)
	, m_endAtTop(false)
	, m_underRectY(0)
	, m_arrowState(Arrow_None)
	, m_lineCode(0)
	, m_lineColor(Qt::green)
	, m_lineWidth(1)
	, m_highlighted(false)
{
	Q_UNUSED(parent);
	m_itemType = CircuitLine;
	setAcceptHoverEvents(true);
}

DirectOpticalLineItem::~DirectOpticalLineItem()
{
}

QRectF DirectOpticalLineItem::boundingRect() const
{
	if (m_points.isEmpty()) return QRectF();
	qreal minX = m_points[0].x();
	qreal maxX = m_points[0].x();
	qreal minY = m_points[0].y();
	qreal maxY = m_points[0].y();
	for (int i = 1; i < m_points.size(); ++i) {
		const QPointF& p = m_points[i];
		if (p.x() < minX) minX = p.x();
		if (p.x() > maxX) maxX = p.x();
		if (p.y() < minY) minY = p.y();
		if (p.y() > maxY) maxY = p.y();
	}
	qreal pad = qMax(1, m_lineWidth * 3) + CONN_R * 2 + 8;
	return QRectF(QPointF(minX - pad, minY - pad), QPointF(maxX + pad, maxY + pad));
}

void DirectOpticalLineItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);
	if (m_points.size() < 2) return;
	int w = m_lineWidth;
	if (m_highlighted) {
		w = m_lineWidth * 3;
		if (w < 2) w = 2;
	}
	QPen pen(m_lineColor);
	pen.setWidth(w);
	painter->setPen(pen);
	painter->setBrush(Qt::NoBrush);
	QPolygonF poly(m_points);
	painter->drawPolyline(poly);
	QPoint upPoint = m_startPoint.y() < m_endPoint.y() ? m_startPoint : m_endPoint;
	QPoint downPoint = m_startPoint.y() < m_endPoint.y() ? m_endPoint : m_startPoint;
	direct_draw_conn_circle(painter, m_startPoint, CONN_R, m_startAtTop);
	direct_draw_conn_circle(painter, m_endPoint, CONN_R, m_endAtTop);
	int underRectY = m_underRectY > 0 ? m_underRectY : (upPoint.y() > downPoint.y() ? upPoint.y() : downPoint.y());
	double outAngle = -90;
	double inAngle = 90;
	if (m_arrowState & Arrow_Out) {
		QPoint pt1 = direct_get_arrow_pt(upPoint, ARROW_LEN, CONN_R, outAngle, true, 0);
		direct_draw_arrow_outline(painter, pt1, outAngle, m_lineColor, ARROW_LEN, w);
		QPoint pt2 = direct_get_arrow_pt(downPoint, ARROW_LEN, CONN_R, outAngle, downPoint.y() > underRectY, 0);
		direct_draw_arrow_outline(painter, pt2, outAngle, m_lineColor, ARROW_LEN, w);
	}
	if (m_arrowState & Arrow_In) {
		QPoint pt1 = direct_get_arrow_pt(upPoint, ARROW_LEN, CONN_R, inAngle, true, 0);
		direct_draw_arrow_outline(painter, pt1, inAngle, m_lineColor, ARROW_LEN, w);
		QPoint pt2 = direct_get_arrow_pt(downPoint, ARROW_LEN, CONN_R, inAngle, downPoint.y() > underRectY, 0);
		direct_draw_arrow_outline(painter, pt2, inAngle, m_lineColor, ARROW_LEN, w);
	}
	direct_draw_port_text(painter, m_startPoint, m_endPoint, m_startAtTop, m_endAtTop,
		m_startIsSwitch, m_endIsSwitch, m_startPort, m_endPort, CONN_R);
}

void DirectOpticalLineItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
	Q_UNUSED(event);
	m_highlighted = true;
	update();
}

QPainterPath DirectOpticalLineItem::shape() const
{
	QPainterPath path;
	if (m_points.size() < 2) return path;
	path.moveTo(m_points[0]);
	for (int i = 1; i < m_points.size(); ++i) {
		path.lineTo(m_points[i]);
	}
	QPainterPathStroker stroker;
	qreal w = qMax(2, m_lineWidth + 2);
	stroker.setWidth(w);
	return stroker.createStroke(path);
}



void DirectOpticalLineItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
	Q_UNUSED(event);
	m_highlighted = false;
	update();
}

void DirectOpticalLineItem::setFromOpticalLine(const OpticalCircuitLine& line)
{
	prepareGeometryChange();
	m_points.clear();
	m_points.append(QPointF(line.startPoint));
	for (int i = 0; i < line.midPoints.size(); ++i) {
		m_points.append(QPointF(line.midPoints[i]));
	}
	m_points.append(QPointF(line.endPoint));
	m_startPoint = line.startPoint;
	m_endPoint = line.endPoint;
	m_arrowState = line.arrowState;
	m_lineCode = line.lineCode;
	m_startPort.clear();
	m_endPort.clear();
	m_startIsSwitch = false;
	m_endIsSwitch = false;
	m_startAtTop = true;
	m_endAtTop = false;
	m_underRectY = 0;
	if (line.pSrcRect && line.pDestRect && line.pOpticalCircuit) {
		m_startIsSwitch = line.pSrcRect->iedName.contains("SW");
		m_endIsSwitch = line.pDestRect->iedName.contains("SW");
		m_underRectY = line.pSrcRect->y > line.pDestRect->y ? line.pSrcRect->y : line.pDestRect->y;
		m_startPort = line.pOpticalCircuit->srcIedPort;
		m_endPort = line.pOpticalCircuit->destIedPort;
		m_startAtTop = direct_is_top_anchor(line.startPoint, line.pSrcRect);
		m_endAtTop = direct_is_top_anchor(line.endPoint, line.pDestRect);
	}
	update();
}
