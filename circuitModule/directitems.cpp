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

#define DIRECT_PORT_TEXT_OFFSET 12	// 端口文字距连接点偏移
#define DIRECT_ARROW_OFFSET 10	// 箭头距连接点偏移
#define DIRECT_BOUND_MARGIN 12	// 交互区域额外边距
#define DIRECT_DOUBLE_ARROW_GAP_RATIO 1.5	// 双向箭头前后错开比例
#define DIRECT_LOGIC_FRAME_TITLE_FONT_SIZE 10	// 逻辑链路框 标题字体大小
// 检修压板相关常量
#define DIRECT_MAINT_PLATE_CIRCLE_RADIUS 5
#define DIRECT_MAINT_PLATE_ICON_SPAN 36
#define DIRECT_MAINT_PLATE_BODY_WIDTH 26
#define DIRECT_MAINT_PLATE_BODY_HEIGHT 10
#define DIRECT_MAINT_PLATE_TEXT_HEIGHT 18
#define DIRECT_MAINT_PLATE_TEXT_FONT_SIZE 8
#define DIRECT_MAINT_PLATE_TEXT_ICON_GAP 30	// 检修压板文本与图标之间的间距
#define DIRECT_MAINT_PLATE_BOTTOM_MARGIN 6

directItemBase::~directItemBase()
{}

static QString direct_build_maint_plate_default_text()
{
	return QString::fromLocal8Bit("检修压板");
}

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

static QPointF direct_build_arrow_point_on_line(const QPointF& connPoint, const QPointF& outwardVec, int conn_r, int arrowLen, int offset)
{
	qreal vectorLength = qSqrt(outwardVec.x() * outwardVec.x() + outwardVec.y() * outwardVec.y());
	if (vectorLength < 0.001)
	{
		return connPoint;
	}
	qreal totalOffset = conn_r * 2 + arrowLen + offset;
	return QPointF(
		connPoint.x() + outwardVec.x() / vectorLength * totalOffset,
		connPoint.y() + outwardVec.y() / vectorLength * totalOffset);
}

static QPointF direct_offset_point_by_vec(const QPointF& point, const QPointF& vec, qreal offset)
{
	qreal vectorLength = qSqrt(vec.x() * vec.x() + vec.y() * vec.y());
	if (vectorLength < 0.001)
	{
		return point;
	}
	return QPointF(
		point.x() + vec.x() / vectorLength * offset,
		point.y() + vec.y() / vectorLength * offset);
}

static void direct_draw_single_port_text(QPainter* painter, const QPoint& connPoint, bool isTopSide, bool isSwitch, const QString& port, int conn_r)
{
	if (port.isEmpty())
	{
		return;
	}
	Q_UNUSED(isSwitch);
	int offset = conn_r * 2 + 10 + DIRECT_PORT_TEXT_OFFSET;
	QPen pen;
	pen.setColor(Qt::white);
	QFont font = painter->font();
	font.setPointSize(12);
	painter->setPen(pen);
	painter->setFont(font);
	int portPtY = isTopSide ? connPoint.y() - offset : connPoint.y() + offset;
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

static bool direct_is_point_attached_to_rect(const QPoint& point, const IedRect* rect)
{
	if (!rect)
	{
		return false;
	}
	int left = rect->x;
	int right = rect->x + rect->width;
	int top = rect->y;
	int bottom = rect->y + rect->height;
	bool withinHorizontal = point.x() >= left && point.x() <= right;
	bool withinVertical = point.y() >= top && point.y() <= bottom;
	if (withinHorizontal && (point.y() == top || point.y() == bottom))
	{
		return true;
	}
	if (withinVertical && (point.x() == left || point.x() == right))
	{
		return true;
	}
	return false;
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
	if (m_extendHeight > 0 && !m_isSwitcher)
	{
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
	if (m_rect.isNull())
	{
		return;
	}
	QPen pen;
	QBrush brush;
	QColor under = (m_underColor == 0) ? utils::ColorHelper::Color(utils::ColorHelper::ied_underground) : utils::ColorHelper::Color(m_underColor);
	brush.setColor(under);
	if (!m_isSwitcher && m_extendHeight > 0)
	{
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
	if (m_underColor == 0)
	{
		painter->setBrush(Qt::NoBrush);
		painter->drawRect(m_rect);
	}
	else
	{
		painter->fillRect(m_rect, brush);
		if (!m_isSwitcher)
		{
			painter->setBrush(Qt::NoBrush);
			painter->drawRect(m_rect);
		}
	}
	if (m_isSwitcher)
	{
		drawText(painter, m_rect, m_iedName, m_iedDesc, 14);
	}
	else
	{
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
	while (size >= 8)
	{
		QFont font = painter->font();
		font.setPointSize(size);
		painter->setFont(font);
		QFontMetrics metrics(painter->font());
		lines.clear();
		QString currentLine;
		int currentWidth = 0;
		lines.append(name);
		lines.append("");
		for (int i = 0; i < desc.size(); ++i)
		{
			QChar ch = desc.at(i);
			int charWidth = metrics.width(ch);
			if (currentWidth + charWidth > maxWidth)
			{
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
		if (totalTextHeight <= int(rect.height()) - 2)
		{
			break;
		}
		--size;
	}
	int text_y = int(rect.y()) + (int(rect.height()) - totalTextHeight) / 2 + lineHeight - painter->fontMetrics().descent();
	for (int li = 0; li < lines.size(); ++li)
	{
		int textWidth = painter->fontMetrics().width(lines[li]);
		int text_x = int(rect.x()) + (int(rect.width()) - textWidth) / 2;
		painter->drawText(text_x, text_y, lines[li]);
		text_y += lineHeight;
	}
}

LogicFrameItem::LogicFrameItem(QGraphicsItem* parent)
	: m_borderColor(Qt::green)
	, m_showLegend(false)
{
	Q_UNUSED(parent);
	m_itemType = Text;
}

LogicFrameItem::~LogicFrameItem()
{
}

void LogicFrameItem::setFrame(const QRectF& rect, const QString& title, const QColor& borderColor, bool showLegend)
{
	prepareGeometryChange();
	m_rect = rect;
	m_title = title;
	m_borderColor = borderColor;
	m_showLegend = showLegend;
	update();
}

QRectF LogicFrameItem::buildLegendRect() const
{
	if (!m_showLegend || m_rect.isNull())
	{
		return QRectF();
	}
	QFont font = QApplication::font();
	font.setPointSize(DIRECT_LOGIC_FRAME_TITLE_FONT_SIZE);
	QFontMetrics metrics(font);
	qreal descWidth = 180.0;
	qreal descHeight = metrics.height() * 2 + 10;
	return QRectF(m_rect.x(), m_rect.y() - descHeight - 20, descWidth, descHeight);
}

QRectF LogicFrameItem::boundingRect() const
{
	QRectF rect = m_rect;
	QRectF legendRect = buildLegendRect();
	if (!legendRect.isNull())
	{
		rect |= legendRect;
	}
	return rect.adjusted(-4.0, -4.0, 4.0, 4.0);
}

void LogicFrameItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);
	if (m_rect.isNull())
	{
		return;
	}
	QFont font = painter->font();
	font.setPointSize(DIRECT_LOGIC_FRAME_TITLE_FONT_SIZE);
	painter->setFont(font);
	QFontMetrics metrics(font);
	QPen pen(m_borderColor);
	pen.setStyle(Qt::DashLine);
	painter->setPen(pen);
	painter->setBrush(Qt::NoBrush);
	painter->drawRect(m_rect);
	QPen textPen(Qt::white);
	painter->setPen(textPen);
	painter->drawText(QPointF(m_rect.x() + 10, m_rect.y() + metrics.height()), m_title);
	QRectF legendRect = buildLegendRect();
	if (legendRect.isNull())
	{
		return;
	}
	QPen legendPen(Qt::white);
	legendPen.setStyle(Qt::DashLine);
	painter->setPen(legendPen);
	painter->drawRect(legendRect);
	qreal lineWidth = 70.0;
	qreal lineLeft = legendRect.x() + 5.0;
	qreal firstLineY = legendRect.y() + metrics.height() / 2.0 + 5.0;
	QPen gsePen(utils::ColorHelper::Color(utils::ColorHelper::line_gse));
	painter->setPen(gsePen);
	painter->drawLine(QPointF(lineLeft, firstLineY), QPointF(lineLeft + lineWidth, firstLineY));
	QPen svPen(utils::ColorHelper::Color(utils::ColorHelper::line_smv));
	painter->setPen(svPen);
	painter->drawLine(QPointF(lineLeft, firstLineY + metrics.height()), QPointF(lineLeft + lineWidth, firstLineY + metrics.height()));
	painter->setPen(textPen);
	painter->drawText(QPointF(lineLeft + lineWidth + 10.0, legendRect.y() + metrics.height()), QString::fromLatin1("Goose"));
	painter->drawText(QPointF(lineLeft + lineWidth + 10.0, legendRect.y() + metrics.height() * 2), QString::fromLatin1("SV"));
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
	const qreal kBoundingMargin = 3.0;
	return m_rect.adjusted(-kBoundingMargin, -kBoundingMargin, kBoundingMargin, kBoundingMargin);
}

void DirectPlateItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);
	if (m_rect.isNull())
	{
		return;
	}
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

	if (m_closed)
	{
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
	if (m_closed == closed)
	{
		return;
	}
	m_closed = closed;
	update();
}

void DirectPlateItem::rebuildGeometry()
{
	if (m_rect.isNull())
	{
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
DirectMaintainPlateItem::DirectMaintainPlateItem(QGraphicsItem* parent)
	: m_closed(true)
{
	Q_UNUSED(parent);
	m_itemType = Plate;
	m_displayText = direct_build_maint_plate_default_text();
}

DirectMaintainPlateItem::~DirectMaintainPlateItem()
{
}

QRectF DirectMaintainPlateItem::boundingRect() const
{
	QRectF rect = m_textRect.united(m_iconRect);
	if (rect.isNull())
	{
		return QRectF();
	}
	return rect.adjusted(-3.0, -3.0, 3.0, 3.0);
}

void DirectMaintainPlateItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);
	if (m_anchorRect.isNull())
	{
		return;
	}
	QPen textPen(Qt::white);
	painter->setPen(textPen);
	QFont font = painter->font();
	font.setPointSize(DIRECT_MAINT_PLATE_TEXT_FONT_SIZE);
	painter->setFont(font);
	QFontMetrics metrics(font);
	QString displayText = metrics.elidedText(m_displayText, Qt::ElideRight, (int)m_textRect.width());
	painter->drawText(m_textRect, Qt::AlignHCenter | Qt::AlignVCenter, displayText);
	QColor iconColor(0, 210, 0);
	QPen iconPen(iconColor);
	iconPen.setWidth(2);
	painter->setPen(iconPen);
	painter->setBrush(Qt::NoBrush);
	painter->drawEllipse(m_leftCircleCenter, DIRECT_MAINT_PLATE_CIRCLE_RADIUS, DIRECT_MAINT_PLATE_CIRCLE_RADIUS);
	painter->drawEllipse(m_rightCircleCenter, DIRECT_MAINT_PLATE_CIRCLE_RADIUS, DIRECT_MAINT_PLATE_CIRCLE_RADIUS);
	if (m_closed)
	{
		qreal topY = m_leftCircleCenter.y() - DIRECT_MAINT_PLATE_CIRCLE_RADIUS;
		qreal bottomY = m_leftCircleCenter.y() + DIRECT_MAINT_PLATE_CIRCLE_RADIUS;
		qreal leftX = m_leftCircleCenter.x();
		qreal rightX = m_rightCircleCenter.x();
		painter->drawLine(QPointF(leftX, topY), QPointF(rightX, topY));
		painter->drawLine(QPointF(leftX, bottomY), QPointF(rightX, bottomY));
	}
	else
	{
		qreal bladeLength = qAbs(m_rightCircleCenter.x() - m_leftCircleCenter.x());
		if (bladeLength < 1.0)
		{
			bladeLength = DIRECT_MAINT_PLATE_ICON_SPAN;
		}
		qreal bladeHalfWidth = DIRECT_MAINT_PLATE_BODY_HEIGHT / 2.0;
		qreal angle = -38.0 * M_PI / 180.0;
		QPointF direction(qCos(angle), qSin(angle));
		QPointF normal(-direction.y(), direction.x());
		QPointF startTop = m_leftCircleCenter + normal * bladeHalfWidth;
		QPointF startBottom = m_leftCircleCenter - normal * bladeHalfWidth;
		QPointF endTop = startTop + direction * bladeLength;
		QPointF endBottom = startBottom + direction * bladeLength;
		painter->drawLine(startTop, endTop);
		painter->drawLine(startBottom, endBottom);
		painter->drawLine(endTop, endBottom);
	}
}

void DirectMaintainPlateItem::setIedName(const QString& iedName)
{
	m_iedName = iedName;
}

void DirectMaintainPlateItem::setDisplayText(const QString& text)
{
	QString displayText = text;
	if (displayText.isEmpty())
	{
		displayText = direct_build_maint_plate_default_text();
	}
	m_displayText = displayText;
	update();
}

void DirectMaintainPlateItem::setStateByValue(int value)
{
	setClosed(value != 0);
}

void DirectMaintainPlateItem::setClosed(bool closed)
{
	if (m_closed == closed)
	{
		return;
	}
	m_closed = closed;
	update();
}

void DirectMaintainPlateItem::setAnchorRect(const QRectF& iedRect)
{
	prepareGeometryChange();
	m_anchorRect = iedRect;
	rebuildGeometry();
	update();
}

void DirectMaintainPlateItem::rebuildGeometry()
{
	if (m_anchorRect.isNull())
	{
		m_textRect = QRectF();
		m_iconRect = QRectF();
		m_bodyRect = QRectF();
		m_leftCircleCenter = QPointF();
		m_rightCircleCenter = QPointF();
		return;
	}
	qreal horizontalMargin = 6.0;
	qreal textWidth = m_anchorRect.width() - horizontalMargin * 2.0;
	if (textWidth < 20.0)
	{
		textWidth = m_anchorRect.width();
		horizontalMargin = 0.0;
	}
	qreal bottomY = m_anchorRect.y() + m_anchorRect.height() - DIRECT_MAINT_PLATE_BOTTOM_MARGIN;
	qreal iconCenterY = bottomY - DIRECT_MAINT_PLATE_CIRCLE_RADIUS - 2.0;
	qreal textTop = bottomY - DIRECT_MAINT_PLATE_TEXT_ICON_GAP - DIRECT_MAINT_PLATE_TEXT_HEIGHT;
	m_textRect = QRectF(m_anchorRect.x() + horizontalMargin, textTop, textWidth, DIRECT_MAINT_PLATE_TEXT_HEIGHT);
	qreal centerX = m_anchorRect.x() + m_anchorRect.width() / 2.0;
	qreal halfSpan = DIRECT_MAINT_PLATE_ICON_SPAN / 2.0;
	m_leftCircleCenter = QPointF(centerX - halfSpan, iconCenterY);
	m_rightCircleCenter = QPointF(centerX + halfSpan, iconCenterY);
	m_bodyRect = QRectF(centerX - DIRECT_MAINT_PLATE_BODY_WIDTH / 2.0, iconCenterY - DIRECT_MAINT_PLATE_BODY_HEIGHT / 2.0,
		DIRECT_MAINT_PLATE_BODY_WIDTH, DIRECT_MAINT_PLATE_BODY_HEIGHT);
	QRectF leftCircleRect(m_leftCircleCenter.x() - DIRECT_MAINT_PLATE_CIRCLE_RADIUS, m_leftCircleCenter.y() - DIRECT_MAINT_PLATE_CIRCLE_RADIUS,
		DIRECT_MAINT_PLATE_CIRCLE_RADIUS * 2.0, DIRECT_MAINT_PLATE_CIRCLE_RADIUS * 2.0);
	QRectF rightCircleRect(m_rightCircleCenter.x() - DIRECT_MAINT_PLATE_CIRCLE_RADIUS, m_rightCircleCenter.y() - DIRECT_MAINT_PLATE_CIRCLE_RADIUS,
		DIRECT_MAINT_PLATE_CIRCLE_RADIUS * 2.0, DIRECT_MAINT_PLATE_CIRCLE_RADIUS * 2.0);
	QRectF closedRect = leftCircleRect.united(rightCircleRect);
	qreal bladeLength = qAbs(m_rightCircleCenter.x() - m_leftCircleCenter.x());
	if (bladeLength < 1.0)
	{
		bladeLength = DIRECT_MAINT_PLATE_ICON_SPAN;
	}
	qreal bladeHalfWidth = DIRECT_MAINT_PLATE_BODY_HEIGHT / 2.0;
	qreal angle = -38.0 * M_PI / 180.0;
	QPointF direction(qCos(angle), qSin(angle));
	QPointF normal(-direction.y(), direction.x());
	QPointF openP1 = m_leftCircleCenter + normal * bladeHalfWidth;
	QPointF openP2 = openP1 + direction * bladeLength;
	QPointF openP3 = m_leftCircleCenter - normal * bladeHalfWidth;
	QPointF openP4 = openP3 + direction * bladeLength;
	QRectF openRect(openP1, openP1);
	openRect = openRect.united(QRectF(openP2, openP2));
	openRect = openRect.united(QRectF(openP3, openP3));
	openRect = openRect.united(QRectF(openP4, openP4));
	m_iconRect = closedRect.united(openRect).adjusted(-3.0, -3.0, 3.0, 3.0);
}

LineItem::LineItem(QGraphicsItem* parent)
	: m_color(Qt::green)
	, m_arrowColor(Qt::green)
	, m_width(1)
	, m_arrowVisible(false)
	, m_startArrowState(Arrow_None)
	, m_endArrowState(Arrow_None)
	, m_highlighted(false)
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
	for (int i = 1; i < m_points.size(); ++i)
	{
		const QPointF& p = m_points[i];
		if (p.x() < minX) minX = p.x();
		if (p.x() > maxX) maxX = p.x();
		if (p.y() < minY) minY = p.y();
		if (p.y() > maxY) maxY = p.y();
	}
	qreal pad = qMax(1, m_width) + ARROW_LEN + 2;
	return QRectF(QPointF(minX - pad, minY - pad), QPointF(maxX + pad, maxY + pad));
}

QPainterPath LineItem::shape() const
{
	QPainterPath path;
	if (m_points.size() < 2)
	{
		return path;
	}
	path.moveTo(m_points[0]);
	for (int i = 1; i < m_points.size(); ++i)
	{
		path.lineTo(m_points[i]);
	}
	QPainterPathStroker stroker;
	qreal width = qMax(2, m_width + 2);
	stroker.setWidth(width);
	return stroker.createStroke(path);
}

void LineItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);
	if (m_points.size() < 2)
	{
		return;
	}
	QPen pen(m_color);
	int lineWidth = m_width;
	if (m_highlighted)
	{
		lineWidth = m_width * 3;
		if (lineWidth < 2)
		{
			lineWidth = 2;
		}
	}
	else if (lineWidth < 1)
	{
		lineWidth = 1;
	}
	pen.setWidth(lineWidth);
	pen.setStyle(Qt::SolidLine);
	painter->setPen(pen);
	for (int i = 1; i < m_points.size(); ++i)
	{
		painter->drawLine(m_points[i - 1], m_points[i]);
	}
	if (!m_arrowVisible)
	{
		return;
	}
	QPointF startVec = m_points[1] - m_points[0];
	QPointF endVec = m_points[m_points.size() - 2] - m_points[m_points.size() - 1];
	if (qFuzzyIsNull(startVec.x()) && qFuzzyIsNull(startVec.y()) &&
		qFuzzyIsNull(endVec.x()) && qFuzzyIsNull(endVec.y()))
	{
		return;
	}
	QPointF startArrowPoint = direct_build_arrow_point_on_line(m_points[0], startVec, 0, ARROW_LEN, 0);
	QPointF endArrowPoint = direct_build_arrow_point_on_line(m_points[m_points.size() - 1], endVec, 0, ARROW_LEN, 0);
	qreal doubleArrowGap = ARROW_LEN * DIRECT_DOUBLE_ARROW_GAP_RATIO;
	QPointF startOutArrowPoint = startArrowPoint;
	QPointF startInArrowPoint = startArrowPoint;
	QPointF endOutArrowPoint = endArrowPoint;
	QPointF endInArrowPoint = endArrowPoint;
	if ((m_startArrowState & Arrow_InOut) == Arrow_InOut)
	{
		startOutArrowPoint = direct_offset_point_by_vec(startArrowPoint, startVec, doubleArrowGap * 0.5);
		startInArrowPoint = direct_offset_point_by_vec(startArrowPoint, startVec, -doubleArrowGap * 0.5);
	}
	if ((m_endArrowState & Arrow_InOut) == Arrow_InOut)
	{
		endOutArrowPoint = direct_offset_point_by_vec(endArrowPoint, endVec, doubleArrowGap * 0.5);
		endInArrowPoint = direct_offset_point_by_vec(endArrowPoint, endVec, -doubleArrowGap * 0.5);
	}
	if ((m_startArrowState & Arrow_Out) && (!qFuzzyIsNull(startVec.x()) || !qFuzzyIsNull(startVec.y())))
	{
		direct_draw_arrow(painter, startOutArrowPoint, direct_angle_by_vec(startVec), m_arrowColor, ARROW_LEN);
	}
	if ((m_startArrowState & Arrow_In) && (!qFuzzyIsNull(startVec.x()) || !qFuzzyIsNull(startVec.y())))
	{
		direct_draw_arrow(painter, startInArrowPoint, direct_angle_by_vec(-startVec), m_arrowColor, ARROW_LEN);
	}
	if ((m_endArrowState & Arrow_Out) && (!qFuzzyIsNull(endVec.x()) || !qFuzzyIsNull(endVec.y())))
	{
		direct_draw_arrow(painter, endOutArrowPoint, direct_angle_by_vec(endVec), m_arrowColor, ARROW_LEN);
	}
	if ((m_endArrowState & Arrow_In) && (!qFuzzyIsNull(endVec.x()) || !qFuzzyIsNull(endVec.y())))
	{
		direct_draw_arrow(painter, endInArrowPoint, direct_angle_by_vec(-endVec), m_arrowColor, ARROW_LEN);
	}
}

void LineItem::setArrowVisible(bool visible)
{
	m_arrowVisible = visible;
	if (!visible)
	{
		m_startArrowState = Arrow_None;
		m_endArrowState = Arrow_None;
	}
	else if (m_startArrowState == Arrow_None && m_endArrowState == Arrow_None)
	{
		m_endArrowState = Arrow_In;
	}
	update();
}

void LineItem::setArrowColor(const QColor& color)
{
	m_arrowColor = color;
	update();
}

void LineItem::setArrowState(quint8 startArrowState, quint8 endArrowState)
{
	m_startArrowState = startArrowState;
	m_endArrowState = endArrowState;
	m_arrowVisible = startArrowState != Arrow_None || endArrowState != Arrow_None;
	update();
}

void LineItem::setDirectionIedNames(const QString& srcIedName, const QString& destIedName)
{
	m_srcIedName = srcIedName;
	m_destIedName = destIedName;
}

void LineItem::setRelatedVirtualCodes(const QSet<quint64>& circuitCodeSet)
{
	m_relatedVirtualCodeSet = circuitCodeSet;
}

void LineItem::setPreferredMainIedName(const QString& iedName)
{
	m_preferredMainIedName = iedName;
}

QString LineItem::srcIedName() const
{
	return m_srcIedName;
}

QString LineItem::destIedName() const
{
	return m_destIedName;
}

QString LineItem::preferredMainIedName() const
{
	return m_preferredMainIedName;
}

const QSet<quint64>& LineItem::relatedVirtualCodes() const
{
	return m_relatedVirtualCodeSet;
}

void LineItem::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
	Q_UNUSED(event);
	if (QApplication::mouseButtons() & Qt::LeftButton)
	{
		return;
	}
	m_highlighted = true;
	update();
}

void LineItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
	Q_UNUSED(event);
	if (QApplication::mouseButtons() & Qt::LeftButton)
	{
		return;
	}
	m_highlighted = false;
	update();
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
	if (m_blinking && !m_blinkOn)
	{
		return;
	}
	QPen pen(m_lineColor);
	int w = m_lineWidth;
	if (m_highlighted)
	{
		w = m_lineWidth * 3;
		if (w < 2) w = 2;
	}
	pen.setWidth(w);
	painter->setPen(pen);
	painter->drawLine(m_startPoint, m_endPoint);
	if (m_arrowColor.isValid())
	{
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
	if (m_valueVisible)
	{
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
	if (QApplication::mouseButtons() & Qt::LeftButton)
	{
		return;
	}
	setHighlighted(true);
}

void DirectVirtualLineItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
	Q_UNUSED(event);
	if (QApplication::mouseButtons() & Qt::LeftButton)
	{
		return;
	}
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
	, m_startArrowState(Arrow_None)
	, m_endArrowState(Arrow_None)
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
	for (int i = 1; i < m_points.size(); ++i)
	{
		const QPointF& point = m_points[i];
		if (point.x() < minX) minX = point.x();
		if (point.x() > maxX) maxX = point.x();
		if (point.y() < minY) minY = point.y();
		if (point.y() > maxY) maxY = point.y();
	}
	QFont font = QApplication::font();
	font.setPointSize(16);
	QFontMetrics metrics(font);
	int maxTextWidth = qMax(metrics.width(m_startPort), metrics.width(m_endPort));
	qreal padX = qMax((qreal)(qMax(1, m_lineWidth * 3) + CONN_R * 2 + 8), (qreal)(maxTextWidth / 2 + DIRECT_BOUND_MARGIN));
	qreal padY = qMax((qreal)(qMax(1, m_lineWidth * 3) + CONN_R * 2 + 8),
		(qreal)(metrics.height() + CONN_R * 2 + ARROW_LEN + DIRECT_PORT_TEXT_OFFSET + DIRECT_ARROW_OFFSET + DIRECT_BOUND_MARGIN));
	return QRectF(QPointF(minX - padX, minY - padY), QPointF(maxX + padX, maxY + padY));
}

void DirectOpticalLineItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);
	if (m_points.size() < 2)
	{
		return;
	}
	int w = m_lineWidth;
	if (m_highlighted)
	{
		w = m_lineWidth * 3;
		if (w < 2) w = 2;
	}
	QPen pen(m_lineColor);
	pen.setWidth(w);
	painter->setPen(pen);
	painter->setBrush(Qt::NoBrush);
	QPolygonF poly(m_points);
	painter->drawPolyline(poly);
	direct_draw_conn_circle(painter, m_startPoint, CONN_R, m_startAtTop);
	direct_draw_conn_circle(painter, m_endPoint, CONN_R, m_endAtTop);
	QPointF startOutwardVec;
	QPointF endOutwardVec;
	if (m_points.size() >= 2)
	{
		startOutwardVec = m_points[1] - m_points[0];
		endOutwardVec = m_points[m_points.size() - 2] - m_points[m_points.size() - 1];
	}
	QPointF startArrowPoint = direct_build_arrow_point_on_line(m_points[0], startOutwardVec, CONN_R, ARROW_LEN, DIRECT_ARROW_OFFSET);
	QPointF endArrowPoint = direct_build_arrow_point_on_line(m_points[m_points.size() - 1], endOutwardVec, CONN_R, ARROW_LEN, DIRECT_ARROW_OFFSET);
	qreal doubleArrowGap = ARROW_LEN * DIRECT_DOUBLE_ARROW_GAP_RATIO;
	QPointF startOutArrowPoint = startArrowPoint;
	QPointF startInArrowPoint = startArrowPoint;
	QPointF endOutArrowPoint = endArrowPoint;
	QPointF endInArrowPoint = endArrowPoint;
	if ((m_startArrowState & Arrow_InOut) == Arrow_InOut)
	{
		startOutArrowPoint = direct_offset_point_by_vec(startArrowPoint, startOutwardVec, doubleArrowGap * 0.5);
		startInArrowPoint = direct_offset_point_by_vec(startArrowPoint, startOutwardVec, -doubleArrowGap * 0.5);
	}
	if ((m_endArrowState & Arrow_InOut) == Arrow_InOut)
	{
		endOutArrowPoint = direct_offset_point_by_vec(endArrowPoint, endOutwardVec, doubleArrowGap * 0.5);
		endInArrowPoint = direct_offset_point_by_vec(endArrowPoint, endOutwardVec, -doubleArrowGap * 0.5);
	}
	if (m_startArrowState & Arrow_Out)
	{
		double startArrowAngle = direct_angle_by_vec(startOutwardVec);
		direct_draw_arrow(painter, startOutArrowPoint, startArrowAngle, m_lineColor, ARROW_LEN);
	}
	if (m_startArrowState & Arrow_In)
	{
		double startArrowAngle = direct_angle_by_vec(-startOutwardVec);
		direct_draw_arrow(painter, startInArrowPoint, startArrowAngle, m_lineColor, ARROW_LEN);
	}
	if (m_endArrowState & Arrow_Out)
	{
		double endArrowAngle = direct_angle_by_vec(endOutwardVec);
		direct_draw_arrow(painter, endOutArrowPoint, endArrowAngle, m_lineColor, ARROW_LEN);
	}
	if (m_endArrowState & Arrow_In)
	{
		double endArrowAngle = direct_angle_by_vec(-endOutwardVec);
		direct_draw_arrow(painter, endInArrowPoint, endArrowAngle, m_lineColor, ARROW_LEN);
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
	for (int i = 1; i < m_points.size(); ++i)
	{
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
	for (int i = 0; i < line.midPoints.size(); ++i)
	{
		m_points.append(QPointF(line.midPoints[i]));
	}
	m_points.append(QPointF(line.endPoint));
	m_startPoint = line.startPoint;
	m_endPoint = line.endPoint;
	m_startArrowState = Arrow_None;
	m_endArrowState = Arrow_None;
	m_lineCode = line.lineCode;
	m_startPort.clear();
	m_endPort.clear();
	m_startIsSwitch = false;
	m_endIsSwitch = false;
	m_startAtTop = true;
	m_endAtTop = false;
	m_underRectY = 0;
	if (line.pSrcRect && line.pDestRect && line.pOpticalCircuit)
	{
		bool startIsSrc = direct_is_point_attached_to_rect(line.startPoint, line.pSrcRect);
		bool endIsSrc = direct_is_point_attached_to_rect(line.endPoint, line.pSrcRect);
		bool useStartAsSrc = true;
		if (!startIsSrc && endIsSrc)
		{
			useStartAsSrc = false;
		}
		m_underRectY = line.pSrcRect->y > line.pDestRect->y ? line.pSrcRect->y : line.pDestRect->y;
		if (useStartAsSrc)
		{
			m_startArrowState = line.srcArrowState;
			m_endArrowState = line.destArrowState;
			m_startIsSwitch = line.pSrcRect->iedName.contains("SW");
			m_endIsSwitch = line.pDestRect->iedName.contains("SW");
			m_startPort = line.pOpticalCircuit->srcIedPort;
			m_endPort = line.pOpticalCircuit->destIedPort;
			m_startAtTop = direct_is_top_anchor(line.startPoint, line.pSrcRect);
			m_endAtTop = direct_is_top_anchor(line.endPoint, line.pDestRect);
		}
		else
		{
			m_startArrowState = line.destArrowState;
			m_endArrowState = line.srcArrowState;
			m_startIsSwitch = line.pDestRect->iedName.contains("SW");
			m_endIsSwitch = line.pSrcRect->iedName.contains("SW");
			m_startPort = line.pOpticalCircuit->destIedPort;
			m_endPort = line.pOpticalCircuit->srcIedPort;
			m_startAtTop = direct_is_top_anchor(line.startPoint, line.pDestRect);
			m_endAtTop = direct_is_top_anchor(line.endPoint, line.pSrcRect);
		}
	}
	update();
}

