#pragma once
#include "basemodel.h"
#include <QPoint>
#include <QRect>
#include <QLineF>
#ifndef SVG_DEFAULT_VALUE
#define SVG_DEFAULT_VALUE
// 默认IED矩形宽高
#define RECT_DEFAULT_WIDTH 280
#define RECT_DEFAULT_HEIGHT 100

// IED矩形 水平间距
#define IED_HORIZONTAL_DISTANCE 400
// 垂直间距
#define IED_VERTICAL_DISTANCE 15
// 虚链路垂直间距
#define CIRCUIT_VERTICAL_DISTANCE 15
// 图标高/宽度
#define ICON_LENGTH 20

#define SVG_VIEWBOX_WIDTH	2200
#define SVG_VIEWBOX_HEIGHT	1500
#define ARROW_LEN			10	// 箭头长度
#define CONN_R				3	// 连接点半径
#define PLATE_GAP			5	// 压板矩形内部间距
#define PLATE_WIDTH			70	// 压板图形宽度，圆间距40，圆半径均为5
#define PLATE_HEIGHT		15	// 压板图形高度
#define PLATE_CIRCLE_RADIUS	5	// 压板图形圆半径
#endif

struct CircuitLine;
struct LogicCircuitLine;
class VirtualCircuitLine;
struct SvgRect
{
	SvgRect()
	{
		width = RECT_DEFAULT_WIDTH;
		height = RECT_DEFAULT_HEIGHT;
		horizontal_margin = 40;
		vertical_margin = 20;
		inner_gap = 15;
		padding = 10;
		text_color = 0x000000;	// white
		left_connect_index = 0;
		right_connect_index = 0;
		bottom_connect_index = 0;
		top_connect_index = 0;
		extend_height = 0;
	}
	~SvgRect()
	{
		qDeleteAll(logic_line_list);
		logic_line_list.clear();
	}

	//************************************
	// 函数名称:	GetY
	// 函数全名:	SvgRect::GetY
	// 访问权限:	public
	// 函数说明:	获取底部Y坐标
	// 返回值:		quint16
	//************************************
	quint16 GetExtendBottomY() const
	{
		return y + extend_height - inner_gap;
	}
	quint16 GetInnerBottomY() const
	{
		return y + height;
	}
	quint16 GetExtendHeight() const
	{
		return 2 * inner_gap + height + extend_height;
	}
	//LogicCircuitLine* GetLogicCircuitLineById(quint8 id) const;

	quint16 x;
	quint16 y;
	quint16 width;
	quint16 height;
	quint16 vertical_margin;		// 矩形上下边距
	quint16 horizontal_margin;		// 矩形外边距（外部虚线框边距）
	quint16 extend_height;			// 外部虚线框矩形的高度, IED矩形底边到外部虚线框底边的距离（由回路数量决定）
	quint16 inner_gap;				// 内部矩形与外部虚线框之间的间隔
	quint16 padding;				// 矩形内边距（文本与外部虚线框距离）

	quint32 border_color;		// hex
	quint32 border_style;		// Qt::PenStyle
	quint32 underground_color;
	quint32 text_color;
	QString text;

	// 连接点数量
	quint8 left_connect_index;
	quint8 right_connect_index;
	quint8 bottom_connect_index;
	quint8 top_connect_index;

	QList<LogicCircuitLine*> logic_line_list;
};

struct IedRect : public SvgRect
{
	//const IED* pIed;
	QString iedName;	// IED名称
	QString iedDesc;	// 设备描述
};

// 连接点箭头状态，支持双向箭头
enum ArrowState
{
	Arrow_None = 0x0,
	Arrow_In = 0x01,
	Arrow_Out = 0x10,
	Arrow_InOut = Arrow_In | Arrow_Out
};

struct BaseCircuitLine
{
	BaseCircuitLine()
	{
		startPoint = QPoint(0, 0);
		midPoint = QPoint(0, 0);
		endPoint = QPoint(0, 0);
		pSrcIedRect = NULL;
		pDestIedRect = NULL;
	}
	//quint8 id;		// 标识IED中的回路编号，从0开始，仅供SVG中使用
	QPoint startPoint;	// 起点坐标，和方向相关，指向srcIedRect
	QPoint midPoint;	// 折线中点坐标，仅用于普通逻辑链路
	QPoint endPoint;	// 终点坐标，和方向相关，指向destIedRect
	QList<QPoint> turnPointList;	// 多拐点逻辑线的中间折点

	IedRect* pSrcIedRect;
	IedRect* pDestIedRect;
	//Circuit* pCircuit;
};

// 逻辑链路
struct LogicCircuitLine : public BaseCircuitLine
{
	LogicCircuitLine()
	{
		pLogicCircuit = NULL;
		arrowState = Arrow_None;
		srcArrowState = Arrow_None;
		destArrowState = Arrow_In;
	}
	~LogicCircuitLine()
	{
		qDeleteAll(virtual_line_list);
		virtual_line_list.clear();
	}
	LogicCircuit* pLogicCircuit;
	quint8 arrowState;
	quint8 srcArrowState;
	quint8 destArrowState;
	QList<VirtualCircuitLine*> virtual_line_list;	// 逻辑链路对应的虚回路列表
};

// 不存在跨交换机线路，交换机本身作为设备存储，连交换机的光纤视为单独光纤
struct OpticalCircuitLine
{
	OpticalCircuitLine()
	{
		lineCode = 0;
		arrowState = Arrow_None;
		srcArrowState = Arrow_None;
		destArrowState = Arrow_None;
		startPoint = QPoint(0, 0);
		endPoint = QPoint(0, 0);
		pSrcRect = NULL;
		pDestRect = NULL;
		pOpticalCircuit = NULL;
	}
	quint16 lineCode;		// 光纤编号
	quint8 arrowState;
	quint8 srcArrowState;
	quint8 destArrowState;
	QPoint startPoint;			// 光纤起点，位于图像上方，与光纤方向无关
	QPoint endPoint;			// 光纤终点，位于图像下方，与光纤方向无关
	QList<QPoint> midPoints;	// 可能有多个折线点
	IedRect* pSrcRect;			// 输出设备
	IedRect* pDestRect;			// 输入设备
	OpticalCircuit* pOpticalCircuit;
};
//// SVG描述结构，管理内存释放
struct BaseSvg
{
	BaseSvg()
	{
		mainIedRect = NULL;
		viewBoxWidth = SVG_VIEWBOX_WIDTH;
		viewBoxHeight = SVG_VIEWBOX_HEIGHT;
		viewBoxX = 0;
		viewBoxY = 0;
	}
	~BaseSvg()
	{
		if(mainIedRect)
		{
			delete mainIedRect;
			mainIedRect = NULL;
		}
	}
	IedRect* mainIedRect;
	quint16 viewBoxX;		// SVG视图左上角X坐标
	quint16 viewBoxY;		// SVG视图左上角Y坐标
	quint16 viewBoxWidth;
	quint16 viewBoxHeight;
};
// 逻辑链路SVG
struct LogicSvg : public BaseSvg
{
public:
	size_t GetLeftLogicCircuitSize() const
	{
		return GetLogicCircuitSize(leftIedRectList);
	}

	size_t GetRightLogicCircuitSize() const
	{
		return GetLogicCircuitSize(rightIedRectList);
	}

	//************************************
	// 函数名称:	GetLeftCircuitSize
	// 函数全名:	LogicSvg::GetLeftCircuitSize
	// 访问权限:	public
	// 函数说明:	获取左侧链路数量，包含全部数据引用
	// 返回值:		size_t
	//************************************
	size_t GetLeftCircuitSize() const
	{
		return GetCircuitSize(leftIedRectList);
	}

	//************************************
	// 函数名称:	GetRightCircuitSize
	// 函数全名:	LogicSvg::GetRightCircuitSize
	// 访问权限:	public
	// 函数说明:	获取右侧链路数量，包含全部数据引用
	// 返回值:		size_t
	//************************************
	size_t GetRightCircuitSize() const
	{
		return GetCircuitSize(rightIedRectList);
	}

	~LogicSvg()
	{
		qDeleteAll(centerIedRectList);
		qDeleteAll(leftIedRectList);
		qDeleteAll(rightIedRectList);
		qDeleteAll(descRectList);
	}

private:
	size_t GetLogicCircuitSize(const QList<IedRect*>& rectList) const
	{
		size_t ret = 0;
		foreach(IedRect * rect, rectList)
		{
			ret += rect->logic_line_list.size();
		}
		return ret;
	}
	size_t GetCircuitSize(const QList<IedRect*>& rectList) const
	{
		size_t ret = 0;
		foreach(IedRect * rect, rectList)
		{
			foreach(LogicCircuitLine * pLine, rect->logic_line_list)
			{
				ret += pLine->pLogicCircuit->circuitList.size();
			}
		}
		return ret;
	}
public:
	QList<IedRect*> centerIedRectList;
	QList<IedRect*> leftIedRectList;
	QList<IedRect*> rightIedRectList;
	QList<IedRect*> descRectList;
};

// 光纤SVG
struct OpticalSvg : public BaseSvg
{
	OpticalSvg()
	{
	}
	~OpticalSvg();

	IedRect* GetIedRectByIedName(QString iedName) const
	{
		foreach(IedRect * pRect, iedRectList)
		{
			if (pRect->iedName.compare(iedName, Qt::CaseInsensitive) == 0)
			{
				return pRect;
			}
		}
		return NULL;
	}
	OpticalCircuitLine* GetOpticalCircuitLineByIedName(QString iedName) const
	{
		foreach(OpticalCircuitLine* pLine, opticalCircuitLineList)
		{
			if (pLine->pSrcRect->iedName.compare(iedName, Qt::CaseInsensitive) == 0 ||
				pLine->pDestRect->iedName.compare(iedName, Qt::CaseInsensitive) == 0)
			{
				return pLine;
			}
		}
		return NULL;
	}
	OpticalCircuitLine* GetOpticalCircuitLineByLineCode(const quint16 lineCode) const
	{
		foreach(OpticalCircuitLine* pLine, opticalCircuitLineList)
		{
			if (pLine->lineCode == lineCode)
			{
				return pLine;
			}
		}
		return NULL;
	}
	QList<IedRect*> iedRectList;			// 管理全部设备矩形内存
	QList<IedRect*> switcherRectList;		// 管理交换机矩形内存
	QList<OpticalCircuitLine*> opticalCircuitLineList;
};
struct VirtualCircuitLine : public BaseCircuitLine
{
	VirtualCircuitLine(VirtualCircuit* _circuit) :
		pVirtualCircuit(_circuit)
	{
		valStr = QString::number(pVirtualCircuit->val);
	}
	QPoint startIconPt;					// 起点图标位置，图标矩形的左上角点位
	QPoint endIconPt;					// 终点图标位置，图标矩形的左上角点位
	QRect startValRect;					// 起点值矩形，显示当前值
	QRect endValRect;					// 终点值矩形，显示当前值
	QRect circuitDescRect;				// 描述矩形，显示回路描述
	QString valStr;						// 当前值字符串
	QString circuitDesc;				// 回路描述
	VirtualCircuit* pVirtualCircuit;	// 虚回路
};

struct PlateRect
{
	QString iedName;	// 软压板所属IED名称
	quint64 code;	// 软压板ID
	QRect rect;			// 软压板矩形
	QString ref;		// 软压板路径引用，PL2205NAPROT/PTRC1.TrStrp
	QString desc;		// 软压板名称
};

struct VirtualSvg : public LogicSvg
{
	VirtualSvg()
	{
	}
	~VirtualSvg()
	{
	}
	//QList<QRect> plateRectList;			// 软压板矩形列表
	QHash<QString, PlateRect> plateRectHash;	// 软压板矩形列表，key为软压板路径
};
// 完整虚实回路svg图
enum WholeGroupMode
{
	WholeGroupMode_Fallback = 0,
	WholeGroupMode_Direct,
	WholeGroupMode_Switch
};

struct WholeGroupDecor
{
	WholeGroupDecor()
	{
		pLogicLine = NULL;
		groupMode = WholeGroupMode_Fallback;
		leftBraceRect = QRectF();
		rightBraceRect = QRectF();
		centerArrowLine = QLineF();
		switchIconRect = QRectF();
		gapStartX = 0;
		gapEndX = 0;
		hasSwitchIcon = false;
		leftPortRect = QRectF();
		rightPortRect = QRectF();
	}

	LogicCircuitLine* pLogicLine;
	WholeGroupMode groupMode;
	QRectF leftBraceRect;
	QRectF rightBraceRect;
	QLineF centerArrowLine;
	QRectF switchIconRect;
	qreal gapStartX;
	qreal gapEndX;
	bool hasSwitchIcon;
	QString switchIedName;
	QString leftPortText;
	QString rightPortText;
	QRectF leftPortRect;
	QRectF rightPortRect;
};

struct WholeCircuitSvg : public LogicSvg
{
	WholeCircuitSvg()
	{
	}
	~WholeCircuitSvg()
	{
		qDeleteAll(groupDecorList);
		groupDecorList.clear();
	}
	QHash<QString, PlateRect> plateRectHash;
	QList<WholeGroupDecor*> groupDecorList;
};
