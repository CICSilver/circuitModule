#pragma once

#include "directitembase.h"
#include <QColor>
#include <QLineF>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QtGlobal>

struct PlateRect;
struct IedRect;

class IedItem : public directItemBase
{
public:
	IedItem(QGraphicsItem* parent = NULL);
	~IedItem();
	void setFromIedRect(const IedRect& rect, bool isSwitcher);
	QRectF boundingRect() const;
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

private:
	void drawText(QPainter* painter, const QRectF& rect, const QString& name, const QString& desc, int pointSize);

private:
	QRectF m_rect;
	int m_extendHeight;
	int m_innerGap;
	int m_padding;
	quint32 m_borderColor;
	quint32 m_underColor;
	QString m_iedName;
	QString m_iedDesc;
	bool m_isSwitcher;
};

class LogicFrameItem : public directItemBase
{
public:
	LogicFrameItem(QGraphicsItem* parent = NULL);
	~LogicFrameItem();
	static int TitleFontPointSize();
	QRectF boundingRect() const;
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
	void setFrame(const QRectF& rect, const QString& title, const QColor& borderColor, bool showLegend);

private:
	QRectF buildLegendRect() const;

private:
	QRectF m_rect;
	QString m_title;
	QColor m_borderColor;
	bool m_showLegend;
};

class DirectPlateItem : public directItemBase
{
public:
	DirectPlateItem(QGraphicsItem* parent = NULL);
	~DirectPlateItem();
	QRectF boundingRect() const;
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
	void setFromPlateRect(const PlateRect& rect, const QString& idKey);
	void setClosed(bool closed);
	bool isClosed() const
	{
		return m_closed;
	}
	QString idKey() const
	{
		return m_idKey;
	}
	QString iedName() const
	{
		return m_iedName;
	}
	QString ref() const
	{
		return m_ref;
	}
	QString desc() const
	{
		return m_desc;
	}
	quint64 code() const
	{
		return m_code;
	}

private:
	void rebuildGeometry();

private:
	QRectF m_rect;
	QPointF m_circle1;
	QPointF m_circle2;
	QLineF m_lineTop;
	QLineF m_lineBottom;
	bool m_closed;
	QString m_idKey;
	QString m_iedName;
	QString m_ref;
	QString m_desc;
	quint64 m_code;
};

class DirectMaintainPlateItem : public directItemBase
{
public:
	DirectMaintainPlateItem(QGraphicsItem* parent = NULL);
	~DirectMaintainPlateItem();
	QRectF boundingRect() const;
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
	void setIedName(const QString& iedName);
	QString iedName() const
	{
		return m_iedName;
	}
	void setDisplayText(const QString& text);
	void setStateByValue(int value);
	void setClosed(bool closed);
	bool isClosed() const
	{
		return m_closed;
	}
	void setAnchorRect(const QRectF& iedRect);

private:
	void rebuildGeometry();

private:
	QRectF m_anchorRect;
	QRectF m_textRect;
	QRectF m_iconRect;
	QRectF m_bodyRect;
	QPointF m_leftCircleCenter;
	QPointF m_rightCircleCenter;
	QString m_iedName;
	QString m_displayText;
	bool m_closed;
};
