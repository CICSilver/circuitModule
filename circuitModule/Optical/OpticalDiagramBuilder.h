#pragma once
#include "Common/DiagramBuilderBase.h"

class OpticalDiagramBuilder : public DiagramBuilderBase
{
public:
	OpticalDiagramBuilder();
	~OpticalDiagramBuilder();
	//************************************
	// 函数名称:	BuildOpticalDiagramByIedName
	// 函数全名:	OpticalDiagramBuilder::BuildOpticalDiagramByIedName
	// 函数权限:	public
	// 函数说明:	按IED名称生成实回路图模型
	// 输入参数:	const QString& iedName	IED名称
	// 输出参数:	无
	// 返回值:	OpticalSvg*
	//************************************
	OpticalSvg* BuildOpticalDiagramByIedName(const QString& iedName);
	//************************************
	// 函数名称:	BuildOpticalDiagramByStation
	// 函数全名:	OpticalDiagramBuilder::BuildOpticalDiagramByStation
	// 函数权限:	public
	// 函数说明:	按站范围生成实回路图模型
	// 输入参数:	无
	// 输出参数:	无
	// 返回值:	OpticalSvg*
	//************************************
	OpticalSvg* BuildOpticalDiagramByStation();
	//************************************
	// 函数名称:	BuildOpticalDiagramByBayName
	// 函数全名:	OpticalDiagramBuilder::BuildOpticalDiagramByBayName
	// 函数权限:	public
	// 函数说明:	按间隔名称生成实回路图模型
	// 输入参数:	const QString& bayName	间隔名称
	// 输出参数:	无
	// 返回值:	OpticalSvg*
	//************************************
	OpticalSvg* BuildOpticalDiagramByBayName(const QString& bayName);

private:
	//************************************
	// 函数名称:	GenerateOpticalDiagramByIed
	// 函数全名:	OpticalDiagramBuilder::GenerateOpticalDiagramByIed
	// 函数权限:	private
	// 函数说明:	根据IED对象生成实回路图模型
	// 输入参数:	const IED* pIed	当前IED对象
	// 输出参数:	OpticalSvg& svg	生成的实回路图模型
	// 返回值:	void
	//************************************
	void GenerateOpticalDiagramByIed(const IED* pIed, OpticalSvg& svg);
	//************************************
	// 函数名称:	SetArrowStateDirect
	// 函数全名:	OpticalDiagramBuilder::SetArrowStateDirect
	// 函数权限:	private
	// 函数说明:	按直连关系设置光纤线路箭头方向
	// 输入参数:	const QList<LogicCircuit*>& inList	流入逻辑回路列表
	// 输入参数:	const QList<LogicCircuit*>& outList	流出逻辑回路列表
	// 输入参数:	const QString& mainIedName	主IED名称
	// 输入参数:	const QString& peerIedName	对端IED名称
	// 输出参数:	OpticalCircuitLine* pLine	待设置箭头的光纤线路
	// 返回值:	void
	//************************************
	void SetArrowStateDirect(const QList<LogicCircuit*>& inList, const QList<LogicCircuit*>& outList, const QString& mainIedName, const QString& peerIedName, OpticalCircuitLine* pLine);
	//************************************
	// 函数名称:	SetArrowStateThroughSwitch
	// 函数全名:	OpticalDiagramBuilder::SetArrowStateThroughSwitch
	// 函数权限:	private
	// 函数说明:	按经交换机关系设置光纤线路箭头方向
	// 输入参数:	const QList<LogicCircuit*>& inList	流入逻辑回路列表
	// 输入参数:	const QList<LogicCircuit*>& outList	流出逻辑回路列表
	// 输入参数:	const QString& mainIedName	主IED名称
	// 输入参数:	const QString& peerIedName	对端IED名称
	// 输出参数:	OpticalCircuitLine* pOppLine	对端光纤线路
	// 输出参数:	OpticalCircuitLine* pMainSwitchLine	主IED到交换机的光纤线路
	// 返回值:	void
	//************************************
	void SetArrowStateThroughSwitch(const QList<LogicCircuit*>& inList, const QList<LogicCircuit*>& outList, const QString& mainIedName, const QString& peerIedName, OpticalCircuitLine* pOppLine, OpticalCircuitLine* pMainSwitchLine);
};
