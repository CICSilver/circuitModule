#pragma once
#include "basemodel.h"
#include <QPoint>
#ifndef SVG_DEFAULT_VALUE
#define SVG_DEFAULT_VALUE
// 默认IED矩形宽高
#define RECT_DEFAULT_WIDTH 280
#define RECT_DEFAULT_HEIGHT 100

// IED矩形 水平间距
#define IED_HORIZONTAL_DISTANCE 300
// 垂直间距
#define IED_VERTICAL_DISTANCE 15
// 虚链路垂直间距
#define CIRCUIT_VERTICAL_DISTANCE 10
// 图标高/宽度
#define ICON_LENGTH 20

#endif

struct CircuitLine;
struct LogicCircuitLine;
class OpticalCircuitLine_old;
struct SvgRect
{
	SvgRect()
	{
		width = RECT_DEFAULT_WIDTH;
		height = RECT_DEFAULT_HEIGHT;
		horizontal_margin = 40;
		vertical_margin = 20;
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
		qDeleteAll(logic_circuit_line_list);
		logic_circuit_line_list.clear();
	}
	//LogicCircuitLine* GetLogicCircuitLineById(quint8 id) const;

	quint16 x;
	quint16 y;
	quint16 width;
	quint16 height;
	quint16 vertical_margin;		// 矩形上下边距
	quint16 horizontal_margin;		// 矩形外边距（外部虚线框边距）
	quint16 extend_height;			// 外部虚线框矩形的高度, IED矩形底边到外部虚线框底边的距离（由回路数量决定）
	quint16 padding;	// 矩形内边距（文本与外部虚线框距离）

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

	QList<LogicCircuitLine*> logic_circuit_line_list;
};

struct IedRect : public SvgRect
{
	//const IED* pIed;
	QString iedName;	// IED名称
	QString iedDesc;	// 设备描述
};

struct SwitcherRect : public SvgRect
{
	QString switcher_name;
	QMap<IedRect*, OpticalCircuitLine_old*> ied_circuit_map;	// IED矩形指针 - 光纤链路
};

struct BaseCircuitLine
{
	//quint8 id;		// 标识在IED中的链路编号，从0开始，仅在SVG内使用
	QPoint startPoint;
	QPoint midPoint;
	QPoint endPoint;

	IedRect* pSrcIedRect;
	IedRect* pDestIedRect;
	//Circuit* pCircuit;
};

// 虚实回路
struct CircuitLine : public BaseCircuitLine
{
	const Circuit* pCircuit;
};

// 逻辑链路
struct LogicCircuitLine : public BaseCircuitLine
{
	LogicCircuitLine()
	{
		pLogicCircuit = NULL;
	}
	LogicCircuit* pLogicCircuit;
};

// 光纤链路，对于指定IED及交换机，链路唯一
struct OpticalCircuitLine_old
{
	OpticalCircuitLine_old()
	{
		isStartArrowExist = false;
		isEndArrowExist = false;
	}
	QString iedName;
	QString switcherName;
	QPoint startPoint;
	QPoint endPoint;
	QString iedPort;
	QString switcherPort;
	bool isStartArrowExist;
	bool isEndArrowExist;

	bool isPortEmpty() const
	{
		return iedPort.isEmpty() || switcherPort.isEmpty();
	}
	void SetPort(const QString& iedPort, const QString& switcherPort)
	{
		this->iedPort = iedPort;
		this->switcherPort = switcherPort;
	}
};
// 连接点箭头状态：入、出、出入
enum ArrowState
{
	Arrow_None = 0x0,
	Arrow_In = 0x01,
	Arrow_Out = 0x10,
	Arrow_InOut = Arrow_In | Arrow_Out
};
//struct OpticalCircuitLine
//{
//	QPoint startPoint;
//	QPoint endPoint;
//	QList<QPoint> midPoints;	// 可能有多个折线点
//	//QPoint midPoint;	// 仅用于直连IED，作折线
//	QPoint src2swPoint;	// 仅用于过交换机的IED，源IED对应的交换机的连接点
//	QPoint sw2destPoint;// 仅用于过交换机的IED，交换机对应目标IED的连接点
//	quint8 arrowState;
//	IedRect* pSrcRect;
//	IedRect* pDestRect;
//	SwitcherRect* pSwRect;
//	OpticalCircuit* pOpticalCircuit;
//};

// 不存在跨交换机线路，交换机本身作为设备存储，连交换机的光纤视为单独光纤
struct OpticalCircuitLine
{
	OpticalCircuitLine()
	{
		lineCode = 0;
		arrowState = Arrow_None;
		pSrcRect = NULL;
		pDestRect = NULL;
		pOpticalCircuit = NULL;
	}
	quint16 lineCode;		// 光纤编号
	QPoint startPoint;			// 光纤起点，位于图像上方，与光纤方向无关
	QPoint endPoint;			// 光纤终点，位于图像下方，与光纤方向无关
	QList<QPoint> midPoints;	// 可能有多个折线点
	IedRect* pSrcRect;			// 输出设备
	IedRect* pDestRect;			// 输入设备
	quint8 arrowState;
	OpticalCircuit* pOpticalCircuit;
};
//// SVG描述结构，管理内存释放
struct BaseSvg
{
	BaseSvg() {}
	~BaseSvg() 
	{
		delete mainIedRect;
		mainIedRect = NULL;
	}
	IedRect* mainIedRect;
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
			ret += rect->logic_circuit_line_list.size();
		}
		return ret;
	}
	size_t GetCircuitSize(const QList<IedRect*>& rectList) const
	{
		size_t ret = 0;
		foreach(IedRect * rect, rectList)
		{
			foreach(LogicCircuitLine * pLine, rect->logic_circuit_line_list)
			{
				ret += pLine->pLogicCircuit->circuitList.size();
			}
		}
		return ret;
	}
public:
	QList<IedRect*> leftIedRectList;
	QList<IedRect*> rightIedRectList;
	QList<IedRect*> descRectList;
};

// 光纤SVG
struct OpticalSvg : public BaseSvg
{
	OpticalSvg() {}
	~OpticalSvg() 
	{
		qDeleteAll(iedRectList);
		qDeleteAll(opticalCircuitLineList);
	};

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
	QList<IedRect*> switcherRectList;		// 仅记录，不做内存管理
	QList<OpticalCircuitLine*> opticalCircuitLineList;
};
struct VirtualCircuitLine : public BaseCircuitLine
{
	
};

struct VirtualSvg : public LogicSvg
{
	QMap<IedRect*, VirtualCircuitLine*> vtLineMap;	// 
};

// 完整虚实回路svg图
struct WholeCircuitSvg : public LogicSvg
{
	WholeCircuitSvg() {}
	~WholeCircuitSvg()
	{
	}


};