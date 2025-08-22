#ifndef INTERACTIVESVGMAPITEM_H
#define INTERACTIVESVGMAPITEM_H

#include <QGraphicsItem>
#include <QPixmap>
#include <QVector>
#include <QPointF>
#include <QString>
#include <QSvgRenderer>
#include "pugixml.hpp"
#include <QTransform>
#include <QMap>
#include <QSharedPointer>

// 前置声明以避免在头文件中包含大量 Qt 头（减少编译依赖并修复不完整类型报错）
class QGraphicsSceneHoverEvent;
class QGraphicsSceneContextMenuEvent;
class QGraphicsSceneMouseEvent;
class QGraphicsSceneWheelEvent;
class QScrollBar;

// 先定义样式与箭头，再定义 MapLine
struct SvgNodeStyle
{
    QColor stroke;
    int strokeWidth;
    QString dashArray;
    double strokeOpacity;
    QColor fill;
    double fillOpacity;
    
    SvgNodeStyle() : strokeWidth(1), strokeOpacity(1.0), fillOpacity(1.0) {}
};

// 极简线条样式，用于回路与箭头，满足“加粗/变颜色/闪烁”三需求
struct LineStyle
{
	QRgb strokeRgb;         // 只存颜色值，绘制时组装 QColor
	unsigned short strokeWidth; // 线宽（像素）
	LineStyle() : strokeRgb(0), strokeWidth(1) {}
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
	struct Attrs {
		QString ref;   // 软压板引用，作为关联键
		QString desc;
		QString id;
		Attrs() {}
	} attrs;
};

// 箭头多边形数据
struct ArrowHead
{
	QVector<QPointF> points;   // 解析自 path d 的点（变换后）
};

// 回路类型，替代 QString，显著减小内存
enum LineType
{
    LineType_Virtual = 0,
    LineType_Logic   = 1,
    LineType_Optical = 2
};

struct MapLine
{
	QVector<QPointF> points;
	LineType type; // 类型标记（虚/逻/光）
	LineStyle style;                    // 线条样式
	QVector<ArrowHead> arrows;         // 箭头多边形
	int svgGrpId;                      // 来自组的 id，用于与虚拟数值框关联
	QString code;                      // 统一的线路 code（optical: code; logic: circuit-code; virtual: code 如有）

	// 显式回路属性（按类型划分），仅保留一类；使用单一多态指针以减小内存
	struct AttrBase { virtual ~AttrBase() {} };
	struct OpticalAttrs : AttrBase {
		QString srcIed;
		QString destIed;
		QString srcPort;
		QString destPort;
		QString code;       // 与 basemodel::OpticalCircuit::code 对应，原为数值，这里保留字符串，外部可再转
		QString status;     // true/false
		QString remoteId;   // 原为整数
		OpticalAttrs() {}
	};
	struct LogicAttrs : AttrBase {
		QString srcIedName;
		QString destIedName;
		QString srcCbName;
		QString circuitCode; // circuit-code
		LogicAttrs() {}
	};
	struct VirtualAttrs : AttrBase {
		QString srcIedName;
		QString destIedName;
		QString srcSoftPlateDesc;
		QString destSoftPlateDesc;
		QString srcSoftPlateRef;
		QString destSoftPlateRef;
		QString remoteId;
		QString remoteSigId_A;
		QString remoteSigId_B;
		QString virtualType; // virtual-type: gse/sv
		VirtualAttrs() {}
	};
	QSharedPointer<AttrBase> attrs; // 仅分配一种属性结构

	MapLine() : type(LineType_Virtual), svgGrpId(-1) {}
};

static QVector<double> extractNumbers(const QString& d) {
	QVector<double> out;
	QString num; num.reserve(d.size());
	for (int i = 0; i < d.size(); ++i) {
		const QChar c = d[i];
		if (c.isDigit() || c == '-' || c == '.') {
			num.append(c);
		}
		else {
			if (!num.isEmpty()) { out.append(num.toDouble()); num.clear(); }
		}
	}
	if (!num.isEmpty()) out.append(num.toDouble());
	return out;
}

static QColor parseColor(const char* s, double opacity = 1.0) {
	if (!s || !*s) return Qt::transparent;
	QColor c(QString::fromLatin1(s));
	if (!c.isValid()) return Qt::transparent;
	c.setAlphaF(qBound(0.0, opacity, 1.0));
	return c;
}


class InteractiveSvgMapItem : public QGraphicsItem
{
public:
	InteractiveSvgMapItem(const QString& svgPath);

	QRectF boundingRect() const;
	void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

	void setHighlightedLine(int idx);

	// 动态设置某条线路（按组 id）的左右虚拟数值（double -> 文本，默认保留 3 位小数）
	void setVirtualValues(int lineId, double leftValue, double rightValue, int precision = 3);
	// 仅设置一侧：isLeft=true 设置左侧；false 设置右侧
	void setVirtualValue(int lineId, bool isLeft, double value, int precision = 3);
	// 查询该线路对应的左右文本框矩形，返回是否找到
	bool getVirtualValueRects(int lineId, QRectF& leftRect, QRectF& rightRect) const;

	// 基于线路 code 的便捷接口：
	// 同时将左右文本设置为同一个数值
	void setVirtualValuesByCode(const QString& lineCode, double value, int precision = 3);
	// 仅设置左侧/右侧文本
	void setLeftVirtualValue(const QString& lineCode, double value, int precision = 3);
	void setRightVirtualValue(const QString& lineCode, double value, int precision = 3);

protected:
	void hoverMoveEvent(QGraphicsSceneHoverEvent* event);
	void contextMenuEvent(QGraphicsSceneContextMenuEvent* event);
	void mousePressEvent(QGraphicsSceneMouseEvent* event);
	void mouseMoveEvent(QGraphicsSceneMouseEvent* event);
	void mouseReleaseEvent(QGraphicsSceneMouseEvent* event);
	void wheelEvent(QGraphicsSceneWheelEvent* event);

private:
	void Clean();
	void parseSvgAndInit(const QString& svgPath);
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


	QPixmap m_bgPixmap;
	QVector<MapLine> m_allLines;
    QVector<PlateItem> m_allPlates;
	QMap<QString, PlateItem*> m_plateMap; // plateRef -> PlateItem*
	QMap<QString, int> m_lineIdByCode;    // 线路 code -> 组 id
	// 每条线路（组 id）对应两侧数值框
	struct ValueBox { QRectF rect; QString text; };
	struct ValuePair { bool hasLeft; bool hasRight; ValueBox left; ValueBox right; ValuePair():hasLeft(false),hasRight(false){} };
	QMap<int, ValuePair> m_valuePairs; // lineId -> {left,right}
	int m_highlightedLineIdx;
	bool m_dragging;
	QPoint m_lastViewPos;
	bool m_fittedOnce;
	double m_minScale;
	double m_maxScale;
	LineType m_svgType;		// 当前 SVG 的类型（虚/逻/光），用于菜单逻辑
	// 将文档坐标系映射为像素坐标系（当 viewBox.x/y 非 0 时用于整体平移）
	QTransform m_docToPix;
    // 解析节点的通用样式属性
    SvgNodeStyle parseNodeStyle(const pugi::xml_node& node);
	LineStyle parseLineStyle(const pugi::xml_node& node);

};

#endif // INTERACTIVESVGMAPITEM_H
