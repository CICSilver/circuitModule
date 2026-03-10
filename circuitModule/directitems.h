#pragma once

#include <QGraphicsItem>
#include <QPainterPath>
#include <QString>
#include <QRectF>
#include <QPointF>
#include <QLineF>
#include <QVector>
#include <QColor>
#include <QtGlobal>

struct PlateRect;
struct IedRect;
struct VirtualCircuitLine;
struct OpticalCircuitLine;
class directItemBase  : public QGraphicsItem
{
public:
	enum ItemType
	{
		Plate = 0,
		CircuitLine,
		Icon,
		Text,
		Ied
	};
	virtual ~directItemBase() = 0;
	virtual QRectF  boundingRect() const = 0;
	virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) = 0;
	ItemType itemType() const { return static_cast<ItemType>(m_itemType); }
protected:
	quint16 m_itemType;
};
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

class DirectPlateItem : public directItemBase
{
public:
	DirectPlateItem(QGraphicsItem* parent = NULL);
	~DirectPlateItem();
	QRectF boundingRect() const;
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
	void setFromPlateRect(const PlateRect& rect, const QString& idKey);
	void setClosed(bool closed);
	bool isClosed() const { return m_closed; }
	QString idKey() const { return m_idKey; }
	QString iedName() const { return m_iedName; }
	QString ref() const { return m_ref; }
	QString desc() const { return m_desc; }
	quint64 code() const { return m_code; }
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

class DirectVirtualLineItem : public directItemBase
{
public:
	DirectVirtualLineItem(QGraphicsItem* parent = NULL);
	~DirectVirtualLineItem();
	QRectF boundingRect() const;
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
	//************************************
	// 函数名称:	shape
	// 函数全名:	DirectVirtualLineItem::shape
	// 访问权限:	public
	// 函数说明:	返回线条命中区域
	// 函数参数:	void
	// 返回值:	QPainterPath
	//************************************
	QPainterPath shape() const;
	//************************************
	// 函数名称:	setFromVirtualLine
	// 函数全名:	DirectVirtualLineItem::setFromVirtualLine
	// 访问权限:	public
	// 函数说明:	从虚回路结构设置数据
	// 函数参数:	const VirtualCircuitLine& line
	// 函数参数:	int vtype
	// 返回值:	void
	//************************************
	void setFromVirtualLine(const VirtualCircuitLine& line, int vtype);
	//************************************
	// 函数名称:	setVirtualType
	// 函数全名:	DirectVirtualLineItem::setVirtualType
	// 访问权限:	public
	// 函数说明:	设置虚回路类型(GSE/SV)
	// 函数参数:	int vtype
	// 返回值:	void
	//************************************
	void setVirtualType(int vtype);
	//************************************
	// 函数名称:	setValues
	// 函数全名:	DirectVirtualLineItem::setValues
	// 访问权限:	public
	// 函数说明:	设置虚回路数值(出/入)
	// 函数参数:	const QString& outVal
	// 函数参数:	const QString& inVal
	// 返回值:	void
	//************************************
	void setValues(const QString& outVal, const QString& inVal);
	//************************************
	// 函数名称:	setBlinking
	// 函数全名:	DirectVirtualLineItem::setBlinking
	// 访问权限:	public
	// 函数说明:	设置闪烁状态
	// 函数参数:	bool blinking
	// 返回值:	void
	//************************************
	void setBlinking(bool blinking);
	//************************************
	// 函数名称:	setBlinkOn
	// 函数全名:	DirectVirtualLineItem::setBlinkOn
	// 访问权限:	public
	// 函数说明:	设置闪烁显示开关
	// 函数参数:	bool on
	// 返回值:	void
	//************************************
	void setBlinkOn(bool on);
	//************************************
	// 函数名称:	setHighlighted
	// 函数全名:	DirectVirtualLineItem::setHighlighted
	// 访问权限:	public
	// 函数说明:	设置高亮显示
	// 函数参数:	bool highlighted
	// 返回值:	void
	//************************************
	void setHighlighted(bool highlighted);
	//************************************
	// 函数名称:	setCircuitCode
	// 函数全名:	DirectVirtualLineItem::setCircuitCode
	// 访问权限:	public
	// 函数说明:	设置回路编码
	// 函数参数:	quint64 code
	// 返回值:	void
	//************************************
	void setCircuitCode(quint64 code);
	//************************************
	// 函数名称:	circuitCode
	// 函数全名:	DirectVirtualLineItem::circuitCode
	// 访问权限:	public
	// 函数说明:	获取回路编码
	// 函数参数:	void
	// 返回值:	quint64
	//************************************
	quint64 circuitCode() const;
	//************************************
	// 函数名称:	virtualType
	// 函数全名:	DirectVirtualLineItem::virtualType
	// 访问权限:	public
	// 函数说明:	获取虚回路类型
	// 函数参数:	void
	// 返回值:	int
	//************************************
	int virtualType() const;
	//************************************
	// 函数名称:	isBlinking
	// 函数全名:	DirectVirtualLineItem::isBlinking
	// 访问权限:	public
	// 函数说明:	获取闪烁状态
	// 函数参数:	void
	// 返回值:	bool
	//************************************
	bool isBlinking() const;
	//************************************
	// 函数名称:	setLineColor
	// 函数全名:	DirectVirtualLineItem::setLineColor
	// 访问权限:	public
	// 函数说明:	设置线条颜色
	// 函数参数:	const QColor& color
	// 返回值:	void
	//************************************
	void setLineColor(const QColor& color);
	//************************************
	// 函数名称:	setArrowColor
	// 函数全名:	DirectVirtualLineItem::setArrowColor
	// 访问权限:	public
	// 函数说明:	设置箭头颜色
	// 函数参数:	const QColor& color
	// 返回值:	void
	//************************************
	void setArrowColor(const QColor& color);
	//************************************
	// 函数名称:	setValueVisible
	// 函数全名:	DirectVirtualLineItem::setValueVisible
	// 访问权限:	public
	// 函数说明:	设置数值显示
	// 函数参数:	bool visible
	// 返回值:	void
	//************************************
	void setValueVisible(bool visible);

protected:
	//************************************
	// 函数名称:	hoverEnterEvent
	// 函数全名:	DirectVirtualLineItem::hoverEnterEvent
	// 访问权限:	protected
	// 函数说明:	鼠标悬停进入
	// 函数参数:	QGraphicsSceneHoverEvent* event
	// 返回值:	void
	//************************************
	void hoverEnterEvent(QGraphicsSceneHoverEvent* event);
	//************************************
	// 函数名称:	hoverLeaveEvent
	// 函数全名:	DirectVirtualLineItem::hoverLeaveEvent
	// 访问权限:	protected
	// 函数说明:	鼠标悬停离开
	// 函数参数:	QGraphicsSceneHoverEvent* event
	// 返回值:	void
	//************************************
	void hoverLeaveEvent(QGraphicsSceneHoverEvent* event);
private:
	QPointF m_startPoint;
	QPointF m_endPoint;
	QPointF m_startIconPt;
	QPointF m_endIconPt;
	QRectF m_startValRect;
	QRectF m_endValRect;
	QRectF m_descRect;
	QString m_outValStr;
	QString m_inValStr;
	QString m_descStr;
	quint64 m_circuitCode;
	int m_virtualType;
	QColor m_lineColor;
	QColor m_arrowColor;
	int m_lineWidth;
	bool m_valueVisible;
	bool m_blinking;
	bool m_blinkOn;
	bool m_highlighted;
};

class DirectOpticalLineItem : public directItemBase
{
public:
	DirectOpticalLineItem(QGraphicsItem* parent = NULL);
	~DirectOpticalLineItem();
	QRectF boundingRect() const;
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
	//************************************
	// 函数名称:	shape
	// 函数全名:	DirectOpticalLineItem::shape
	// 访问权限:	public
	// 函数说明:	返回光纤线路命中区域
	// 函数参数:	void
	// 返回值:	QPainterPath
	//************************************
	QPainterPath shape() const;
	//************************************
	// 函数名称:	setFromOpticalLine
	// 函数全名:	DirectOpticalLineItem::setFromOpticalLine
	// 访问权限:	public
	// 函数说明:	设置光纤链路数据
	// 函数参数:	const OpticalCircuitLine& line
	// 返回值:	void
	//************************************
	void setFromOpticalLine(const OpticalCircuitLine& line);
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
	quint8 m_arrowState;
	quint16 m_lineCode;
	QColor m_lineColor;
	int m_lineWidth;
	bool m_highlighted;
};

class LineItem : public directItemBase
{
public:
	LineItem(QGraphicsItem* parent = NULL);
	~LineItem();
	QRectF boundingRect() const;
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);
	void setPoints(const QVector<QPointF>& pts);
	void setColor(const QColor& color);
	void setWidth(int w);

private:
	QVector<QPointF> m_points;
	QColor m_color;
	int m_width;
};

//class DirectPlateItem : public directItemBase
//{
//public:
//	DirectPlateItem(QGraphicsItem* parent = NULL) {};
//	~DirectPlateItem() {};
//	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;
//	QRectF boundingRect() const;
//};
//class CircuitLineItem : public directItemBase
//{
//};
//class IconItem : public directItemBase
//{
//};
//class TextItem : public directItemBase
//{
//};
