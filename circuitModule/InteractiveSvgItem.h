#ifndef INTERACTIVESVGMAPITEM_H
#define INTERACTIVESVGMAPITEM_H

#include <QGraphicsItem>
#include <QObject>
#include <QPixmap>
#include <QByteArray>
#include <QSize>
#include <QSizeF>
#include <QVector>
#include <QPointF>
#include <QString>
#include <QSvgRenderer>
#include "pugixml.hpp"
#include <QTransform>
#include <QMap>
#include <QSharedPointer>
#include <QPointer>
#include <circuitconfig.h>
#include "RtdbClient.h"
class QGraphicsSceneHoverEvent;
class QGraphicsSceneContextMenuEvent;
class QGraphicsSceneMouseEvent;
class QGraphicsSceneWheelEvent;
class QScrollBar;
class SecWidget;
struct SvgNodeStyle
{
	QColor stroke;
	int strokeWidth;
	QString dashArray;
	double strokeOpacity;
	QColor fill;
	double fillOpacity;

	SvgNodeStyle() : strokeWidth(1), strokeOpacity(1.0), fillOpacity(1.0)
	{
	}
};

struct LineStyle
{
	QRgb strokeRgb;         // 只存颜色值，绘制时组装 QColor
	unsigned short strokeWidth; // 线宽（像素）
	LineStyle() : strokeRgb(0), strokeWidth(1)
	{
	}
};

struct PlateCircleItem
{
	QPointF center;
	qreal   radius;
	SvgNodeStyle style;
};

struct PlateLineItem
{
	QPointF start;
	QPointF end;
	SvgNodeStyle style;
	qreal   length() const { return QLineF(start, end).length(); }
};

struct PlateRectItem
{
	QRectF rect;
	SvgNodeStyle style;
};

struct PlateItem
{
	QString svgGrpId;              // plate 节点 id，用于和虚回路关联
	bool isClosed;           // true=置合，false=置分
	QRectF rect;             // plate 节点中的 rect（用于命中/逻辑）
	QRectF outerRect;        // plate-component 中的外部矩形（覆盖回路用）
	QVector<PlateCircleItem> circles;	// 压板的两个圆
	QVector<PlateLineItem> lines;			// 压板的两条线
	QVector<PlateRectItem> rects;

	// 压板的语义属性
	struct Attrs
	{
		QString code;
		QString iedName;
		QString ref;   // 软压板引用，作为关联键
		QString desc;
		QString id;
		Attrs()
		{
		}
	} attrs;
};

// 箭头多边形数据
struct ArrowHead
{
	QVector<QPointF> points;   // 解析自 path d 的点（变换后）
};

enum LineType
{
	LineType_Virtual = 0,
	LineType_Logic   = 1,
	LineType_Optical = 2
};
enum CircuitType
{
	CircuitType_GSE = 0,
	CircuitType_SV  = 1
};

enum LineStatus
{
	Status_Disconnected = 0,	// 断开
	Status_Connected = 1,		// 连接
	Status_Alarm = 2			// 告警
};

struct MapLine
{
	QVector<QPointF> points;
	LineType type; // 类型标记（虚/逻/光）
	LineStyle style;					// 线条样式
	QVector<ArrowHead> arrows;			// 箭头多边形
	int svgGrpId;						// 来自组的 id，用于与虚拟数值框关联
	QString code;						// 统一的线路 code（optical: code; logic: circuit-code; virtual: code 如有）
	LineStatus status;					// 线路状态
	bool isBlinking;					// 是否闪烁
	// 显式回路属性（按类型划分），仅保留一类；使用单一多态指针以减小内存
	struct AttrBase
	{
		virtual ~AttrBase()
		{
		}
	};
	struct OpticalAttrs : AttrBase
	{
		QString srcIed;
		QString destIed;
		QString srcPort;
		QString destPort;
		QString loopCode;	// 回路编号#后部分，同光纤回路LoopCode相同
		QString remoteId;   // 原为整数
		OpticalAttrs()
		{
		}
	};
	struct LogicAttrs : AttrBase
	{
		QString srcIedName;
		QString destIedName;
		QString srcCbName;
		QString circuitCode; // circuit-code
		LogicAttrs()
		{
		}
	};
	struct VirtualAttrs : AttrBase
	{
		QString srcSoftPlateCode;
		QString destSoftPlateCode;
		QString srcIedName;
		QString destIedName;
		QString srcSoftPlateDesc;
		QString destSoftPlateDesc;
		QString srcSoftPlateRef;
		QString destSoftPlateRef;
		QString circuitDesc;
		QString remoteId;
		QString remoteSigId_A;
		QString remoteSigId_B;
		CircuitType circuitType;
		VirtualAttrs()
		{
		}
	};
	QSharedPointer<AttrBase> attrs; // 仅分配一种属性结构

	MapLine() : type(LineType_Virtual), svgGrpId(-1), status(Status_Connected)
	{
	}
};

static QVector<double> extractNumbers(const QString& d)
{
	QVector<double> out;
	QString num; num.reserve(d.size());
	for (int i = 0; i < d.size(); ++i)
	{
		const QChar c = d[i];
		if (c.isDigit() || c == '-' || c == '.')
		{
			num.append(c);
		}
		else
		{
			if (!num.isEmpty())
			{
				out.append(num.toDouble());
				num.clear();
			}
		}
	}
	if (!num.isEmpty()) out.append(num.toDouble());
	return out;
}

static QColor parseColor(const char* s, double opacity = 1.0)
{
	if (!s || !*s) return Qt::transparent;
	QColor c(QString::fromLatin1(s));
	if (!c.isValid()) return Qt::transparent;
	c.setAlphaF(qBound(0.0, opacity, 1.0));
	return c;
}

class InteractiveSvgMapItem : public QGraphicsObject
{
	Q_OBJECT
public:
	InteractiveSvgMapItem(const QString& svgPath);
	// 从内存 SVG 字节构造（与文件版逻辑一致）
	InteractiveSvgMapItem(const QByteArray& svgBytes);

	QRectF boundingRect() const;
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

	void setHighlightedLine(int idx);

	// 动态设置某条线路（按组 id）的左右虚拟数值（double -> 文本，默认保留 3 位小数）
	void setVirtualValues(int lineId, double leftValue, double rightValue, int precision = 3);
	// 仅设置一侧：isLeft=true 设置左侧；false 设置右侧
	void setVirtualValue(int lineId, bool isLeft, double value, int precision = 3);

	QMap<QString, int>& lineIdMapByType(CircuitType type)
	{
		return type == CircuitType_SV ? m_svLineIdByCode :
			type == CircuitType_GSE ? m_gseLineIdByCode :
			m_svLineIdByCode;
	}
	void clearLineIdMap()
	{
		m_svLineIdByCode.clear();
		m_gseLineIdByCode.clear();
	}
	// 同时将左右文本设置为同一个数值
	void setVirtualValuesByCode(const MapLine& line, double value, int precision = 3);
	// 仅设置左侧/右侧文本
	void setOutVirtualValue(const MapLine& line, double value, int precision = 3);
	void setInVirtualValue(const MapLine& line, double value, int precision = 3);
	void updatePlateStatuses();
	void updateLineStatuses();
	void UpdateOpticalCircuitStatus(MapLine& line);
	void UpdateVirtualCircuitStatus(const MapLine& line);
protected:
	void hoverMoveEvent(QGraphicsSceneHoverEvent* event);
	void hoverLeaveEvent(QGraphicsSceneHoverEvent* event);
	void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event);
	void contextMenuEvent(QGraphicsSceneContextMenuEvent* event);
	void mousePressEvent(QGraphicsSceneMouseEvent* event);
	void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);
	void wheelEvent(QGraphicsSceneWheelEvent* event);

protected slots:
	void onBlinkTimeout();
	void onTooltipTimeout();
	void onStatusTimeout();

private:
	void initCommon();
	void Clean();
	void parseSvgAndInit(const QString& svgPath);
	void parseSvgAndInit(const QByteArray& svgBytes);
	void parseVirtualSvg(const pugi::xml_document& doc);
	void parseLogicSvg(const pugi::xml_document& doc);
	void parseOpticalSvg(const pugi::xml_document& doc);
	// 解析 type="virtual-value" 的文本框，按组 id 归并为左右一对
	void parseVirtualValueBoxes(const pugi::xml_document& doc);
	// 通用回路与箭头解析
	QVector<MapLine> parseCircuitLines(const pugi::xml_document& doc, const char* type);
	QVector<ArrowHead> parseArrowHeadsForGroup(const pugi::xml_document& doc, const int grp_id);
	// 将 SVG 的属性键名规整为 basemodel 可直接复用的键名（并保留原始键）
	void normalizeAttrsForBaseModel(MapLine& line, const pugi::xml_node& g) const;
	// 根据当前压板状态，决定回路线条颜色
	QColor colorForLine(const MapLine& line) const;
	// 命中测试：返回点击命中的压板索引，未命中返回 -1
	int hitTestPlate(const QPointF& pos) const;
	void drawPlateIcon(QPainter* painter, const QPointF& center) const;
	QString buildPlateTooltip(const PlateItem& plate) const;

	/// 跳转
	void showOpticalRelatedCircuits(const MapLine& line);

	/// 绘制线路
	void paintLine(QPainter* painter, const MapLine& line, bool isHighLight) const;

	//// 解析压板相关信息
	// 解析svg中圆的贝塞尔近似描述
	QVector<PlateCircleItem> parsePlateCircles(const pugi::xml_node& plateCircleNode);
	QVector<PlateLineItem> parsePlateLines(const pugi::xml_node& plateLineNode);
	QVector<PlateRectItem> parsePlateRects(const pugi::xml_node& plateRectNode);
	// 绘制压板的圆
	void drawPlateCircles(QPainter* p, const QVector<PlateCircleItem>& cs);
	// 绘制压板的线
	void drawPlateLines(QPainter* p, const QVector<PlateLineItem>& ls);
	// 绘制压板的矩形
	void drawPlateRects(QPainter* p, const QVector<PlateRectItem>& rs);

	// 绘制压板（从 paint 中抽离），保证在回路之后绘制
	void paintPlates(QPainter* painter);
	void paintSinglePlate(QPainter* painter, const PlateItem& plate);
	void paintVirtualValues(QPainter* painter);
	void fitToViewIfPossible();
	// 解析节点的通用样式属性
	SvgNodeStyle parseNodeStyle(const pugi::xml_node& node);
	LineStyle parseLineStyle(const pugi::xml_node& node);

	// 创建线路描述
	QString buildLineTooltip(const MapLine& line) const;

private:
	enum HoverPart
	{
		Hover_None = 0,
		Hover_Line = 1,
		Hover_Plate = 2
	};
	HoverPart m_currentHoverPart;
	QTimer* m_blinkTimer;
	QTimer* m_statusTimer;
	bool m_blinkOn;
	QPixmap m_bgPixmap;
	QByteArray m_svgCache;
	QString m_svgSourcePath;
	QSize m_baseRasterSize;
	QSizeF m_itemSize;
	double m_pixmapScaleFactor;
	QVector<MapLine> m_allLines;
	QVector<PlateItem> m_allPlates;
	QMap<QString, PlateItem*> m_plateMap; // plateRef -> PlateItem*
	QMap<QString, int> m_svLineIdByCode;    // 线路 code -> 组 id
	QMap<QString, int> m_gseLineIdByCode;    // 线路 code -> 组 id
	// 每条线路（组 id）对应两侧数值框
	struct ValueBox
	{
		QRectF rect;
		QString text;
	};
	struct ValuePair
	{
		ValueBox out;
		ValueBox in;
		ValuePair()
		{
		}
	};
	QMap<int, ValuePair> m_valuePairs; // lineId -> {left,right}
	//QVector<ValueBox> m_circuitDescBoxes; // 线路描述文本框
	int m_highlightedLineIdx;
	int m_hoverPlateIdx; // 当前悬停的压板索引，-1 表示无
	bool m_dragging;
	QPoint m_lastViewPos;
	bool m_fittedOnce;
	double m_minScale;
	double m_maxScale;
	LineType m_svgType;		// 当前 SVG 的类型（虚/逻/光），用于菜单逻辑
	// 将文档坐标系映射为像素坐标系（当 viewBox.x/y 非 0 时用于整体平移）
	QTransform m_docToPix;
	QTimer* m_tooltipTimer;
	QString m_tooltipText;
	QPoint m_tooltipPos;
	CircuitConfig* m_circuitConfig;
	RtdbClient& m_rtdb;
	//SecWidget* m_secWidget;
};

#endif // INTERACTIVESVGMAPITEM_H
