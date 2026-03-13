#include "directnodeitems.h"
#include "svgmodel.h"
#include "SvgUtils.h"
#include <QApplication>
#include <QFontMetrics>
#include <QPainter>
#include <QVector>
#include <QStringList>
#include <qmath.h>

namespace
{
	enum LogicFrameConfig
	{
		DIRECT_LOGIC_FRAME_TITLE_FONT_SIZE = 8
	};

	enum DirectMaintainPlateConfig
	{
		DIRECT_MAINT_PLATE_CIRCLE_RADIUS = 5,
		DIRECT_MAINT_PLATE_ICON_SPAN = 36,
		DIRECT_MAINT_PLATE_BODY_WIDTH = 26,
		DIRECT_MAINT_PLATE_BODY_HEIGHT = 10,
		DIRECT_MAINT_PLATE_TEXT_HEIGHT = 18,
		DIRECT_MAINT_PLATE_TEXT_FONT_SIZE = 8,
		DIRECT_MAINT_PLATE_TEXT_ICON_GAP = 30,
		DIRECT_MAINT_PLATE_BOTTOM_MARGIN = 6
	};

	QString direct_build_maint_plate_default_text()
	{
		return QString::fromLocal8Bit("¼ìÐÞÑ¹°å");
	}
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
	if (m_rect.isNull())
	{
		return QRectF();
	}
	if (m_extendHeight > 0 && !m_isSwitcher)
	{
		qreal width = m_rect.width() + 2 * m_innerGap;
		qreal height = m_rect.height() + m_extendHeight + 2 * m_innerGap;
		QRectF outerRect(m_rect.x() - m_innerGap, m_rect.y() - m_innerGap, width, height);
		return outerRect.adjusted(-2.0, -2.0, 2.0, 2.0);
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
	QColor underColor = m_underColor == 0 ? utils::ColorHelper::Color(utils::ColorHelper::ied_underground) : utils::ColorHelper::Color(m_underColor);
	brush.setColor(underColor);
	if (!m_isSwitcher && m_extendHeight > 0)
	{
		brush.setColor(utils::ColorHelper::Color(utils::ColorHelper::ied_underground));
		pen.setColor(utils::ColorHelper::Color(utils::ColorHelper::ied_border));
		pen.setStyle(Qt::DashLine);
		painter->setPen(pen);
		painter->setBrush(brush);
		QRectF outerRect(m_rect.x() - m_innerGap, m_rect.y() - m_innerGap,
			m_rect.width() + 2 * m_innerGap, m_rect.height() + m_extendHeight + 2 * m_innerGap);
		painter->fillRect(outerRect, brush);
		painter->setBrush(Qt::NoBrush);
		painter->drawRect(outerRect);
	}
	pen.setStyle(Qt::SolidLine);
	pen.setColor(utils::ColorHelper::Color(m_borderColor));
	painter->setPen(pen);
	brush.setColor(underColor);
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
	if (maxWidth < 10)
	{
		maxWidth = 10;
	}
	QStringList lines;
	int lineHeight = 0;
	int totalTextHeight = 0;
	int fontSize = pointSize;
	while (fontSize >= 8)
	{
		QFont font = painter->font();
		font.setPointSize(fontSize);
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
		if (!currentLine.isEmpty())
		{
			lines.append(currentLine);
		}
		lineHeight = metrics.height();
		totalTextHeight = lines.size() * lineHeight;
		if (totalTextHeight <= int(rect.height()) - 2)
		{
			break;
		}
		--fontSize;
	}
	int textY = int(rect.y()) + (int(rect.height()) - totalTextHeight) / 2 + lineHeight - painter->fontMetrics().descent();
	for (int lineIndex = 0; lineIndex < lines.size(); ++lineIndex)
	{
		int textWidth = painter->fontMetrics().width(lines[lineIndex]);
		int textX = int(rect.x()) + (int(rect.width()) - textWidth) / 2;
		painter->drawText(textX, textY, lines[lineIndex]);
		textY += lineHeight;
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

int LogicFrameItem::TitleFontPointSize()
{
	return DIRECT_LOGIC_FRAME_TITLE_FONT_SIZE;
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
	font.setPointSize(LogicFrameItem::TitleFontPointSize());
	QFontMetrics metrics(font);
	qreal descWidth = 150.0;
	qreal descHeight = metrics.height() * 2 + 30;
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
	font.setPointSize(LogicFrameItem::TitleFontPointSize());
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
	if (m_rect.isNull())
	{
		return QRectF();
	}
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
	QPen pen;
	pen.setColor(QColor(255, 255, 255));
	pen.setWidth(2);
	pen.setStyle(Qt::DashDotLine);
	QVector<qreal> dashPattern;
	dashPattern << 3 << 3;
	pen.setDashPattern(dashPattern);
	painter->setPen(pen);
	painter->setBrush(QColor(0x23, 0x3f, 0x4f));
	painter->drawRect(m_rect);
	QColor circleColor = m_closed ? QColor(Qt::green) : QColor(Qt::red);
	QPen circlePen(circleColor);
	circlePen.setWidth(2);
	painter->setPen(circlePen);
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
	qreal centerX = m_rect.x() + m_rect.width() * 0.5 - PLATE_WIDTH / 2.0 + PLATE_CIRCLE_RADIUS + PLATE_GAP;
	qreal centerY = m_rect.y() + m_rect.height() * 0.5;
	qreal distance = PLATE_WIDTH - PLATE_CIRCLE_RADIUS * 4;
	m_circle1 = QPointF(centerX, centerY);
	m_circle2 = QPointF(centerX + distance, centerY);
	m_lineTop = QLineF(QPointF(centerX, centerY - PLATE_CIRCLE_RADIUS), QPointF(centerX + distance, centerY - PLATE_CIRCLE_RADIUS));
	m_lineBottom = QLineF(QPointF(centerX + distance, centerY + PLATE_CIRCLE_RADIUS), QPointF(centerX, centerY + PLATE_CIRCLE_RADIUS));
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
	QPointF openPoint1 = m_leftCircleCenter + normal * bladeHalfWidth;
	QPointF openPoint2 = openPoint1 + direction * bladeLength;
	QPointF openPoint3 = m_leftCircleCenter - normal * bladeHalfWidth;
	QPointF openPoint4 = openPoint3 + direction * bladeLength;
	QRectF openRect(openPoint1, openPoint1);
	openRect = openRect.united(QRectF(openPoint2, openPoint2));
	openRect = openRect.united(QRectF(openPoint3, openPoint3));
	openRect = openRect.united(QRectF(openPoint4, openPoint4));
	m_iconRect = closedRect.united(openRect).adjusted(-3.0, -3.0, 3.0, 3.0);
}