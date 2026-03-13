#include "directlineitems.h"
#include "directlinehelpers.h"
#include "svgmodel.h"
#include "SvgUtils.h"
#include <QApplication>
#include <QFontMetrics>
#include <QPainter>
#include <QPixmap>
#include <qmath.h>

namespace
{
	enum DirectLineConfig
	{
		DIRECT_PORT_TEXT_OFFSET = 12,
		DIRECT_ARROW_OFFSET = 10,
		DIRECT_BOUND_MARGIN = 12
	};

	enum WholeLabelConfig
	{
		DIRECT_WHOLE_LABEL_FONT_SIZE = 8,
		DIRECT_WHOLE_SIDE_LABEL_TEXT_WIDTH = 180,
		DIRECT_WHOLE_SIDE_LABEL_SAFE_DISTANCE = 120,
		DIRECT_WHOLE_SIDE_LABEL_VERTICAL_PADDING = 3,
		DIRECT_WHOLE_SIDE_LABEL_LINE_GAP = 2,
		DIRECT_WHOLE_SIDE_LABEL_BRACE_GAP = 6,
		DIRECT_WHOLE_SIDE_LABEL_BOTTOM_PADDING = 1
	};

	void direct_draw_virtual_body(QPainter* painter, const QPointF& startPoint, const QPointF& endPoint, bool hasMiddleGap, qreal gapStartX, qreal gapEndX)
	{
		if (!hasMiddleGap || qAbs(startPoint.y() - endPoint.y()) > 0.001)
		{
			painter->drawLine(startPoint, endPoint);
			return;
		}
		qreal leftGapX = qMin(gapStartX, gapEndX);
		qreal rightGapX = qMax(gapStartX, gapEndX);
		if (startPoint.x() <= endPoint.x())
		{
			qreal leftEndX = qMin(leftGapX, endPoint.x());
			qreal rightStartX = qMax(rightGapX, startPoint.x());
			if (leftEndX > startPoint.x())
			{
				painter->drawLine(startPoint, QPointF(leftEndX, startPoint.y()));
			}
			if (endPoint.x() > rightStartX)
			{
				painter->drawLine(QPointF(rightStartX, endPoint.y()), endPoint);
			}
		}
		else
		{
			qreal leftStartX = qMax(rightGapX, endPoint.x());
			qreal rightEndX = qMin(leftGapX, startPoint.x());
			if (startPoint.x() > leftStartX)
			{
				painter->drawLine(startPoint, QPointF(leftStartX, startPoint.y()));
			}
			if (rightEndX > endPoint.x())
			{
				painter->drawLine(QPointF(rightEndX, endPoint.y()), endPoint);
			}
		}
	}

	void direct_draw_whole_brace(QPainter* painter, const QRectF& braceRect, bool isLeftBrace)
	{
		if (braceRect.isNull())
		{
			return;
		}
		qreal leftX = braceRect.left();
		qreal rightX = braceRect.right();
		qreal topY = braceRect.top();
		qreal bottomY = braceRect.bottom();
		if (isLeftBrace)
		{
			painter->drawLine(QPointF(rightX, topY), QPointF(rightX, bottomY));
			painter->drawLine(QPointF(leftX, topY), QPointF(rightX, topY));
			painter->drawLine(QPointF(leftX, bottomY), QPointF(rightX, bottomY));
		}
		else
		{
			painter->drawLine(QPointF(leftX, topY), QPointF(leftX, bottomY));
			painter->drawLine(QPointF(leftX, topY), QPointF(rightX, topY));
			painter->drawLine(QPointF(leftX, bottomY), QPointF(rightX, bottomY));
		}
	}

	QVector<QPointF> direct_build_two_point_vector(const QPointF& startPoint, const QPointF& endPoint)
	{
		QVector<QPointF> points;
		points << startPoint << endPoint;
		return points;
	}
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
	qreal pad = qMax(1, m_width) + ARROW_LEN + 2;
	return directlinehelpers::BuildPolylineBoundingRect(m_points, pad, pad);
}

QPainterPath LineItem::shape() const
{
	return directlinehelpers::BuildPolylineShape(m_points, qMax(2, m_width + 2));
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
	pen.setWidth(directlinehelpers::BuildPaintLineWidth(m_width, m_highlighted));
	pen.setStyle(Qt::SolidLine);
	painter->setPen(pen);
	directlinehelpers::DrawPolyline(painter, m_points);
	if (!m_arrowVisible)
	{
		return;
	}
	directlinehelpers::DrawPolylineArrows(painter, m_points, 0, 0, m_startArrowState, m_endArrowState, m_arrowColor, ARROW_LEN);
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
	, m_lineStyle(Qt::SolidLine)
	, m_arrowVisible(true)
	, m_hasMiddleGap(false)
	, m_gapStartX(0)
	, m_gapEndX(0)
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

int DirectVirtualLineItem::LabelFontPointSize()
{
	return DIRECT_WHOLE_LABEL_FONT_SIZE;
}

qreal DirectVirtualLineItem::SideLabelTextWidth()
{
	return DIRECT_WHOLE_SIDE_LABEL_TEXT_WIDTH;
}

qreal DirectVirtualLineItem::SideLabelMaxWidth()
{
	return SideLabelTextWidth();
}

qreal DirectVirtualLineItem::SideLabelSafeDistance()
{
	return DIRECT_WHOLE_SIDE_LABEL_SAFE_DISTANCE;
}

qreal DirectVirtualLineItem::CenterArrowMinLength()
{
	return WHOLE_CENTER_ARROW_MIN_LENGTH;
}

qreal DirectVirtualLineItem::SideLabelVerticalPadding()
{
	return DIRECT_WHOLE_SIDE_LABEL_VERTICAL_PADDING;
}

qreal DirectVirtualLineItem::SideLabelLineGap()
{
	return DIRECT_WHOLE_SIDE_LABEL_LINE_GAP;
}

qreal DirectVirtualLineItem::SideLabelBraceGap()
{
	return DIRECT_WHOLE_SIDE_LABEL_BRACE_GAP;
}

qreal DirectVirtualLineItem::SideLabelBottomPadding()
{
	return DIRECT_WHOLE_SIDE_LABEL_BOTTOM_PADDING;
}

QRectF DirectVirtualLineItem::boundingRect() const
{
	QRectF rect;
	rect |= QRectF(m_startPoint, m_endPoint).normalized();
	if (!m_startValRect.isNull())
	{
		rect |= m_startValRect;
	}
	if (!m_endValRect.isNull())
	{
		rect |= m_endValRect;
	}
	if (!m_descRect.isNull())
	{
		rect |= m_descRect;
	}
	if (!m_leftLabelRect.isNull())
	{
		rect |= m_leftLabelRect;
	}
	if (!m_rightLabelRect.isNull())
	{
		rect |= m_rightLabelRect;
	}
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
	pen.setWidth(directlinehelpers::BuildPaintLineWidth(m_lineWidth, m_highlighted));
	pen.setStyle(m_lineStyle);
	painter->setPen(pen);
	direct_draw_virtual_body(painter, m_startPoint, m_endPoint, m_hasMiddleGap, m_gapStartX, m_gapEndX);
	if (m_arrowVisible && m_arrowColor.isValid())
	{
		painter->save();
		directlinehelpers::DrawArrow(painter, m_endPoint, directlinehelpers::AngleByVec(m_endPoint - m_startPoint), m_arrowColor, ARROW_LEN);
		painter->restore();
	}
	QColor iconColor = m_lineColor;
	painter->setBrush(Qt::NoBrush);
	if (m_virtualType == SV)
	{
		utils::drawSvIcon(painter, m_startIconPt.toPoint(), ICON_LENGTH, iconColor);
	}
	else
	{
		utils::drawGseIcon(painter, m_startIconPt.toPoint(), ICON_LENGTH, iconColor);
	}
	if (m_virtualType == SV)
	{
		utils::drawSvIcon(painter, m_endIconPt.toPoint(), ICON_LENGTH, iconColor);
	}
	else
	{
		utils::drawGseIcon(painter, m_endIconPt.toPoint(), ICON_LENGTH, iconColor);
	}
	QFont font = painter->font();
	font.setPointSize(LabelFontPointSize());
	painter->setFont(font);
	painter->setPen(Qt::white);
	if (m_valueVisible)
	{
		int valueVerticalAlign = Qt::AlignVCenter;
		if (!m_startValRect.isNull())
		{
			bool startValueOnLeft = m_startValRect.center().x() <= m_startIconPt.x();
			int startValueAlign = startValueOnLeft ? (valueVerticalAlign | Qt::AlignRight) : (valueVerticalAlign | Qt::AlignLeft);
			painter->drawText(m_startValRect, startValueAlign, m_outValStr);
		}
		if (!m_endValRect.isNull())
		{
			bool endValueOnLeft = m_endValRect.center().x() <= m_endIconPt.x();
			int endValueAlign = endValueOnLeft ? (valueVerticalAlign | Qt::AlignRight) : (valueVerticalAlign | Qt::AlignLeft);
			painter->drawText(m_endValRect, endValueAlign, m_inValStr);
		}
	}
	if (!m_leftLabelRect.isNull() && !m_leftLabelStr.isEmpty())
	{
		QRectF leftDrawRect = m_leftLabelRect.adjusted(0.0, 0.0, 0.0, -SideLabelBottomPadding());
		painter->drawText(leftDrawRect, Qt::AlignBottom | Qt::AlignLeft, m_leftLabelStr);
	}
	if (!m_rightLabelRect.isNull() && !m_rightLabelStr.isEmpty())
	{
		QRectF rightDrawRect = m_rightLabelRect.adjusted(0.0, 0.0, 0.0, -SideLabelBottomPadding());
		painter->drawText(rightDrawRect, Qt::AlignBottom | Qt::AlignLeft, m_rightLabelStr);
	}
	if (m_leftLabelStr.isEmpty() && m_rightLabelStr.isEmpty() && !m_descRect.isNull())
	{
		painter->drawText(m_descRect, Qt::AlignVCenter | Qt::AlignLeft, m_descStr);
	}
}

QPainterPath DirectVirtualLineItem::shape() const
{
	return directlinehelpers::BuildPolylineShape(direct_build_two_point_vector(m_startPoint, m_endPoint), qMax(2, m_lineWidth + 4));
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
	m_leftLabelRect = QRectF();
	m_rightLabelRect = QRectF();
	m_leftLabelStr.clear();
	m_rightLabelStr.clear();
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

void DirectVirtualLineItem::setArrowVisible(bool visible)
{
	m_arrowVisible = visible;
	update();
}

void DirectVirtualLineItem::setLineStyle(Qt::PenStyle style)
{
	m_lineStyle = style;
	update();
}

void DirectVirtualLineItem::setMiddleGap(qreal gapStartX, qreal gapEndX)
{
	m_hasMiddleGap = true;
	m_gapStartX = gapStartX;
	m_gapEndX = gapEndX;
	update();
}

void DirectVirtualLineItem::clearMiddleGap()
{
	m_hasMiddleGap = false;
	m_gapStartX = 0;
	m_gapEndX = 0;
	update();
}

void DirectVirtualLineItem::setSideLabels(const QString& leftText, const QRectF& leftRect, const QString& rightText, const QRectF& rightRect)
{
	m_leftLabelStr = leftText;
	m_leftLabelRect = leftRect;
	m_rightLabelStr = rightText;
	m_rightLabelRect = rightRect;
	update();
}

void DirectVirtualLineItem::clearSideLabels()
{
	m_leftLabelStr.clear();
	m_rightLabelStr.clear();
	m_leftLabelRect = QRectF();
	m_rightLabelRect = QRectF();
	update();
}

void DirectVirtualLineItem::setValueVisible(bool visible)
{
	m_valueVisible = visible;
	update();
}

DirectWholeGroupItem::DirectWholeGroupItem(QGraphicsItem* parent)
	: m_braceColor(Qt::green)
	, m_centerLineColor(Qt::green)
	, m_braceBlinking(false)
	, m_braceBlinkOn(true)
	, m_centerLineBlinking(false)
	, m_centerLineBlinkOn(true)
	, m_hasSwitchIcon(false)
{
	Q_UNUSED(parent);
	m_itemType = CircuitLine;
}

DirectWholeGroupItem::~DirectWholeGroupItem()
{
}

QRectF DirectWholeGroupItem::boundingRect() const
{
	QRectF rect = m_leftBraceRect.united(m_rightBraceRect);
	rect |= QRectF(m_centerArrowLine.p1(), m_centerArrowLine.p2()).normalized();
	if (!m_switchIconRect.isNull())
	{
		rect |= m_switchIconRect;
	}
	if (!m_leftPortRect.isNull())
	{
		rect |= m_leftPortRect;
	}
	if (!m_rightPortRect.isNull())
	{
		rect |= m_rightPortRect;
	}
	return rect.adjusted(-8.0, -8.0, 8.0, 8.0);
}

void DirectWholeGroupItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);
	painter->setBrush(Qt::NoBrush);
	if (!m_braceBlinking || m_braceBlinkOn)
	{
		painter->save();
		QPen bracePen(m_braceColor);
		bracePen.setStyle(Qt::DashLine);
		bracePen.setWidth(0);
		bracePen.setCapStyle(Qt::FlatCap);
		bracePen.setJoinStyle(Qt::MiterJoin);
		bracePen.setCosmetic(true);
		painter->setRenderHint(QPainter::Antialiasing, false);
		painter->setPen(bracePen);
		direct_draw_whole_brace(painter, m_leftBraceRect, true);
		direct_draw_whole_brace(painter, m_rightBraceRect, false);
		painter->restore();
	}
	QPointF arrowStartPoint = m_centerArrowLine.p1();
	QPointF arrowEndPoint = m_centerArrowLine.p2();
	if (!m_centerLineBlinking || m_centerLineBlinkOn)
	{
		QPen arrowPen(m_centerLineColor);
		arrowPen.setWidth(4);
		painter->setPen(arrowPen);
		if (m_hasSwitchIcon && !m_switchIconRect.isNull())
		{
			if (arrowStartPoint.x() <= arrowEndPoint.x())
			{
				painter->drawLine(arrowStartPoint, QPointF(m_switchIconRect.left() - 4.0, arrowStartPoint.y()));
				painter->drawLine(QPointF(m_switchIconRect.right() + 4.0, arrowEndPoint.y()), arrowEndPoint);
			}
			else
			{
				painter->drawLine(arrowStartPoint, QPointF(m_switchIconRect.right() + 4.0, arrowStartPoint.y()));
				painter->drawLine(QPointF(m_switchIconRect.left() - 4.0, arrowEndPoint.y()), arrowEndPoint);
			}
		}
		else
		{
			painter->drawLine(arrowStartPoint, arrowEndPoint);
		}
		directlinehelpers::DrawArrow(painter, arrowEndPoint, directlinehelpers::AngleByVec(arrowEndPoint - arrowStartPoint), m_centerLineColor, ARROW_LEN + 2);
	}
	if (m_hasSwitchIcon && !m_switchIconRect.isNull())
	{
		QPixmap switchPixmap(QString::fromLatin1(":/MainWindow/virtual_real_switch.png"));
		if (!switchPixmap.isNull())
		{
			painter->drawPixmap(m_switchIconRect.toRect(), switchPixmap);
		}
		else
		{
			QPen iconPen(m_centerLineColor);
			painter->setPen(iconPen);
			painter->drawRect(m_switchIconRect);
		}
	}
	if (!m_leftPortText.isEmpty() || !m_rightPortText.isEmpty())
	{
		QFont font = painter->font();
		font.setPointSize(WHOLE_PORT_TEXT_FONT_SIZE);
		painter->setFont(font);
		painter->setPen(Qt::white);
		if (!m_leftPortText.isEmpty() && !m_leftPortRect.isNull())
		{
			painter->drawText(m_leftPortRect, Qt::AlignHCenter | Qt::AlignVCenter, m_leftPortText);
		}
		if (!m_rightPortText.isEmpty() && !m_rightPortRect.isNull())
		{
			painter->drawText(m_rightPortRect, Qt::AlignHCenter | Qt::AlignVCenter, m_rightPortText);
		}
	}
}

void DirectWholeGroupItem::setFromWholeGroupDecor(const WholeGroupDecor& groupDecor)
{
	prepareGeometryChange();
	m_leftBraceRect = groupDecor.leftBraceRect;
	m_rightBraceRect = groupDecor.rightBraceRect;
	m_centerArrowLine = groupDecor.centerArrowLine;
	m_switchIconRect = groupDecor.switchIconRect;
	m_hasSwitchIcon = groupDecor.hasSwitchIcon;
	m_switchIedName = groupDecor.switchIedName;
	m_leftPortText = groupDecor.leftPortText;
	m_rightPortText = groupDecor.rightPortText;
	m_leftPortRect = groupDecor.leftPortRect;
	m_rightPortRect = groupDecor.rightPortRect;
	update();
}

void DirectWholeGroupItem::setLineColor(const QColor& color)
{
	m_braceColor = color;
	m_centerLineColor = color;
	update();
}

void DirectWholeGroupItem::setBraceColor(const QColor& color)
{
	m_braceColor = color;
	update();
}

void DirectWholeGroupItem::setCenterLineColor(const QColor& color)
{
	m_centerLineColor = color;
	update();
}

void DirectWholeGroupItem::setBraceBlinking(bool blinking)
{
	m_braceBlinking = blinking;
	update();
}

void DirectWholeGroupItem::setBraceBlinkOn(bool on)
{
	m_braceBlinkOn = on;
	update();
}

void DirectWholeGroupItem::setCenterLineBlinking(bool blinking)
{
	m_centerLineBlinking = blinking;
	update();
}

void DirectWholeGroupItem::setCenterLineBlinkOn(bool on)
{
	m_centerLineBlinkOn = on;
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
	QFont font = QApplication::font();
	font.setPointSize(16);
	QFontMetrics metrics(font);
	int maxTextWidth = qMax(metrics.width(m_startPort), metrics.width(m_endPort));
	qreal padX = qMax((qreal)(qMax(1, m_lineWidth * 3) + CONN_R * 2 + 8), (qreal)(maxTextWidth / 2 + DIRECT_BOUND_MARGIN));
	qreal padY = qMax((qreal)(qMax(1, m_lineWidth * 3) + CONN_R * 2 + 8),
		(qreal)(metrics.height() + CONN_R * 2 + ARROW_LEN + DIRECT_PORT_TEXT_OFFSET + DIRECT_ARROW_OFFSET + DIRECT_BOUND_MARGIN));
	return directlinehelpers::BuildPolylineBoundingRect(m_points, padX, padY);
}

void DirectOpticalLineItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
	Q_UNUSED(option);
	Q_UNUSED(widget);
	if (m_points.size() < 2)
	{
		return;
	}
	QPen pen(m_lineColor);
	pen.setWidth(directlinehelpers::BuildPaintLineWidth(m_lineWidth, m_highlighted));
	painter->setPen(pen);
	painter->setBrush(Qt::NoBrush);
	directlinehelpers::DrawPolyline(painter, m_points);
	directlinehelpers::DrawConnCircle(painter, m_startPoint, CONN_R, m_startAtTop);
	directlinehelpers::DrawConnCircle(painter, m_endPoint, CONN_R, m_endAtTop);
	directlinehelpers::DrawPolylineArrows(painter, m_points, CONN_R, DIRECT_ARROW_OFFSET, m_startArrowState, m_endArrowState, m_lineColor, ARROW_LEN);
	directlinehelpers::DrawPortText(painter, m_startPoint, m_endPoint, m_startAtTop, m_endAtTop,
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
	return directlinehelpers::BuildPolylineShape(m_points, qMax(2, m_lineWidth + 2));
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
		bool startIsSrc = directlinehelpers::IsPointAttachedToRect(line.startPoint, line.pSrcRect);
		bool endIsSrc = directlinehelpers::IsPointAttachedToRect(line.endPoint, line.pSrcRect);
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
			m_startAtTop = directlinehelpers::IsTopAnchor(line.startPoint, line.pSrcRect);
			m_endAtTop = directlinehelpers::IsTopAnchor(line.endPoint, line.pDestRect);
		}
		else
		{
			m_startArrowState = line.destArrowState;
			m_endArrowState = line.srcArrowState;
			m_startIsSwitch = line.pDestRect->iedName.contains("SW");
			m_endIsSwitch = line.pSrcRect->iedName.contains("SW");
			m_startPort = line.pOpticalCircuit->destIedPort;
			m_endPort = line.pOpticalCircuit->srcIedPort;
			m_startAtTop = directlinehelpers::IsTopAnchor(line.startPoint, line.pDestRect);
			m_endAtTop = directlinehelpers::IsTopAnchor(line.endPoint, line.pSrcRect);
		}
	}
	update();
}
