#pragma once

#include "directitembase.h"
#include <QColor>
#include <QLineF>
#include <QPainterPath>
#include <QPoint>
#include <QPointF>
#include <QRectF>
#include <QSet>
#include <QString>
#include <QVector>
#include <QtGlobal>

class QGraphicsSceneHoverEvent;
struct VirtualCircuitLine;
struct OpticalCircuitLine;
struct WholeGroupDecor;

enum WholePortConfig
{
	WHOLE_PORT_TEXT_FONT_SIZE = 8,
	WHOLE_PORT_TEXT_LINE_GAP = 2,
	WHOLE_PORT_TEXT_SIDE_GAP = 8,
	WHOLE_PORT_TEXT_VERTICAL_PADDING = 2,
	WHOLE_CENTER_ARROW_MIN_LENGTH = 180
};

class DirectVirtualLineItem : public directItemBase
{
public:
	DirectVirtualLineItem(QGraphicsItem* parent = NULL);
	~DirectVirtualLineItem();
	static int LabelFontPointSize();
	static qreal SideLabelTextWidth();
	static qreal SideLabelMaxWidth();
	static qreal SideLabelSafeDistance();
	static qreal CenterArrowMinLength();
	static qreal SideLabelVerticalPadding();
	static qreal SideLabelLineGap();
	static qreal SideLabelBraceGap();
	static qreal SideLabelBottomPadding();
	QRectF boundingRect() const;
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
	QPainterPath shape() const;
	void setFromVirtualLine(const VirtualCircuitLine& line, int vtype);
	void setVirtualType(int vtype);
	void setValues(const QString& outVal, const QString& inVal);
	void setBlinking(bool blinking);
	void setBlinkOn(bool on);
	void setHighlighted(bool highlighted);
	void setCircuitCode(quint64 code);
	quint64 circuitCode() const;
	int virtualType() const;
	bool isBlinking() const;
	void setLineColor(const QColor& color);
	void setArrowColor(const QColor& color);
	void setArrowVisible(bool visible);
	void setLineStyle(Qt::PenStyle style);
	void setMiddleGap(qreal gapStartX, qreal gapEndX);
	void clearMiddleGap();
	void setSideLabels(const QString& leftText, const QRectF& leftRect, const QString& rightText, const QRectF& rightRect);
	void clearSideLabels();
	void setValueVisible(bool visible);

protected:
	void hoverEnterEvent(QGraphicsSceneHoverEvent* event);
	void hoverLeaveEvent(QGraphicsSceneHoverEvent* event);

private:
	QPointF m_startPoint;
	QPointF m_endPoint;
	QPointF m_startIconPt;
	QPointF m_endIconPt;
	QRectF m_startValRect;
	QRectF m_endValRect;
	QRectF m_descRect;
	QRectF m_leftLabelRect;
	QRectF m_rightLabelRect;
	QString m_outValStr;
	QString m_inValStr;
	QString m_descStr;
	QString m_leftLabelStr;
	QString m_rightLabelStr;
	quint64 m_circuitCode;
	int m_virtualType;
	QColor m_lineColor;
	QColor m_arrowColor;
	int m_lineWidth;
	Qt::PenStyle m_lineStyle;
	bool m_arrowVisible;
	bool m_hasMiddleGap;
	qreal m_gapStartX;
	qreal m_gapEndX;
	bool m_valueVisible;
	bool m_blinking;
	bool m_blinkOn;
	bool m_highlighted;
};

class DirectWholeGroupItem : public directItemBase
{
public:
	DirectWholeGroupItem(QGraphicsItem* parent = NULL);
	~DirectWholeGroupItem();
	QRectF boundingRect() const;
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
	void setFromWholeGroupDecor(const WholeGroupDecor& groupDecor);
	void setLineColor(const QColor& color);
	void setBraceColor(const QColor& color);
	void setCenterLineColor(const QColor& color);
	void setBraceBlinking(bool blinking);
	void setBraceBlinkOn(bool on);
	void setCenterLineBlinking(bool blinking);
	void setCenterLineBlinkOn(bool on);

private:
	QRectF m_leftBraceRect;
	QRectF m_rightBraceRect;
	QLineF m_centerArrowLine;
	QRectF m_switchIconRect;
	QRectF m_leftPortRect;
	QRectF m_rightPortRect;
	QColor m_braceColor;
	QColor m_centerLineColor;
	bool m_braceBlinking;
	bool m_braceBlinkOn;
	bool m_centerLineBlinking;
	bool m_centerLineBlinkOn;
	bool m_hasSwitchIcon;
	QString m_switchIedName;
	QString m_leftPortText;
	QString m_rightPortText;
};

class DirectOpticalLineItem : public directItemBase
{
public:
	DirectOpticalLineItem(QGraphicsItem* parent = NULL);
	~DirectOpticalLineItem();
	QRectF boundingRect() const;
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
	QPainterPath shape() const;
	void setFromOpticalLine(const OpticalCircuitLine& line);
	void setHighlighted(bool highlighted);
	void hoverEnterEvent(QGraphicsSceneHoverEvent* event);
	void hoverLeaveEvent(QGraphicsSceneHoverEvent* event);

private:
	QVector<QPointF> m_points;
	QPoint m_startPoint;
	QPoint m_endPoint;
	QString m_startPort;
	QString m_endPort;
	bool m_startIsSwitch;
	bool m_endIsSwitch;
	bool m_startAtTop;
	bool m_endAtTop;
	int m_underRectY;
	quint8 m_startArrowState;
	quint8 m_endArrowState;
	quint16 m_lineCode;
	QColor m_lineColor;
	int m_lineWidth;
	bool m_highlighted;
};

class DirectOpticalConnPointItem : public directItemBase
{
public:
	DirectOpticalConnPointItem(QGraphicsItem* parent = NULL);
	~DirectOpticalConnPointItem();
	QRectF boundingRect() const;
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
	QPainterPath shape() const;
	void setConnPoint(const QPoint& anchorPoint, bool isCircleUnderPt, DirectOpticalLineItem* pOpticalLineItem);

protected:
	void hoverEnterEvent(QGraphicsSceneHoverEvent* event);
	void hoverLeaveEvent(QGraphicsSceneHoverEvent* event);

private:
	QPoint m_anchorPoint;
	bool m_isCircleUnderPt;
	DirectOpticalLineItem* m_pOpticalLineItem;
	bool m_highlighted;
};

class LineItem : public directItemBase
{
public:
	LineItem(QGraphicsItem* parent = NULL);
	~LineItem();
	QRectF boundingRect() const;
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
	QPainterPath shape() const;
	void setPoints(const QVector<QPointF>& pts);
	void setColor(const QColor& color);
	void setWidth(int w);
	void setArrowVisible(bool visible);
	void setArrowColor(const QColor& color);
	void setArrowState(quint8 startArrowState, quint8 endArrowState);
	void setDirectionIedNames(const QString& srcIedName, const QString& destIedName);
	void setRelatedVirtualCodes(const QSet<quint64>& circuitCodeSet);
	void setPreferredMainIedName(const QString& iedName);
	QString srcIedName() const;
	QString destIedName() const;
	QString preferredMainIedName() const;
	const QSet<quint64>& relatedVirtualCodes() const;

protected:
	void hoverEnterEvent(QGraphicsSceneHoverEvent* event);
	void hoverLeaveEvent(QGraphicsSceneHoverEvent* event);

private:
	QVector<QPointF> m_points;
	QColor m_color;
	QColor m_arrowColor;
	int m_width;
	bool m_arrowVisible;
	quint8 m_startArrowState;
	quint8 m_endArrowState;
	bool m_highlighted;
	QString m_srcIedName;
	QString m_destIedName;
	QString m_preferredMainIedName;
	QSet<quint64> m_relatedVirtualCodeSet;
};
