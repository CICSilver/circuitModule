#pragma once

#include <QGraphicsSvgItem>
#include <QPointF>
#include <QGraphicsView>
#include <QWheelEvent>
#include "include/pugixml/pugixml.hpp"
class LogicCircuitLine;
class IedRect;
class CustomView : public QGraphicsView
{
public:
	CustomView(QWidget* parent = NULL)
		: QGraphicsView(parent)
	{
		setRenderHint(QPainter::Antialiasing);
		setRenderHint(QPainter::TextAntialiasing);
		setRenderHint(QPainter::SmoothPixmapTransform);
		setDragMode(QGraphicsView::ScrollHandDrag);
	};
	void wheelEvent(QWheelEvent* event)
	{
		qreal scaleFactor = (event->delta() > 0) ? 1.2 : 1.0 / 1.2;
		this->scale(scaleFactor, scaleFactor);
		event->accept();
	}

	void mouseMoveEvent(QMouseEvent* event)
	{
		this->setCursor(Qt::ArrowCursor);
		QGraphicsView::mouseMoveEvent(event);
	}

	void mouseReleaseEvent(QMouseEvent* event)
	{
		this->setCursor(Qt::ArrowCursor);
		QGraphicsView::mouseReleaseEvent(event);
	}
};

class InteractiveLineItem : public QGraphicsPathItem
{
public:
	enum LineType { Virtual, Logic, Optical };
	InteractiveLineItem(const QPainterPath& path, LineType t, const QColor& color, QGraphicsItem* parent = NULL)
		: QGraphicsPathItem(path, parent), m_type(t), m_baseColor(color)
	{
		setAcceptHoverEvents(true);
		setPen(QPen(m_baseColor, 6));
	}
protected:
	void hoverEnterEvent(QGraphicsSceneHoverEvent*)
	{
		setPen(QPen(m_baseColor.darker(150), 10)); // 悬停变粗变深
		update();
	}
	void hoverLeaveEvent(QGraphicsSceneHoverEvent*)
	{
		setPen(QPen(m_baseColor, 6));
		update();
	}
private:
	LineType m_type;
	QColor m_baseColor;
};

class CustomSvgItem  : public QGraphicsSvgItem
{
public:
	typedef pugi::xpath_node_set::const_iterator nodeSetConstIterator;
	CustomSvgItem(const QString& fileName, QGraphicsItem* parent = NULL);
	CustomSvgItem(QGraphicsItem* parent = NULL);
	~CustomSvgItem()
	{
	}

	void LoadSvg(const QString& fileName);
	void LoadInteractiveLines(const QString& fileName, QGraphicsScene* scene);

protected:

	//************************************
	// 函数名称:	LoadSvgData
	// 函数全名:	CustomSvgItem::LoadSvgData
	// 访问权限:	protected
	// 函数说明:	以QByteArray形式读取svg数据，存储与m_originalSvgData以进行动态修改操作
	// 函数参数:	const QString & fileName
	// 返回值:		QByteArray
	//************************************
	QByteArray LoadSvgData(const QString& fileName);

	//************************************
	// 函数名称:    GetSvgNodeByPos
	// 函数全名:	CustomSvgItem::GetSvgNodeByPos
	// 访问权限:	protected
	// 函数说明:	获取指定pos所在的图形的g节点(包含图形属性)
	// 函数参数:	const QPointF & pos
	// 返回值:		pugi::xml_node
	//************************************
	pugi::xml_node GetSvgNodeByPos(const QPointF& pos);

	//************************************
	// 函数名称:	isPointOnPath
	// 函数全名:	CustomSvgItem::isPointOnPath
	// 访问权限:	protected
	// 函数说明:	判断输入点是否在指定路径line上，width为判定宽度
	// 函数参数:	const QPointF & pos
	// 函数参数:	QLineF & line
	// 函数参数:	quint8 width
	// 返回值:		bool
	//************************************
	bool isPointOnPath(const QPointF& pos, QLineF& line, quint8 width = 8);
protected:
	//void hoverMoveEvent(QGraphicsSceneHoverEvent* event);
	void wheelEvent(QGraphicsSceneWheelEvent* event);
private:
	QList<QLineF> GetLinesFromGNode(pugi::xml_node& gNode);
	QRectF GetRectFromGNode(pugi::xml_node& gNode);
	QLineF GetLineFromStr(const QString& str);
	qreal DegreeToRadians(qreal degree);

private:
	// 解析svg图元位置辅助
	// 简易svg path解析器，只支持M/L/Z
	QPainterPath ParseSvgPath(const QString& d);
	// svg polyline解析器
	QPainterPath ParseSvgPolyLine(const QString& points);
	// xml_document转QByteArray辅助类
	struct ByteWriter : pugi::xml_writer
	{
		ByteWriter(QByteArray& byteArray) : m_byteArray(byteArray)
		{
		}
		virtual void write(const void* data, size_t size)
		{
			m_byteArray.append(static_cast<const char*>(data), static_cast<int>(size));
		}
	private:
		QByteArray& m_byteArray;
	};
	// xml_document转QByteArray
	QByteArray xmlDocumentToByteArray()
	{
		QByteArray data;
		ByteWriter writer(data);
		m_doc.save(writer);
		return data;
	}

private:
	bool m_isDragging;
	QPointF m_lastMousePos;
	QByteArray m_originalSvgData;
	pugi::xml_document m_doc;
	QList<InteractiveLineItem*> m_interactiveLines;
};
